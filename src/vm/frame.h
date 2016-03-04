#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include <stdbool.h>

struct frame {
	void* page;
    bool swappable;
};

void frametable_init(void);
void* frametable_get_page(enum palloc_flags flags);

#endif /* vm/frame.h */
