#include <bitmap.h>
#include <debug.h>
#include <stdbool.h>
#include <string.h>
#include "filesys/buffer.h"
#include "filesys/filesys.h"
#include "threads/synch.h"

#define BUFFERCACHE_SIZE 64

struct buffercache_entry {
	bool occupied;
	uint8_t storage[BLOCK_SECTOR_SIZE]; // 512 bytes of block data
	struct lock lock;
};

static struct buffercache_entry buffercache[BUFFERCACHE_SIZE];
static struct bitmap* buffercache_freemap;

// I just got tired of typing this out a lot, it's nothing special
// just gets the void* to a cache entry's storage
void* storage_of_index(int index) {
	ASSERT(index >= 0 && index < BUFFERCACHE_SIZE);
	return &((buffercache[index]).storage);
}

void buffer_init(void) {
	buffercache_freemap = bitmap_create(BUFFERCACHE_SIZE);

	int i;
	for (i = 0; i < BUFFERCACHE_SIZE; i++) {
		buffercache[i].occupied = false;
		lock_init(&(buffercache[i].lock));
	}
}

void buffer_read(block_sector_t sector, void* buffer) {
	
	lock_acquire(&(buffercache[1].lock));
	buffercache[1].occupied = true;
	block_read(fs_device, sector, storage_of_index(1));
	memcpy(buffer, storage_of_index(1), BLOCK_SECTOR_SIZE);
	lock_release(&(buffercache[1].lock));
}

void buffer_write(block_sector_t sector, const void* buffer) {

	lock_acquire(&(buffercache[2].lock));
	buffercache[2].occupied = true;
	memcpy(storage_of_index(2), buffer, BLOCK_SECTOR_SIZE);
	block_write(fs_device, sector, storage_of_index(2));
	lock_release(&(buffercache[2].lock));
}