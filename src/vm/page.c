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


/*! Adds a mapping from user virtual address UPAGE to kernel
    virtual address KPAGE to the page table.
    If WRITABLE is true, the user process may modify the page;
    otherwise, it is read-only.
    UPAGE must not already be mapped.
    KPAGE should probably be a page obtained from the user pool
    with palloc_get_page().
    Returns true on success, false if UPAGE is already mapped or
    if memory allocation fails. */
static bool install_page(void *upage, void *kpage, bool writable) {
    struct thread *t = thread_current();

    /* Verify that there's not already a page at that virtual
       address, then map our page there. */
    return (pagedir_get_page(t->pagedir, upage) == NULL &&
            pagedir_set_page(t->pagedir, upage, kpage, writable));
}


bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
    struct page_info *page_a = hash_entry(a, struct page_info, hash_elem);
    struct page_info *page_b = hash_entry(b, struct page_info, hash_elem);
    
    return page_a->virtual_address < page_b->virtual_address;
}

unsigned page_hash(const struct hash_elem *e, void *aux UNUSED) {
    struct page_info *page = hash_entry(e, struct page_info, hash_elem);
    return hash_bytes(&page->virtual_address, sizeof(page->virtual_address));
}

// This accepts any user virtual address and rounds down to the nearest page
// to look up the page_info struct for that address
// Returns null if the given address doesn't exist in our supplementary page table
struct page_info *pagetable_info_for_address(struct hash *pagetable, void *address) {
    void* rounded_address = pg_round_down(address);

    // Create dummy entry for lookup
    struct page_info lookup_entry;
    lookup_entry.virtual_address = rounded_address;
    
    // Get the page_info associated with the given page.
    struct hash_elem *e = hash_find(pagetable, (void *)&lookup_entry);
    if (e == NULL) {
        return e;
    } else {
        return hash_entry(e, struct page_info, hash_elem);
    }
}

// This function should only be called by the page fault handler
void pagetable_load_page(struct page_info *page) {
    ASSERT(!page->loaded);

    // Get a frame to store the page into
    void* available_frame = frametable_create_page(0);
    
    switch (page->type) {
        case ALLOCATED_PAGE: {
            uint8_t *address;
            if (page->allocated_info.swap_index == SWAP_INDEX_UNINITIALIZED) {
                // Zero initialize
                address = frametable_create_page(PAL_ZERO);

            } else {
                // Load from swap table
                load_swapped_page_into_frame(page, available_frame);
            }
            // Install the page into virtual memory
            bool success = install_page(page->virtual_address, address, 0);
            ASSERT(success);
            break;
        }
        case FILE_PAGE: {
            PANIC("TODO: Loading from file is not yet supported.");
            break;
        }
        default:
            NOT_REACHED();
    }
    page->loaded = true;

    // Update the page table
    pagedir_set_page(thread_current()->pagedir,
                         page->virtual_address,
                               available_frame, true);
}

// This function should only be called by the frametable code 
void pagetable_evict_page(struct page_info *page) {
    ASSERT(page->loaded);

    switch (page->type) {
        case ALLOCATED_PAGE:
            add_page_to_swapfile(page);
            break;
            
        case FILE_PAGE:
            PANIC("Writing a file page to disk is not yet supported.");
            
        default:
            NOT_REACHED();
    }
    
    // Uninstall page from virtual memory here
    
    page->loaded = false;

    // Update the page table
    pagedir_clear_page(thread_current()->pagedir, page->virtual_address);
}

void pagetable_install_file_page(struct hash *pagetable, struct file *file, void *address) {
    // Reopen the file
    struct file *reopened_file;
    if (!(reopened_file = file_reopen(file))) {
        PANIC("TODO: Handle file failed to open more elegantly.");
    }
    
    // Compute how many pages are necessary to store the file
    int num_pages = pg_count(file_length(file));
    
    // Loop over the pages in reverse setting up pointers to the next page
    int i;
    struct page_info *successor = NULL;
    for (i = num_pages - 1; i >= 0; i--) {
        
        // Allocate a page
        struct page_info *page = malloc(sizeof(struct page_info));
        memset(page, 0, sizeof(*page));

        // Initialize the properties
        page->virtual_address = (void *)((char *)address + (i * PGSIZE));
        page->type = FILE_PAGE;
        page->file_info.file = reopened_file;
        page->file_info.file_index = i;
        page->file_info.next = successor;
        
        // Insert the page into the pagetable
        hash_insert(pagetable, &page->hash_elem);
        
        // Update the successor variable
        successor = page;
    }
}

void pagetable_uninstall_file_page(struct page_info *page) {
    ASSERT(page->type == FILE_PAGE);
    
    // Write the file back to disk
    if (page->loaded) pagetable_evict_page(page);
    
    // Close the file descriptor
    file_close(page->file_info.file);
    
    // Loop over the list of pages and free them
    struct page_info *previous = NULL;
    do {
        previous = page;
        page = page->file_info.next;
        free(previous);
    } while (page != NULL);
}

void pagetable_allocate_page(struct hash *pagetable, void *address) {
    // Allocate a page
    struct page_info *page = malloc(sizeof(struct page_info));
    memset(page, 0, sizeof(*page));

    // Initialize the properties
    page->virtual_address = address;
    page->type = ALLOCATED_PAGE;
    page->allocated_info.swap_index = SWAP_INDEX_UNINITIALIZED;
    
    // Insert the page into the pagetable
    hash_insert(pagetable, &page->hash_elem);
}

