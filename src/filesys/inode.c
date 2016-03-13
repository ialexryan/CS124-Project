#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/buffer.h"
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

// Hardcoded pow for compile-time optimization
#define pow(a, b) ((b == 0) ? 1 : (b == 1) ? a : \
    (b == 2) ? a * a : (PANIC("Unsupported power."), -1))

/*! Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

// The number of sector entries on a given indirection. Provides
// the base for the exponential growth of capacity with increased
// levels of indirection.
#define SECTORS_PER_INDIRECTION 4

// The levels of indirection.
#define LEVEL(NAME, _) NAME ## _LEVEL,
enum indirection_level {
    _INDIRECTION
    INDIRECTION_LEVEL_COUNT
};
#undef LEVEL

// The number of sectors in a root object `inode_data` that provide the
// given level of indirection.
#define LEVEL(NAME, COUNT) case NAME ## _LEVEL: return COUNT;
static inline size_t num_inode_root_sectors(enum indirection_level level) {
    switch (level) {
            _INDIRECTION
        default: NOT_REACHED();
    }
}
#undef LEVEL

// Sector used for indirect lookup
struct indirect_sector {
    block_sector_t sectors[SECTORS_PER_INDIRECTION];
};

// The number of inode sectors that can be stored using a given level of indirection.
static inline size_t num_sectors_per_level(enum indirection_level level) {
    return pow(SECTORS_PER_INDIRECTION, level);
}

// The number of sectors that an inode of a given level can store given the count of each
// level of indirection on the root `inode_data`.
static inline size_t total_inode_capacity(enum indirection_level level) {
    return num_sectors_per_level(level) * num_inode_root_sectors(level);
}

// The index after the last index associated with a given inode level.
static inline size_t inode_sector_end_index(enum indirection_level level) {
    if ((int)level < 0) return 0;
    return inode_sector_end_index(level - 1) + total_inode_capacity(level);
}

// The first index associated with a given inode level.
static inline size_t inode_sector_start_index(enum indirection_level level) {
    return inode_sector_end_index(level - 1);
}

// The level of indirection associated with an inode index.
static enum indirection_level level_for_inode_index(size_t index) {
    enum indirection_level level;
    for (level = DIRECT_LEVEL; level < INDIRECTION_LEVEL_COUNT; level++) {
        if (inode_sector_end_index(level) > index) return level;
    }
    return -1;
}

// The number of root sectors (that is, sectors on an an `inode_data`) that are below the
// target level.
static size_t num_inode_root_sectors_below_level(enum indirection_level target_level) {
    size_t count = 0;
    enum indirection_level level;
    for (level = DIRECT_LEVEL; level < target_level; level++) {
        count += num_inode_root_sectors(level);
    }
    return count;
}

// Given an index, the array of sector data, and the indirection level of this data,
// computes the corresponding sector.
static block_sector_t _sector_at_indirect_index(size_t index, block_sector_t source_sector, enum indirection_level level) {
    //	assert(level >= 0);
    
    // The index of our sector in the level is simply the given index.
    size_t index_in_level = index;
    size_t sectors_per_level = num_sectors_per_level(level);
    
    // The index of the sector in this level is simply the index in this level
    // divided by the number of indices that each sector handles
    size_t index_of_sector_in_level = index_in_level / sectors_per_level;
    
    // The index of our target sector in our sector is whatever is left after travelling
    // to the right sector on this level.
    size_t index_in_sector = index_in_level % sectors_per_level;
    
    // Sector index is the same as the index of the sector in the level since there
    // are no other levels in our indirect sector data.
    size_t index_of_sector = index_of_sector_in_level;
    block_sector_t sector = buffer_read_member(source_sector,
                                               struct indirect_sector,
                                               sectors[index_of_sector]);

    // If we're on the direct level, return the sector. Otherwise, recurse on looking
    // up the sector in the next lowest indirection level.
    if (level == DIRECT_LEVEL) return sector;
    else return _sector_at_indirect_index(index_in_sector, sector, level - 1);
}

// Given an index and an inode data structure, computes the corresponding sector.
static block_sector_t sector_at_inode_index(size_t index, struct inode *inode) {
    // Determine the level from the inode index
    enum indirection_level level = level_for_inode_index(index);
    //	assert(level >= 0);
    
    // The index of our sector in the level is the given index offset by this
    // level's start index, subtracting sectors of lower levels.
    size_t index_in_level = index - inode_sector_start_index(level);
    size_t sectors_per_level = num_sectors_per_level(level);
    
    // The index of the sector in this level is simply the index in this level
    // divided by the number of indices that each sector handles
    size_t index_of_sector_in_level = index_in_level / sectors_per_level;
    
    // The index of our target sector in our sector is whatever is left after travelling
    // to the right sector on this level.
    size_t index_in_sector = index_in_level % sectors_per_level;
    
    // Sector index is the index of the sector in the level plus the number of sectors
    // in the levels below.
    size_t index_of_sector = index_of_sector_in_level + num_inode_root_sectors_below_level(level);
    block_sector_t sector = buffer_read_member(inode->sector, struct inode_data,
                                               sectors[index_of_sector]);
				
    // If we're on the direct level, return the sector. Otherwise, recurse on looking
    // up the sector in the next lowest indirection level. Note that we will not again
    // check the root inode data sector, but an indirection sector.
    if (level == DIRECT_LEVEL) return sector;
    else return _sector_at_indirect_index(index_in_sector, sector, level - 1);
}

/*! Returns the number of sectors to allocate for an inode SIZE
    bytes long. */
