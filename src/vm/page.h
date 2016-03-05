#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <stdbool.h>

struct page_info {
    void *address; // key
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

#endif /* vm/page.h */
