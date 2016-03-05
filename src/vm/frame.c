#include "vm/frame.h"
#include <debug.h>
#include "threads/loader.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

// `frametable` is a palloc'ed region of memory big enough to hold
// as many frame_info structs as there are frames in physical memory.
// Note that the frametable only contains info about the pages that are
// currently in physical memory, not pages that have been swapped out.
struct frame_info *frametable;

void frametable_init(void) {
	// init_ram_pages is the number of 4KB pages aka frames in physical RAM
	int frametable_size_in_bytes = init_ram_pages * sizeof(struct frame_info);
	int frametable_size_in_pages = pg_count(frametable_size_in_bytes);
	frametable = palloc_get_multiple(0, frametable_size_in_pages);
}

static struct frame_info *frame_for_page(void *page) {
    // Get the physical address and make sure the
    // given page actually falls on a page boundary
    uintptr_t physical_address = vtop(page);
    ASSERT((int)physical_address % PGSIZE == 0);
    
    // Compute the frametable index from the physical address
    // and return the appropriate frame.
    int index = physical_address / PGSIZE;
    return &frametable[index];
}

static void *page_for_frame(struct frame_info *frame) {
    // Get the index and make sure its valid
    int frame_index = frame - frametable;
    ASSERT(frame_index >= 0 && frame_index < (int)init_ram_pages);
    
    // Compute the physical address for a given index
    // and convert to a virtual address
    uintptr_t physical_address = frame_index * PGSIZE;
    return ptov(physical_address);
}

static struct frame_info* choose_frame_for_eviction(void) {
    // TODO
}

// Creates a new page with the given flags, returning a pointer to this page.
void *frametable_create_page(enum palloc_flags flags) {
    // Try to get a new page from palloc
    void *page = palloc_get_page(flags | PAL_USER);

    if (page == NULL) {
        PANIC("frametable_get_page: out of pages!!"); // TODO remove
        
        // We were out of space!

        // Choose a frame to evict and evict it
        struct frame_info* evict_me_f = choose_frame_for_eviction();
        void* evict_me_p = page_for_frame(evict_me_f);
        struct page_info* evict_me_pi = pagetable_info_for_address(&(thread_current()->page_table), evict_me_p);
        pagetable_evict_page(evict_me_pi);

        // We've freed up some space!
        page = evict_me_p;
    }
 
    // Alright, we've finally gotten a valid page.
    // Let's do any initialization needed for the frame_info entry.
    struct frame_info *frame = frame_for_page(page);
    frame->is_user_page = true;
    
    // Make sure that our page_for_frame and frame_for_page functions work properly.
    // No particular reason to be here, but where else :)?
    ASSERT(page == page_for_frame(frame_for_page(page)));
    
    return page;
}

