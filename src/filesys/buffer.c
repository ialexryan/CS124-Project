#include <bitmap.h>
#include <debug.h>
#include <hash.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "filesys/buffer.h"
#include "filesys/filesys.h"
#include "threads/synch.h"

#define BUFFER_SIZE 64
#define UNOCCUPIED UINT32_MAX


// OBJECT DEFINITIONS
struct buffer_entry {
	// hash_elem for the buffer_table
	struct hash_elem hash_elem;

	// The block that currently occupies this cache slot. Used as the
	// key in the hash table.
	block_sector_t occupied_by_sector;

	// If this cache entry has been modified and hasn't
	// been written back to disk yet.
	bool dirty;

	uint8_t storage[BLOCK_SECTOR_SIZE]; // 512 bytes of block data
	struct lock lock;
};


// GLOBAL VARIABLES
static struct buffer_entry buffer[BUFFER_SIZE];
uint8_t buffer_unoccupied_slots;

static struct hash buffer_table;
struct lock buffer_table_lock;


// BUFFER HASH TABLE FUNCTIONS
bool buffer_less(const struct hash_elem *a,
               const struct hash_elem *b,
               void *aux UNUSED) {
    struct buffer_entry *buffer_entry_a = hash_entry(a, struct buffer_entry, hash_elem);
    struct buffer_entry *buffer_entry_b = hash_entry(b, struct buffer_entry, hash_elem);
    
    return buffer_entry_a->occupied_by_sector < buffer_entry_b->occupied_by_sector;
}
unsigned buffer_hash(const struct hash_elem *e, void *aux UNUSED) {
    struct buffer_entry *b = hash_entry(e, struct buffer_entry, hash_elem);
    
    return hash_bytes(&b->occupied_by_sector, sizeof(b->occupied_by_sector));
}


// BUFFER FUNCTIONS
void buffer_init(void) {
	/* A brief note about how we handle unoccupied slots.
       Since slots are only unoccupied for a moment at the very
       beginning and then occupied forevermore, we just keep track
       of how many unoccupied slots are left, counting down from 64 to 0.
       While there are unoccupied slots left, we just fill them up in
       order from 63 to 0. Once there are no unoccupied slots left
       (i.e. unoccupied_slots == 0) then we start evicting things. */
	buffer_unoccupied_slots = BUFFER_SIZE;

	hash_init(&buffer_table, buffer_hash, buffer_less, NULL);

	lock_init(&buffer_table_lock);

	int i;
	for (i = 0; i < BUFFER_SIZE; i++) {
		buffer[i].occupied_by_sector = UNOCCUPIED;
		buffer[i].dirty = false;
		lock_init(&(buffer[i].lock));
	}
}


// This accepts a sector number and looks up the buffer_entry struct for that
// sector. Returns null if the given sector isn't in the cache.
struct buffer_entry *buffer_entry_for_sector(struct hash *buffer_table,
                                             block_sector_t sector) {
    
    ASSERT(lock_held_by_current_thread(&buffer_table_lock)); 

    // Create dummy entry for lookup
    struct buffer_entry lookup_entry;
    lookup_entry.occupied_by_sector = sector;
    
    // Get the buffer_entry associated with the given page.
    struct hash_elem *e = hash_find(buffer_table, &lookup_entry.hash_elem);
    if (e == NULL) {
        return NULL;
    } else {
        return hash_entry(e, struct buffer_entry, hash_elem);
    }
}

// Returns a pointer to an empty buffer slot.
// If there isn't one, takes care of evicting one.
// IMPORTANT: whatever slot is returned will have its lock acquired.
// It is the callers responsibility to release that lock.
struct buffer_entry* buffer_get_free_slot(void) {

	// Any function that calls this function should have already
	// acquired the lock on the buffer table.
	ASSERT(lock_held_by_current_thread(&buffer_table_lock));

	struct buffer_entry* b;

	// First, let's just hope there's an unoccupied slot.
	if (buffer_unoccupied_slots > 0) {
		// The first unoccupied slot we'll fill is index 63, the last is 0
		buffer_unoccupied_slots -= 1;
		lock_acquire(&(buffer[buffer_unoccupied_slots].lock));
		b = &(buffer[buffer_unoccupied_slots]);
		ASSERT(b->occupied_by_sector == UNOCCUPIED);
		return b;
	}

	// If there wasn't one, evict slot 1. We'll revisit
	// this functionality later, obviously.
	lock_acquire(&(buffer[1].lock));
	b = &(buffer[1]);
	hash_delete(&buffer_table, &(b->hash_elem));
	b->occupied_by_sector = UNOCCUPIED;
	return b;
}

void buffer_read(block_sector_t sector, void* buffer) {

	lock_acquire(&buffer_table_lock);
	struct buffer_entry* b;

	// First, let's see if sector is aleady in the buffer
	b = buffer_entry_for_sector(&buffer_table, sector);

	// If it was, fulfill the read from the buffer
	if (b != NULL) {
		lock_release(&buffer_table_lock);
		lock_acquire(&(b->lock));
		memcpy(buffer, &(b->storage), BLOCK_SECTOR_SIZE);
		lock_release(&(b->lock));
		return;
	}

	// If it wasn't, get a new cache slot and
	// read from the disk into that slot.
	b = buffer_get_free_slot();
	lock_release(&buffer_table_lock);
	block_read(fs_device, sector, &(b->storage));
	memcpy(buffer, &(b->storage), BLOCK_SECTOR_SIZE);
	b->occupied_by_sector = sector;
	hash_insert(&buffer_table, &(b->hash_elem));
	lock_release(&(b->lock));
}

void buffer_write(block_sector_t sector, const void* buffer) {

	lock_acquire(&buffer_table_lock);
	struct buffer_entry* b;

	// First, let's see if sector is already in the buffer
	b = buffer_entry_for_sector(&buffer_table, sector);

	// If it was, write to the cache then writeback immediately
	if (b != NULL) {
		lock_release(&buffer_table_lock);
		lock_acquire(&(b->lock));
		memcpy(&(b->storage), buffer, BLOCK_SECTOR_SIZE);
		block_write(fs_device, sector, &(b->storage));
		lock_release(&(b->lock));
		return;
	}

	// If it wasn't, just write to disk.
	lock_release(&buffer_table_lock);
	block_write(fs_device, sector, buffer);

	// lock_acquire(&(buffer[2].lock));
	// memcpy(storage_of_index(2), buffer, BLOCK_SECTOR_SIZE);
	// block_write(fs_device, sector, storage_of_index(2));
	// lock_release(&(buffer[2].lock));
}
