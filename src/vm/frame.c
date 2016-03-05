#include "vm/frame.h"
#include <debug.h>
#include "threads/loader.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"

// `frametable` is a palloc'ed region of memory big enough to hold
// as many frame_info structs as there are frames in physical memory.
// Note that the frametable only contains info about the pages that are
// currently in physical memory, not pages that have been swapped out.
struct frame_info *frametable;

static inline int div_up(int x, int y) {
    return x / y + (x % y > 0);
}

void frametable_init(void) {
	// init_ram_pages is the number of 4KB pages aka frames in physical RAM
	int frametable_size_in_bytes = init_ram_pages * sizeof(struct frame_info);
	int frametable_size_in_pages = div_up(frametable_size_in_bytes, PGSIZE);
	frametable = palloc_get_multiple(0, frametable_size_in_pages);
}

static struct frame_info *frametable_frame_for_page(void *page) {
    // Get the physical address and make sure the
    // given page actually falls on a page boundary
    uintptr_t physical_address = vtop(page);
    ASSERT((int)physical_address % PGSIZE == 0);
    
    // Compute the frametable index from the physical address
    // and return the appropriate frame.
    int index = physical_address / PGSIZE;
    return &frametable[index];
}

static void *frametable_page_for_frame(struct frame_info *frame) {
    // Get the index and make sure its valid
    int frame_index = frame - frametable;
    ASSERT(frame_index >= 0 && frame_index < (int)init_ram_pages);
    
    // Compute the physical address for a given index
    // and convert to a virtual address
    uintptr_t physical_address = frame_index * PGSIZE;
    return ptov(physical_address);
}

// Creates a new page with the given flags, returning a pointer to this page.
void *frametable_create_page(enum palloc_flags flags) {
    void *page = palloc_get_page(flags | PAL_USER);
    if (page == NULL) {
        // We're out of space!! Oh noes!!!!!!
        // TODO: Do some swapping, probs.
        PANIC("frametable_get_page: out of pages!!");
    } else {
        // Yay, we still have physical memory left!
        // Let's do any initialization needed for the frame_info entry.
        
        struct frame_info *frame = frametable_frame_for_page(page);
        frame->is_user_page = true;
    }
    
    // Make sure that oure page_for_frame and frame_for_page functions work properly.
    // No particular reason to be here, but where else :)?
    ASSERT(page == frametable_page_for_frame(frametable_frame_for_page(page)));
    
    return page;
}

