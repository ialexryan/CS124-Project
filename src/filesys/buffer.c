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
// Precondition: Must hold the buffer_table_lock.
struct buffer_entry *_buffer_entry_for_sector(block_sector_t sector) {

    ASSERT(lock_held_by_current_thread(&buffer_table_lock)); 

    // Create dummy entry for lookup
    struct buffer_entry lookup_entry;
    lookup_entry.occupied_by_sector = sector;
    
    // Get the buffer_entry associated with the given page.
    struct hash_elem *e = hash_find(&buffer_table, &lookup_entry.hash_elem);
    if (e == NULL) {
        return NULL;
    } else {
        return hash_entry(e, struct buffer_entry, hash_elem);
    }
}

// This accepts a sector number and looks up the buffer_entry struct for that
// sector. Returns null if the given sector isn't in the cache.
// Precondition: The global lock is held by the current thread.
// Postcondition: The global lock is released only if a buffer entry is returned.
struct buffer_entry *buffer_acquire_existing_entry(block_sector_t sector) {
    ASSERT(lock_held_by_current_thread(&buffer_table_lock));
    
    // First grab the entry from the sector.
    struct buffer_entry* b = _buffer_entry_for_sector(sector);
    
    // The buffer doesn't contain an entry for this sector.
    // Return WITHOUT releasing the global lock!
    if (b == NULL) return NULL;

    // We found it, so let's avoid locking on the global lock.
    // And make sure nobody is trying to steal our buffer entry!
    lock_release(&buffer_table_lock);
    lock_acquire(&b->lock);
    
    // Is this the same buffer entry we thought we retrieved?
    while (b->occupied_by_sector != sector) {
        // Nope, guess someone swapped it out. Rude.
        
        // Let go of this imposter and grab the global lock.
        lock_release(&b->lock);
        lock_acquire(&buffer_table_lock);
        
        // Let's try that again..
        b = _buffer_entry_for_sector(sector);
        
        // We found it, so let's avoid locking on the global lock.
        // And make sure nobody is trying to steal our buffer entry!
        lock_release(&buffer_table_lock);
        lock_acquire(&b->lock);
    }
    
    // Guess we acquired it.
    return b;
}

// Returns a pointer to an empty buffer slot.
// If there isn't one, takes care of evicting one.
// Precondition: The global lock is held by the current thread.
// Postcondition: The global lock is released, and
//                the buffer entry's lock is acquired.
struct buffer_entry* buffer_acquire_free_slot(void) {

	// Any function that calls this function should have already
	// acquired the lock on the buffer table.
	ASSERT(lock_held_by_current_thread(&buffer_table_lock));

    struct buffer_entry *b;
	// Check if there's any unoccupied slots
	if (buffer_unoccupied_slots > 0) {
		// The first unoccupied slot we'll fill is index 63, the last is 0
		buffer_unoccupied_slots -= 1;
        b = &(buffer[buffer_unoccupied_slots]);
        lock_acquire(&b->lock);
        ASSERT(b->occupied_by_sector == UNOCCUPIED);

        // We can release the global lock after acquiring the buffer lock
        // since we can't block---nobody to wait on for an unused buffer.
        lock_release(&buffer_table_lock);
	}
    // Otherwise, evict an existing buffer entry...
    else {
        // Naively evict slot 1. We'll revisit
        // this functionality later, obviously.
        b = &buffer[1];
        lock_acquire(&b->lock);
        hash_delete(&buffer_table, &(b->hash_elem));
        b->occupied_by_sector = UNOCCUPIED;

        // TODO: Uncertain if we ought to block on the table lock in the
        //       case of eviction. Reconsider later.
        lock_release(&buffer_table_lock);
    }
    return b;
}

void buffer_read(block_sector_t sector, void* buffer) {
    // Acquire the global lock. Will be automatically released
    // when an existing entry or free slot is acquired.
    lock_acquire(&buffer_table_lock);
    struct buffer_entry* b;
    
    // First, let's see if sector is aleady in the buffer.
    b = buffer_acquire_existing_entry(sector);
    
    // If this sector isn't yet loaded, load it here...
    if (b == NULL) {
        // Grab a buffer entry, lock acquired, and set it up.
        b = buffer_acquire_free_slot();
        b->occupied_by_sector = sector;
        
        // Read the on-disk data into the buffer.
        block_read(fs_device, sector, &(b->storage));
    }
    
    // Copy the data into the buffer
    memcpy(buffer, &(b->storage), BLOCK_SECTOR_SIZE);

    // Release the lock on the buffer.
    lock_release(&(b->lock));
}

void buffer_write(block_sector_t sector, const void* buffer) {

    // Obtain the global lock to look up a buffer entry. Note that it
    // will be released automatically if we find an entry, otherwise
    // it must be released manually.
	lock_acquire(&buffer_table_lock);
    
	struct buffer_entry* b;
	// Try to find a cache entry.
    if ((b = buffer_acquire_existing_entry(sector))) {
        // Copy the buffer data into the cache.
        memcpy(&(b->storage), buffer, BLOCK_SECTOR_SIZE);

        // We're done writing, so release the lock.
        lock_release(&b->lock);
    } else {
        // Release the buffer lock.
        lock_release(&buffer_table_lock);
        
        // Just write to disk.
        block_write(fs_device, sector, buffer);
    }
}
