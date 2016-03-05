#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <stdbool.h>
#include "filesys/file.h"
#include "threads/vaddr.h"

#define SWAP_INDEX_UNINITIALIZED -1

struct page_info {
    // Requirements for member of hash map
    void *virtual_address; // key
    struct hash_elem hash_elem;
    
    // The type of page
    enum {
        ALLOCATED_PAGE,
        FILE_PAGE
    } type;
    bool loaded;

    // Data used for loading the page
    union {
        /* ALLOCATED_PAGE */
        struct {
            int swap_index;
        } allocated_info;
        
        /* FILE_PAGE */
        struct {
            struct file *file;
            struct page_info *next;
            int file_index;
        } file_info;
    };
};

bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);
unsigned page_hash(const struct hash_elem *e, void *aux);

struct page_info *pagetable_info_for_address(struct hash *pagetable, void *address);
void pagetable_load_page(struct page_info *page);
void pagetable_evict_page(struct page_info *page);

void pagetable_install_file_page(struct hash *pagetable, struct file *file, void *address);
void pagetable_uninstall_file_page(struct page_info *page);

void pagetable_allocate_page(struct hash *pagetable, void *address);

#endif /* vm/page.h */
