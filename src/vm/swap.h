#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "vm/page.h"

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

void swaptable_init(void);
void add_page_to_swapfile(struct page_info* p);
void load_swapped_page_into_frame(struct page_info* p, void* frame);
void delete_swapped_page(struct page_info* p);

#endif /* vm/swap.h */
