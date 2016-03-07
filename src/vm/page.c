#include "vm/page.h"
#include <hash.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/swap.h"

// MARK: Page Table Element

bool page_less(const struct hash_elem *a,
               const struct hash_elem *b,
               void *aux UNUSED) {
    struct page_info *page_a = hash_entry(a, struct page_info, hash_elem);
    struct page_info *page_b = hash_entry(b, struct page_info, hash_elem);
    
    return page_a->virtual_address < page_b->virtual_address;
}

unsigned page_hash(const struct hash_elem *e, void *aux UNUSED) {
    struct page_info *page = hash_entry(e, struct page_info, hash_elem);
    
    return hash_bytes(&page->virtual_address, sizeof(page->virtual_address));
}

// MARK: Page Table Access

// This accepts any user virtual address and rounds down to the nearest page
// to look up the page_info struct for that address
// Returns null if the given address doesn't exist in supplementary page table
struct page_info *pagetable_info_for_address(struct hash *pagetable,
                                             void *address) {
    void* rounded_address = pg_round_down(address);
    
    // Create dummy entry for lookup
    struct page_info lookup_entry;
    lookup_entry.virtual_address = rounded_address;
    
    // Get the page_info associated with the given page.
    struct hash_elem *e = hash_find(pagetable, &lookup_entry.hash_elem);
    if (e == NULL) {
        return NULL;
    } else {
        return hash_entry(e, struct page_info, hash_elem);
    }
}

// MARK: Page Loading

static void *_pagetable_load_page_from_swap(struct page_info *page);
static void *_pagetable_load_page_from_file(struct page_info *page);
static void *_pagetable_load_page_zero_initialized(struct page_info *page);

// This function should only be called by the page fault handler
void pagetable_load_page(struct page_info *page) {
    void *address;
    
    // Check the state of the page and use the proper loading method
    switch (page->state) {
        case UNINITIALIZED_STATE:
            // Initialize the page using the specified initialization method
            switch (page->initialization_method) {
                case ZERO_INITIALIZATION:
                    // Load the page initializing with all zeros
                    address = _pagetable_load_page_zero_initialized(page);
                    break;
                    
                case FILE_INITIALIZATION:
                    // Load the page from file
                    address = _pagetable_load_page_from_file(page);
                    break;
                    
                default:
                    NOT_REACHED();
            }
            break;
            
        case EVICTED_STATE:
            // Restore the page using the specified restoration method
            switch (page->restoration_method) {
                case SWAP_RESTORATION:
                    // Load the page from swap memory
                    address = _pagetable_load_page_from_swap(page);
                    break;
                    
                case FILE_RESTORATION:
                    // Load the page from file
                    address = _pagetable_load_page_from_file(page);
                    break;
                    
                default:
                    NOT_REACHED();
            }
            break;
            
        case LOADED_STATE:
            PANIC("Unexpected attempt to load an already loaded page.");
            
        default:
            NOT_REACHED();
    }
    
    // Install the page into virtual memory, updating the page table
    pagedir_install_page(page->virtual_address, address, page->writable);
    
    // Update the page state
    page->state = LOADED_STATE;
}

// Private function called by `pagetable_load_page`
// Returns the loaded frame, ready for installation
static void *_pagetable_load_page_from_swap(struct page_info *page) {
    // Perform sanity checks
    ASSERT(page->state == EVICTED_STATE);
    ASSERT(page->restoration_method == SWAP_RESTORATION);
    
    // Create frame page
    void *f = frametable_create_page(0);
    struct frame_info* fi = frame_for_page(f);
    fi->user_vaddr = page->virtual_address;
    ASSERT(is_user_vaddr(page->virtual_address));

    // Swap page into memory
    load_swapped_page_into_frame(page, f);
    
    return f;
}

