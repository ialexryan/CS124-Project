#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"

struct frame {
	void* page;
};

void frametable_init(void);
void* frametable_get_page(enum palloc_flags flags);

#endif /* vm/frame.h */
