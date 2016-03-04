#include "vm/frame.h"
#include <debug.h>
#include "threads/loader.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"

// frametable is a palloc'ed region of memory big enough to hold
// as many frame structs as there are frames in physical memory
struct frame* frametable;

static inline int div_up(int x, int y) {
    return x / y + (x % y > 0);
}

void frametable_init(void) {
	// init_ram_pages is the number of 4KB pages aka frames in physical RAM
	int frametable_size_in_bytes = init_ram_pages * sizeof(struct frame);
	int frametable_size_in_pages = div_up(frametable_size_in_bytes, PGSIZE);
	frametable = palloc_get_multiple(0, frametable_size_in_pages);
}

void* frametable_get_page(enum palloc_flags flags) {
	void* page = palloc_get_page(flags | PAL_USER);
	if (page == NULL) { PANIC("frametable_get_page: out of pages!!"); }

	ASSERT((int)vtop(page) % PGSIZE == 0);
	unsigned int frametable_index = (int)vtop(page) / PGSIZE;
	
	struct frame* f = &frametable[frametable_index];
	f->page = page;
	return page;
}