// Private function called by `pagetable_load_page`
// Returns the loaded frame, ready for installation
static void *_pagetable_load_page_from_file(struct page_info *page) {
    // Perform sanity checks
    ASSERT((page->state == UNINITIALIZED_STATE &&
            page->initialization_method == FILE_INITIALIZATION) ||
           (page->state == EVICTED_STATE &&
            page->restoration_method == FILE_RESTORATION));
    
    // Create frame page
    void *f = frametable_create_page(0);
    struct frame_info* fi = frame_for_page(f);
    fi->user_vaddr = page->virtual_address;
    ASSERT(is_user_vaddr(page->virtual_address));
    
    // Read the file into the newly created page
    off_t bytes_read = file_read_at(page->file_info.file,
                                    f,
                                    page->file_info.num_bytes,
                                    page->file_info.offset);
    
    // Make sure that file was actually long enough to read the number
    // of bytes specified by the page table.
    ASSERT(bytes_read == (off_t)page->file_info.num_bytes);
    
    // If the file was smaller than a page, zero out the rest of the page
    memset((void *)((char *)f + bytes_read), 0, PGSIZE - bytes_read);
    
    return f;
}

// Private function called by `pagetable_load_page`
// Returns the loaded frame, ready for installation
static void *_pagetable_load_page_zero_initialized(struct page_info *page) {
    // Perform sanity checks
    ASSERT(page->state == UNINITIALIZED_STATE);
    ASSERT(page->initialization_method == ZERO_INITIALIZATION);
    
    void *f = frametable_create_page(PAL_ZERO);
    struct frame_info* fi = frame_for_page(f);
    fi->user_vaddr = page->virtual_address;
    ASSERT(is_user_vaddr(page->virtual_address));

    return f;
}

// MARK: Page Eviction

static void _pagetable_evict_page_to_swap(struct page_info *page);
static void _pagetable_evict_page_to_file(struct page_info *page);

// Evict the given page without freeing its frame. Returns the kernel address
// of the page's memory.
void *pagetable_evict_page(struct page_info *page) {
    ASSERT(page->state == LOADED_STATE);

    // Evict the page using a method that matches its restoration
    switch (page->restoration_method) {
        case SWAP_RESTORATION:
            _pagetable_evict_page_to_swap(page);
            break;
            
        case FILE_RESTORATION:
            _pagetable_evict_page_to_file(page);
            break;
            
        default:
            NOT_REACHED();
    }
    
    // Update the page state
    page->state = EVICTED_STATE;
    
    // Uninstall page from virtual memory, updating the page table
    return pagedir_uninstall_page(page->virtual_address);
}

// Private function called by `pagetable_evict_page`
static void _pagetable_evict_page_to_swap(struct page_info *page) {
    // Perform sanity checks
    ASSERT(page->state == LOADED_STATE)
    ASSERT(page->restoration_method == SWAP_RESTORATION);
    
    // Return early if we shouldn't actually swap.
    if (page->swap_info.swap_index == DO_NOT_SWAP_INDEX) return;
    
    add_page_to_swapfile(page);
}

// Private function called by `pagetable_evict_page`
static void _pagetable_evict_page_to_file(struct page_info *page) {
    // Perform sanity checks
    ASSERT(page->state == LOADED_STATE)
    ASSERT(page->restoration_method == FILE_RESTORATION);
    
    // Skip writing the file back if it isn't writable
    if (!page->writable) return;
    
    // Skipp writing pages that aren't dirty back to disk
    if (!pagedir_is_dirty(thread_current()->pagedir, page->virtual_address))
        return;
    
    // Write file to disk
    off_t bytes_written = file_write_at(page->file_info.file,
                                        page->virtual_address,
                                        page->file_info.num_bytes,
                                        page->file_info.offset);
        
    // Make sure that file was actually written to.
    ASSERT(bytes_written == (off_t)page->file_info.num_bytes);
}

// MARK: Page Installation

// Creates an installs a page with the given properties,
// returning the newly installed page
static void _pagetable_install_page(struct hash *pagetable,
                                    struct page_info *page) {
    // Sanity check the address
    ASSERT(pg_ofs(page->virtual_address) == 0);
    ASSERT(is_user_vaddr(page->virtual_address));
    
    // Final setup
    page->state = UNINITIALIZED_STATE;
    
    // Insert the page into the pagetable
    struct hash_elem *existing = hash_insert(pagetable, &page->hash_elem);
    ASSERT(existing == NULL); // Make sure the item isn't already here.
}