static inline size_t bytes_to_sectors(off_t size) {
    return DIV_ROUND_UP(size, BLOCK_SECTOR_SIZE);
}

/*! Returns the block device sector that contains byte offset POS
    within INODE.
    Returns -1 if INODE does not contain data for a byte at offset
    POS. */
//static block_sector_t byte_to_sector(const struct inode *inode, off_t pos) {
//    ASSERT(inode != NULL);
//    struct inode_data data = buffer_read_struct(inode->sector, struct inode_data);
//    if (pos < data.length)
//        return data.start + pos / BLOCK_SECTOR_SIZE;
//    else
//        return -1;
//}

/*! List of open inodes, so that opening a single inode twice
    returns the same `struct inode'. */
static struct list open_inodes;

/*! Initializes the inode module. */
void inode_init(void) {
    list_init(&open_inodes);
}

/*! Initializes an inode with LENGTH bytes of data and
    writes the new inode to sector SECTOR on the file system
    device.
    Returns true if successful.
    Returns false if memory or disk allocation fails. */
bool inode_create(block_sector_t sector, off_t length) {
    struct inode_disk *disk_inode = NULL;
    bool success = false;

    ASSERT(length >= 0);

    /* If this assertion fails, the inode structure is not exactly
       one sector in size, and you should fix that. */
    ASSERT(sizeof *disk_inode == BLOCK_SECTOR_SIZE);

    disk_inode = calloc(1, sizeof *disk_inode);
    if (disk_inode != NULL) {
        size_t sectors = bytes_to_sectors(length);
        disk_inode->length = length;
        disk_inode->is_directory = false;
        disk_inode->magic = INODE_MAGIC;
        if (free_map_allocate(sectors, &disk_inode->start)) {
            buffer_write(sector, disk_inode);
            if (sectors > 0) {
                static char zeros[BLOCK_SECTOR_SIZE];
                size_t i;
              
                for (i = 0; i < sectors; i++) 
                    buffer_write(disk_inode->start + i, zeros);
            }
            success = true; 
        }
        free(disk_inode);
    }
    return success;
}

/*! Reads an inode from SECTOR
    and returns a `struct inode' that contains it.
    Returns a null pointer if memory allocation fails. */
struct inode * inode_open(block_sector_t sector) {
    struct list_elem *e;
    struct inode *inode;

    /* Check whether this inode is already open. */
    for (e = list_begin(&open_inodes); e != list_end(&open_inodes);
         e = list_next(e)) {
        inode = list_entry(e, struct inode, elem);
        if (inode->sector == sector) {
            inode_reopen(inode);
            return inode; 
        }
    }

    /* Allocate memory. */
    inode = malloc(sizeof *inode);
    if (inode == NULL)
        return NULL;

    /* Initialize. */
    list_push_front(&open_inodes, &inode->elem);
    inode->sector = sector;
    inode->open_cnt = 1;
    inode->deny_write_cnt = 0;
    inode->removed = false;
    return inode;
}

/*! Reopens and returns INODE. */
struct inode * inode_reopen(struct inode *inode) {
    if (inode != NULL)
        inode->open_cnt++;
    return inode;
}

/*! Returns INODE's inode number. */
block_sector_t inode_get_inumber(const struct inode *inode) {
    return inode->sector;
}

