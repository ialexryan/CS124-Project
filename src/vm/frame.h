#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include <stdbool.h>
#include "page.h"

struct frame_info {
    bool is_user_page;
    bool is_pinned;
    void* user_vaddr;
    struct list_elem eviction_queue_list_elem;
    
    // TODO: Add pointer to supplementry page table entry
    //       so that we can update it when we evict it from
    //       physical memory.
    //   OR: Maybe just to the thread?
    //    Q: What happens when the thread dies?
};

void frametable_init(void);
struct frame_info *frame_for_page(void *page);
void *page_for_frame(struct frame_info *frame);
void *frametable_create_page(enum palloc_flags flags);
void frametable_free_page(void *page);

#endif /* vm/frame.h */