// MARK: Segment Mapping

/*! Installs a page for a segment starting at offset OFS in FILE at address
    UPAGE.  In total, num_bytes + ZERO_BYTES bytes of virtual memory will be
    set up for initialization when a page fault occurs, as follows:

        - num_bytes bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + num_bytes must be zeroed.

    The pages initialized by this function must be writable by the user process
    if WRITABLE is true, read-only otherwise.
*/
void pagetable_install_segment(struct hash *pagetable,
                               struct file *file,
                               off_t offset,
                               int32_t num_bytes,
                               int32_t zero_bytes,
                               bool writable,
                               void *address) {
    // Sanity check the input
    ASSERT((num_bytes + zero_bytes) % PGSIZE == 0);
    ASSERT(offset % PGSIZE == 0);
    
    while (num_bytes > 0 || zero_bytes > 0) {
        // Reopen the file
        struct file *reopened_file;
        if (!(reopened_file = file_reopen(file))) {
            PANIC("Failed to reopen file.");
        }
        
        /* Calculate how to fill this page.
           We will read PAGE_num_bytes bytes from FILE
           and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_num_bytes = num_bytes < PGSIZE ? num_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_num_bytes;

        // Allocate a page
        struct page_info *page = malloc(sizeof(struct page_info));
        if (page == NULL) {
            PANIC("Unable to allocate memory to store page_info.");
        }
        
        // Initialize the properties
        page->virtual_address = address;
        page->initialization_method = FILE_INITIALIZATION;
        page->restoration_method = SWAP_RESTORATION;
        page->writable = writable;
        
        // Initialize the load data
        page->file_info.file = reopened_file;
        page->file_info.offset = offset;
        page->file_info.num_bytes = page_num_bytes;
        page->file_info.next = NULL; // We aren't going to unmap.
        
        // Finish page installation
        _pagetable_install_page(pagetable, page);

        /* Advance. */
        num_bytes -= page_num_bytes;
        zero_bytes -= page_zero_bytes;
        offset += PGSIZE;
        address += PGSIZE;
    }
}

// MARK: File Mapping

// Install file pages to the given address of virtual memory, mapping the file
void pagetable_install_file(struct hash *pagetable,
                            struct file *file,
                            bool writable,
                            void *address) {
    
    // Compute how many pages are necessary to store the file
    off_t length = file_length(file);
    int num_pages = pg_count(length);
    
    // Loop over the pages in reverse setting up pointers to the next page
    int i;
    struct page_info *successor = NULL;
    for (i = num_pages - 1; i >= 0; i--) {
        // Reopen the file
        struct file *reopened_file;
        if (!(reopened_file = file_reopen(file))) {
            PANIC("Failed to reopen file.");
        }
        
        // Allocate a page
        struct page_info *page = malloc(sizeof(struct page_info));
        if (page == NULL) {
            PANIC("Unable to allocate memory to store page_info.");
        }
        
        // Initialize the properties
        page->virtual_address = (void *)((char *)address + (i * PGSIZE));
        page->initialization_method = FILE_INITIALIZATION;
        page->restoration_method = FILE_RESTORATION;
        page->writable = writable;
        
        // Compute the number of bytes to read
        bool final_page = i == (num_pages - 1);
        uint32_t num_bytes = final_page ? (length % PGSIZE) : PGSIZE;
        
        // Initialize the load data
        page->file_info.file = reopened_file;
        page->file_info.offset = i * PGSIZE;
        page->file_info.num_bytes = num_bytes;
        page->file_info.next = successor;
        
        // Finish page installation
        _pagetable_install_page(pagetable, page);
        
        // Update the successor variable for chaining pages
        successor = page;
    }
}

