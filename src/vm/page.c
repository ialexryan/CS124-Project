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

void pagetable_install_disk_page(struct hash *pagetable, struct file *file) {
    struct page_info *page = malloc(sizeof(struct page_info));
    memset(page, 0, sizeof(*page));

    page->type = FILE_PAGE;
    page->file = file;
    
    hash_insert(pagetable, &page->hash_elem);
}

