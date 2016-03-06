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
    void *frame = frametable_create_page(0);
    
    // Swap page into memory
    load_swapped_page_into_frame(page, frame);
    
    return frame;
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
    void *frame = frametable_create_page(0);
    
    // Read the file into the newly created page
    off_t bytes_read = file_read_at(page->file_info.file,
                                    frame,
                                    page->file_info.read_bytes,
                                    page->file_info.offset);
    
    // Make sure that file was actually long enough to read the number
    // of bytes specified by the page table.
    ASSERT(bytes_read == (off_t)page->file_info.read_bytes);
    
    // If the file was smaller than a page, zero out the rest of the page
    memset((void *)((char *)frame + bytes_read), 0, PGSIZE - bytes_read);
    
    return frame;
}

// Private function called by `pagetable_load_page`
// Returns the loaded frame, ready for installation
static void *_pagetable_load_page_zero_initialized(struct page_info *page) {
    // Perform sanity checks
    ASSERT(page->state == UNINITIALIZED_STATE);
    ASSERT(page->initialization_method == ZERO_INITIALIZATION);
    
    return frametable_create_page(PAL_ZERO);
}

// MARK: Page Eviction

static void _pagetable_evict_page_to_swap(struct page_info *page);
static void _pagetable_evict_page_to_file(struct page_info *page);

// This function should only be called by the frametable code 
void pagetable_evict_page(struct page_info *page) {
    ASSERT(page->state == LOADED_STATE);

    // Evict the page using a method that matches its restoration
    switch (page->restoration_method) {
        case SWAP_RESTORATION:
            ASSERT(page->writable);
            _pagetable_evict_page_to_swap(page);
            break;
            
        case FILE_RESTORATION:
            _pagetable_evict_page_to_file(page);
            break;
            
        default:
            NOT_REACHED();
    }
    
    // Uninstall page from virtual memory, updating the page table
    pagedir_uninstall_page(page->virtual_address);
    
    // Update the page state
    page->state = EVICTED_STATE;
}

// Private function called by `pagetable_evict_page`
static void _pagetable_evict_page_to_swap(struct page_info *page) {
    add_page_to_swapfile(page);
}

// Private function called by `pagetable_evict_page`
static void _pagetable_evict_page_to_file(struct page_info *page) {
    PANIC("Writing a file page to disk is not yet supported.");
}

// MARK: Page Installation

// Creates an installs a page with the given properties,
// returning the newly installed page
static void _pagetable_install_page(struct hash *pagetable,
                                    struct page_info *page) {
    // Sanity check the address
    ASSERT(pg_ofs(page->virtual_address) == 0);
    
    // Final setup
    page->state = UNINITIALIZED_STATE;
    
    // Insert the page into the pagetable
    struct hash_elem *existing = hash_insert(pagetable, &page->hash_elem);
    ASSERT(existing == NULL); // Make sure the item isn't already here.
}

// MARK: Segment Mapping

/*! Installs a page for a segment starting at offset OFS in FILE at address
    UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual memory will be
    set up for initialization when a page fault occurs, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

    The pages initialized by this function must be writable by the user process
    if WRITABLE is true, read-only otherwise.
*/
void pagetable_install_segment(struct hash *pagetable,
                               struct file *file,
                               off_t offset,
                               int32_t read_bytes,
                               int32_t zero_bytes,
                               bool writable,
                               void *address) {
    // Sanity check the input
    ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
    ASSERT(offset % PGSIZE == 0);

    // Reopen the file
    struct file *reopened_file;
    if (!(reopened_file = file_reopen(file))) {
        PANIC("TODO: Handle file failed to open more elegantly.");
    }
    
    while (read_bytes > 0 || zero_bytes > 0) {
        /* Calculate how to fill this page.
           We will read PAGE_READ_BYTES bytes from FILE
           and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

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
        page->file_info.read_bytes = page_read_bytes;
        page->file_info.next = NULL; // We aren't going to unmap.
        
        // Finish page installation
        _pagetable_install_page(pagetable, page);

        /* Advance. */
        read_bytes -= page_read_bytes;
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
    // Reopen the file
    struct file *reopened_file;
    if (!(reopened_file = file_reopen(file))) {
        PANIC("TODO: Handle file failed to open more elegantly.");
    }
    
    // Compute how many pages are necessary to store the file
    off_t length = file_length(file);
    int num_pages = pg_count(length);
    
    // Loop over the pages in reverse setting up pointers to the next page
    int i;
    struct page_info *successor = NULL;
    for (i = num_pages - 1; i >= 0; i--) {
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
        uint32_t read_bytes = final_page ? (length % PGSIZE) : PGSIZE;
        
        // Initialize the load data
        page->file_info.file = reopened_file;
        page->file_info.offset = i * PGSIZE;
        page->file_info.read_bytes = read_bytes;
        page->file_info.next = successor;
        
        // Finish page installation
        _pagetable_install_page(pagetable, page);
        
        // Update the successor variable for chaining pages
        successor = page;
    }
}

// Uninstall a file from virtual memory by unmapping all pages that
// are part of this file. Assumes that `page` is the first page of
// the file that was installed using `pagetable_install_file_pages`.
void pagetable_uninstall_file(struct page_info *page) {
    // Sanity checks
    ASSERT(page->initialization_method == FILE_INITIALIZATION);
    ASSERT(page->restoration_method == FILE_RESTORATION);
    ASSERT(page->file_info.offset == 0);
    
    // Save the file to later close
    struct file *file = page->file_info.file;
    
    // Loop over the list of pages
    struct page_info *previous = NULL;
    do {
        previous = page;
        page = page->file_info.next;
        
        // Write the page back to disk, if necessary
        if (previous->state == LOADED_STATE) pagetable_evict_page(previous);
        
        // Free the page
        free(previous);
    } while (page != NULL);
    
    // Close the file descriptor after we've written stuff back to disk
    file_close(file);
}

// MARK: Allocation

void pagetable_allocate(struct hash *pagetable, void *address) {
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

