#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include <stdbool.h>

struct frame_info {
    int TEMP_PLACEHOLDER_REMOVE_ME_LATER;
};

void frametable_init(void);
void *frametable_create_page(enum palloc_flags flags);

#endif /* vm/frame.h */
