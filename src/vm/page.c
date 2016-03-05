#include "vm/page.h"
#include <hash.h>

bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
    struct page_info *page_a = hash_entry(a, struct page_info, hash_elem);
    struct page_info *page_b = hash_entry(b, struct page_info, hash_elem);
    
    return page_a->address < page_b->address;
}

unsigned page_hash(const struct hash_elem *e, void *aux) {
    struct page_info *page = hash_entry(e, struct page_info, hash_elem);
    return hash_bytes(&page->address, sizeof(page->address));
}