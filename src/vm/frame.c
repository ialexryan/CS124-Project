#include "vm/frame.h"
#include <debug.h>
#include <list.h>
#include <stdio.h>
#include "threads/loader.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

// `frametable` is a palloc'ed region of memory big enough to hold
// as many frame_info structs as there are frames in physical memory.
// Note that the frametable only contains info about the pages that are
// currently in physical memory, not pages that have been swapped out.
// Here, the frames are ordered exactly the same as they are laid out
// in physical memory.
struct frame_info *frametable;

// frame_eviction_queue represents an ordering of the frames that is
// unrelated to their ordering in physical memory. It will be used for
// a second chance eviction algorithm.
struct list frame_eviction_queue;

void frametable_init(void) {
	// init_ram_pages is the number of 4KB pages aka frames in physical RAM
	int frametable_size_in_bytes = init_ram_pages * sizeof(struct frame_info);
	int frametable_size_in_pages = pg_count(frametable_size_in_bytes);
	frametable = palloc_get_multiple(0, frametable_size_in_pages);

    list_init(&frame_eviction_queue);
}

// Takes a kernel virtual address and returns a frame_info struct
struct frame_info *frame_for_page(void *page) {
    // Get the physical address and make sure the
    // given page actually falls on a page boundary
    uintptr_t physical_address = vtop(page);
    ASSERT((int)physical_address % PGSIZE == 0);
    
    // Compute the frametable index from the physical address
    // and return the appropriate frame.
    int index = physical_address / PGSIZE;
    return &frametable[index];
}

// Takes a frame_info struct and returns a kernel virtual address
void *page_for_frame(struct frame_info *frame) {
    // Get the index and make sure its valid
    int frame_index = frame - frametable;
    ASSERT(frame_index >= 0 && frame_index < (int)init_ram_pages);
    
    // Compute the physical address for a given index
    // and convert to a kernel virtual address
    uintptr_t physical_address = frame_index * PGSIZE;
    return ptov(physical_address);
}

static struct frame_info* choose_frame_for_eviction(void) {

    ASSERT(!list_empty(&frame_eviction_queue));

    struct frame_info* front_frame;
    do {
        struct list_elem* front_list_elem = list_front(&frame_eviction_queue);
        front_frame = list_entry(front_list_elem, struct frame_info, eviction_queue_list_elem);
        void* kpage_for_front_frame = page_for_frame(front_frame);
        void* upage_for_front_frame = front_frame->user_vaddr;

        list_pop_front(&frame_eviction_queue);
        list_push_back(&frame_eviction_queue, &(front_frame->eviction_queue_list_elem));

        bool has_been_accessed = pagedir_is_accessed(thread_current()->pagedir, kpage_for_front_frame) &&
                                 pagedir_is_accessed(thread_current()->pagedir, upage_for_front_frame);
        bool is_pinned = front_frame->is_pinned;

        if (has_been_accessed || is_pinned) {
            // This isn't our guy, send him to the back of the line
            pagedir_set_accessed(thread_current()->pagedir, kpage_for_front_frame, false);
            pagedir_set_accessed(thread_current()->pagedir, upage_for_front_frame, false);
        } else {
            ASSERT(front_frame->is_user_page);
            // printf("Evicting frame %d with user_vaddr %p\n", front_frame - frametable, (front_frame)->user_vaddr);
            return front_frame;
        }
    } while (true);
}

// Creates a new page with the given flags, returning a pointer to this page.
void *frametable_create_page(enum palloc_flags flags) {  // PAL_USER is implied
    // Try to get a new page from palloc
    // page is a kernel virtual address
    void *page = palloc_get_page(flags | PAL_USER);

    if (page == NULL) {
        // We were out of space!

        // Choose a frame to evict
        struct frame_info* evict_me_f = choose_frame_for_eviction();
        ASSERT(evict_me_f->user_vaddr);
        ASSERT(is_user_vaddr(evict_me_f->user_vaddr));
        struct page_info* evict_me_pi = pagetable_info_for_address(&(thread_current()->pagetable), evict_me_f->user_vaddr);
        ASSERT(evict_me_pi->virtual_address == evict_me_f->user_vaddr);
        ASSERT(evict_me_pi != NULL);
        pagetable_evict_page(evict_me_pi);

        // We've freed up some space!
        page = page_for_frame(evict_me_f);
    }
 
    // Alright, we've finally gotten a valid page.
    // Let's do any initialization needed for the frame_info entry.
    struct frame_info *frame = frame_for_page(page);
    frame->is_user_page = true;
    frame->is_pinned = false;
    frame->user_vaddr = page;

    // Add to the end of the eviction queue
    list_push_back(&frame_eviction_queue, &(frame->eviction_queue_list_elem));
    
    // Make sure that our page_for_frame and frame_for_page functions work properly.
    // No particular reason to be here, but where else :)?
    ASSERT(page == page_for_frame(frame_for_page(page)));
    
    return page;
}

// Frees the given user page
void frametable_free_page(void *page) {
    // Cleanup table entry
    struct frame_info *frame = frame_for_page(page);
    frame->is_user_page = false;
    frame->user_vaddr = NULL;
    
    // Free the page
    palloc_free_page(page);
}
