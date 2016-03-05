#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <stdbool.h>
#include "filesys/file.h"

struct page_info {
    void *virtual_address; // key
    struct hash_elem hash_elem;
    
    enum {
        FRAME_LOCATION,
        SWAP_LOCATION,
        DISK_LOCATION
    } storage_location;
    
    union {
        struct {
            void *memory;
        } frame_storage;
        struct {
            int index;
        } swap_storage;
        struct {
            struct file *file;
        } disk_storage;
    };
};

bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);
unsigned page_hash(const struct hash_elem *e, void *aux);

struct page_info *pagetable_info_for_address(struct hash *pagetable, void *address);
void pagetable_load_page(struct page_info *page);
void pagetable_evict_page(struct page_info *page);

void pagetable_install_disk_page(struct hash *pagetable, struct file *file);

#endif /* vm/page.h */
