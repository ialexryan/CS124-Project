#include "vm/swap.h"
#include <bitmap.h>
#include <debug.h>
#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

static struct bitmap* swapmap; // This has an "occupied" boolean bit for
                        // each 4KB page in the swap file.
static struct block* swap_block;
static struct lock swap_lock;

void swaptable_init(void) {
	swap_block = block_get_role(BLOCK_SWAP);

	lock_init(&swap_lock);

	block_sector_t swapfile_numsectors = block_size(swap_block);
	uint32_t swapfile_numbytes = swapfile_numsectors * BLOCK_SECTOR_SIZE;
	uint32_t swapfile_numpages = swapfile_numbytes / PGSIZE;

	swapmap = bitmap_create(swapfile_numpages);
}

void add_page_to_swapfile(struct page_info* p) {
	lock_acquire(&swap_lock);

	// Find the first open 4KB slot in the swap file
	unsigned int index = bitmap_scan_and_flip(swapmap, 0, 1, false);
	ASSERT(index != BITMAP_ERROR);
	ASSERT(bitmap_test(swapmap, index));  // should be true i.e. occupied

	// Write out the passed-in page to that 4KB slot
	// We're going to need to write out eight sectors, one at a time
	block_sector_t sector_index = index * SECTORS_PER_PAGE;
	int i;
	for(i = 0; i < SECTORS_PER_PAGE; i++) {
		block_sector_t target_sector = sector_index + i;
		void* source_buf = (uint8_t *)p->virtual_address + (i * BLOCK_SECTOR_SIZE);
		block_write(swap_block, target_sector, source_buf);
	}

	p->swap_info.swap_index = index;
	lock_release(&swap_lock);
}

void load_swapped_page_into_frame(struct page_info* p, void* frame) {
	lock_acquire(&swap_lock);
	int index = p->swap_info.swap_index;
	bitmap_reset(swapmap, index);

	// Read the 4KB page from disk to the passed-in frame
	// We're going to need to read out eight sectors, one at a time
	block_sector_t sector_index = index * SECTORS_PER_PAGE;
	int i;
	for (i = 0; i < SECTORS_PER_PAGE; i++) {
		block_sector_t source_sector = sector_index + i;
		void* target_buf = (uint8_t *)frame + (i * BLOCK_SECTOR_SIZE);
		block_read(swap_block, source_sector, target_buf);
	}

	lock_release(&swap_lock);
}

void delete_swapped_page(struct page_info* p) {
    lock_acquire(&swap_lock);
    int index = p->swap_info.swap_index;
    bitmap_reset(swapmap, index);
    lock_release(&swap_lock);
}
