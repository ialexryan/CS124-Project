#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <stdbool.h>

struct page_info {
    void *page_address; // key
    struct hash_elem hash_elem;
    
    enum {
        frame_location,
        swap_location,
        disk_location
    } storage_location;
    
    union {
        struct {
            void *memory;
        } frame_storage;
        struct {
            int index;
        } swap_storage;
        struct {
            
        } disk_storage;
    };
};

bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);
unsigned page_hash(const struct hash_elem *e, void *aux);

struct page_info *pagetable_info_for_page(struct hash *pagetable, void *page);

#endif /* vm/page.h */
