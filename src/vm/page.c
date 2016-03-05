#include "vm/page.h"
#include <hash.h>
#include <debug.h>
#include <string.h>
#include "threads/malloc.h"

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
        case ALLOCATED_PAGE:
            PANIC("TODO: Loading from swap location is not yet supported.");
            break;
            
        case FILE_PAGE:
            PANIC("TODO: Loading from file is not yet supported.");
            break;
            
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
    page->loaded = false;
}

void pagetable_install_file_page(struct hash *pagetable, struct file *file) {
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
        page->type = FILE_PAGE;
        page->file_info.file = reopened_file;
        page->file_info.index = i;
        page->file_info.next = successor;
        
        // Insert the page into the pagetable
        hash_insert(pagetable, &page->hash_elem);
        
        // TODO: Install page in virtual memory here.
        
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
        // TODO: Uninstall page from virtual memory here.
    } while (page != NULL);
}

