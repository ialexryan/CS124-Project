#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include <stdbool.h>
#include "page.h"

// Each instance of this struct stores metadata about one physical frame
struct frame_info {
    bool is_user_page;
    bool is_pinned;
    void* user_vaddr;
    struct list_elem eviction_queue_list_elem;
};

void frametable_init(void);
struct frame_info *frame_for_page(void *page);
void *page_for_frame(struct frame_info *frame);
void *frametable_create_page(enum palloc_flags flags);
void frametable_free_page(void *page);

#endif /* vm/frame.h */
