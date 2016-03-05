#include "vm/page.h"
#include <hash.h>
#include <debug.h>
#include <string.h>
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "vm/frame.h"

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

struct page_info *pagetable_info_for_address(struct hash *pagetable, void *address) {
    // Create dummy entry for lookup
    struct page_info lookup_entry;
    lookup_entry.virtual_address = address;
    
    // Get the page_info associated with the given page.
    struct hash_elem *e = hash_find(pagetable, (void *)&lookup_entry);
    return hash_entry(e, struct page_info, hash_elem);
}

// This function should only be called by the page fault handler
void pagetable_load_page(struct page_info *page) {
    ASSERT(!page->loaded);
    
    switch (page->type) {
        case ALLOCATED_PAGE: {
            uint8_t *address;
            if (page->allocated_info.swap_index == SWAP_INDEX_UNINITIALIZED) {
                // Zero initialize
                address = frametable_create_page(PAL_ZERO);

            } else {
                // Load from swap table
                PANIC("TODO: Loading from swap location is not yet supported.");
            }
            // Install the page into virtual memory
            bool success = install_page(page->virtual_address, address, true); // TODO: Writable?
            ASSERT(success);
            break;
        }
        case FILE_PAGE: {
            // Create frame page
            uint8_t *address = frametable_create_page(0);
            
            // Compute the proper file offset to start reading from
            int file_index = page->file_info.file_index;
            off_t file_offset = file_index * PGSIZE;
            
            // Read the file into the newly created page
            off_t bytes_read = file_read_at(page->file_info.file, address, PGSIZE, file_offset);
            
            // If the file was smaller than a page, zero out the rest of the page
            if (bytes_read < PGSIZE) {
                memset((void *)((char *)address + bytes_read), 0, PGSIZE - bytes_read);
            }
            
            /* Add the page to the process's address space. */
            if (!install_page(page->virtual_address, address, true)) { // TODO: Writable?
                PANIC("TODO: Handle unable to install page.");
            }
            
            break;
        }
        default:
            NOT_REACHED();
    }
    page->loaded = true;
}

// This function should only be called by the frametable code 
void pagetable_evict_page(struct page_info *page) {
    ASSERT(page->loaded);

    switch (page->type) {
        case ALLOCATED_PAGE:
            PANIC("TODO: Swapping a page is not yet supported.");
            break;
            
        case FILE_PAGE:
            PANIC("Writing a file page to disk is not yet supported.");
            
        default:
            NOT_REACHED();
    }
    
    // Uninstall page from virtual memory here
    
    page->loaded = false;
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

