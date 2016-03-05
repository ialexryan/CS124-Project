#include "vm/page.h"
#include <hash.h>
#include <debug.h>

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

void pagetable_load_page_if_needed(struct page_info *page UNUSED) {
    switch (page->storage_location) {
        case frame_location:
            // Already loaded!
            break;
            
        case swap_location:
            PANIC("TODO: Loading from swap location is not yet supported.");
            break;
            
        case disk_location:
            PANIC("TODO: Loading from disk location is not yet supported.");
            break;
            
        default:
            NOT_REACHED();
    }
}

void pagetable_unload_page_if_needed(struct page_info *page) {
    switch (page->storage_location) {
        case frame_location:
            PANIC("TODO: Unloading frame is not yet supported.");
            break;
            
        case swap_location:
            // Already unloaded
            break;
            
        case disk_location:
            PANIC("It doesn't make sense to unload a disk location.");
            
        default:
            NOT_REACHED();
    }
}