/*! Closes INODE and writes it to disk.
    If this was the last reference to INODE, frees its memory.
    If INODE was also a removed inode, frees its blocks. */
void inode_close(struct inode *inode) {
    /* Ignore null pointer. */
    if (inode == NULL)
        return;

    /* Release resources if this was the last opener. */
    if (--inode->open_cnt == 0) {
        /* Remove from inode list and release lock. */
        list_remove(&inode->elem);
        
        // Retrive the data from the cache
        struct inode_data data = buffer_read_struct(inode->sector, struct inode_data);
 
        /* Deallocate blocks if removed. */
        if (inode->removed) {
            free_map_release(inode->sector, 1);
            free_map_release(data.start,
                             bytes_to_sectors(data.length));
        }

        free(inode); 
    }
}

/*! Marks INODE to be deleted when it is closed by the last caller who
    has it open. */
void inode_remove(struct inode *inode) {
    ASSERT(inode != NULL);
    inode->removed = true;
}

/*! Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t inode_read_at(struct inode *inode, void *buffer_, off_t size, off_t offset) {
    uint8_t *buffer = buffer_;
    off_t bytes_read = 0;

    while (size > 0) {
        /* Disk sector to read, starting byte offset within sector. */
        block_sector_t sector_idx = byte_to_sector (inode, offset);
        int sector_ofs = offset % BLOCK_SECTOR_SIZE;

        /* Bytes left in inode, bytes left in sector, lesser of the two. */
        off_t inode_left = inode_length(inode) - offset;
        int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
        int min_left = inode_left < sector_left ? inode_left : sector_left;

        /* Number of bytes to actually copy out of this sector. */
        int chunk_size = size < min_left ? size : min_left;
        if (chunk_size <= 0)
            break;

        if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE) {
            /* Read full sector directly into caller's buffer. */
            buffer_read (sector_idx, buffer + bytes_read);
        }
        else {
            /* Read partial sector directly into callers' buffer. */
            buffer_read_bytes(sector_idx, sector_ofs, chunk_size, buffer + bytes_read);
        }
      
        /* Advance. */
        size -= chunk_size;
        offset += chunk_size;
        bytes_read += chunk_size;
    }

    return bytes_read;
}

/*! Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
    Returns the number of bytes actually written, which may be
    less than SIZE if end of file is reached or an error occurs.
    (Normally a write at end of file would extend the inode, but
    growth is not yet implemented.) */
off_t inode_write_at(struct inode *inode, const void *buffer_, off_t size, off_t offset) {
    const uint8_t *buffer = buffer_;
    off_t bytes_written = 0;

    if (inode->deny_write_cnt)
        return 0;

    while (size > 0) {
        /* Sector to write, starting byte offset within sector. */
        block_sector_t sector_idx = byte_to_sector(inode, offset);
        int sector_ofs = offset % BLOCK_SECTOR_SIZE;

        /* Bytes left in inode, bytes left in sector, lesser of the two. */
        off_t inode_left = inode_length(inode) - offset;
        int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
        int min_left = inode_left < sector_left ? inode_left : sector_left;

        /* Number of bytes to actually write into this sector. */
        int chunk_size = size < min_left ? size : min_left;
        if (chunk_size <= 0)
            break;

        if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE) {
            /* Write full sector directly to disk. */
            buffer_write(sector_idx, buffer + bytes_written);
        }
        else {
            /* Write partial sector directly to disk (cache). */
            buffer_write_bytes(sector_idx, sector_ofs, chunk_size, buffer + bytes_written);
        }

        /* Advance. */
        size -= chunk_size;
        offset += chunk_size;
        bytes_written += chunk_size;
    }

    return bytes_written;
}

/*! Disables writes to INODE.
    May be called at most once per inode opener. */
void inode_deny_write (struct inode *inode) {
    inode->deny_write_cnt++;
    ASSERT(inode->deny_write_cnt <= inode->open_cnt);
}

/*! Re-enables writes to INODE.
    Must be called once by each inode opener who has called
    inode_deny_write() on the inode, before closing the inode. */
void inode_allow_write (struct inode *inode) {
    ASSERT(inode->deny_write_cnt > 0);
    ASSERT(inode->deny_write_cnt <= inode->open_cnt);
    inode->deny_write_cnt--;
}

/*! Returns the length, in bytes, of INODE's data. */
off_t inode_length(const struct inode *inode) {
    return buffer_read_member(inode->sector, struct inode_data, length);
}