// Uninstall a single page of a file.
static void _pagetable_uninstall_file_page(struct page_info *page) {
    // Clean up the page memory
    switch (page->state) {
        case UNINITIALIZED_STATE:
            break; // No cleanup required
            
        case EVICTED_STATE:
            break; // Already written to disk
            
        case LOADED_STATE:
            // Evict the file, writing it to disk
            // Let the frame table know we're no longer using this frame
            frametable_free_page(pagetable_evict_page(page));
            break;
            
        default:
            NOT_REACHED();
    }
    
    // Close the file
    file_close(page->file_info.file);
    
    // Free the page
    free(page);
}

// Uninstall a file from virtual memory by unmapping all pages that
// are part of this file. Assumes that `page` is the first page of
// the file that was installed using `pagetable_install_file`.
void pagetable_uninstall_file(struct page_info *page) {
    // Sanity checks
    ASSERT(page->initialization_method == FILE_INITIALIZATION);
    ASSERT(page->restoration_method == FILE_RESTORATION);
    ASSERT(page->file_info.offset == 0);
    
    // Loop over the list of pages
    struct page_info *previous = NULL;
    do {
        previous = page;
        page = page->file_info.next;
        
        // Uninstall the page
        _pagetable_uninstall_file_page(previous);
    } while (page != NULL);
}

// MARK: Allocation

void pagetable_install_and_load_allocation(struct hash *pagetable,
                                           void *address) {
    // Set up allocation
    pagetable_install_allocation(pagetable, address);
    
    // Eagerly load frame
    pagetable_load_page(pagetable_info_for_address(pagetable, address));
}

void pagetable_install_allocation(struct hash *pagetable, void *address) {
    // Allocate a page
    struct page_info *page = malloc(sizeof(struct page_info));
    if (page == NULL) {
        PANIC("Unable to allocate memory to store page_info.");
    }
    memset(page, 0, sizeof(*page));

    // Initialize the properties
    page->virtual_address = address;
    page->initialization_method = ZERO_INITIALIZATION;
    page->restoration_method = SWAP_RESTORATION;
    page->writable = true;
    
    // Finish page installation
    _pagetable_install_page(pagetable, page);
}

// Uninstall a allocated page from virtual memory. Assumes that
// the file that was installed using `pagetable_allocate`.
void pagetable_uninstall_allocation(struct page_info *page) {
    // Sanity checks
    ASSERT(page->restoration_method == SWAP_RESTORATION);
    // Don't check that it was zero initialized, because this can
    // also be used to uninstall segment memory.
    
    // Clean up the page memory
    switch (page->state) {
        case UNINITIALIZED_STATE:
            break; // No cleanup required
            
        case EVICTED_STATE:
            // Remove page from swap
            delete_swapped_page(page);
            break;
            
        case LOADED_STATE:
            // Evict the file without writing it to swap
            // Let the frame table know we're no longer using this frame
            page->swap_info.swap_index = DO_NOT_SWAP_INDEX;
            frametable_free_page(pagetable_evict_page(page));
            break;
            
        default:
            NOT_REACHED();
    }
    
    // Free the page struct
    free(page);
}

// MARK: Uninstallation

// Uninstall a single page from memory. Note that this will break a file
// chain, so it should not be used unless mass-uninstalling pages.
static void _pagetable_uninstall(struct page_info *page) {
    switch (page->restoration_method) {
        case SWAP_RESTORATION:
            // Uninstall an allocated page
            pagetable_uninstall_allocation(page);
            break;
            
        case FILE_RESTORATION:
            // Uninstall this single page of the file
            // Note that this will break the list, so this should
            // only be used if we're mass-uninstalling all files
            _pagetable_uninstall_file_page(page);
            break;
            
        default:
            NOT_REACHED();
    }
}

void uninstall_page(struct hash_elem *e, void *aux UNUSED) {
    struct page_info *page = hash_entry(e, struct page_info, hash_elem);
    _pagetable_uninstall(page);
}

void pagetable_uninstall_all(struct hash *pagetable) {
    // This does not yet work reliably due to some stack bug.
    hash_clear(pagetable, uninstall_page);
}
