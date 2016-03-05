#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include <stdbool.h>

struct frame_info {
    bool is_user_page;
    
    // TODO: Add pointer to supplementry page table entry
    //       so that we can update it when we evict it from
    //       physical memory.
};

void frametable_init(void);
void *frametable_create_page(enum palloc_flags flags);

#endif /* vm/frame.h */
