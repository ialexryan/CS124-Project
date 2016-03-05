#include "vm/page.h"
#include <hash.h>
#include <debug.h>

bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
    struct page_info *page_a = hash_entry(a, struct page_info, hash_elem);
    struct page_info *page_b = hash_entry(b, struct page_info, hash_elem);
    
    return page_a->page_address < page_b->page_address;
}

unsigned page_hash(const struct hash_elem *e, void *aux UNUSED) {
    struct page_info *page = hash_entry(e, struct page_info, hash_elem);
    return hash_bytes(&page->page_address, sizeof(page->page_address));
}

struct page_info *pagetable_info_for_page(struct hash *pagetable, void *page) {
    // Create dummy entry for lookup
    struct page_info lookup_entry;
    lookup_entry.page_address = page;
    
    // Get the page_info associated with the given page.
    struct hash_elem *e = hash_find(pagetable, (void *)&lookup_entry);
    return hash_entry(e, struct page_info, hash_elem);
}