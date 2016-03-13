#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <list.h>
#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"

#define NUM_DIRECT 12
#define NUM_INDIRECT 1
#define NUM_DOUBLE_INDIRECT 1
#define NUM_ENTRIES (NUM_DIRECT + NUM_INDIRECT + NUM_DOUBLE_INDIRECT)

struct bitmap;

#define _INODE_DATA \
    block_sector_t start;               /*!< First data sector. */\
    off_t length;                       /*!< File size in bytes. */\
    unsigned magic;                     /*!< Magic number. */\
    block_sector_t direct[NUM_DIRECT];\
    block_sector_t indirect[NUM_INDIRECT];\
    block_sector_t double_indirect[NUM_DOUBLE_INDIRECT];\

// The informational disk-stored data associated with a given inode.
struct inode_data { _INODE_DATA };

/*! On-disk inode.
	Must be exactly BLOCK_SECTOR_SIZE bytes long. */
// The actual disk-stored struct that includes the padding.
struct inode_disk {
    // Anonymously embed all the data members in this struct.
    struct { _INODE_DATA };
    
    /*!< Not used, except to pad inode_disk to BLOCK_SECTOR_SIZE */
    char unused[BLOCK_SECTOR_SIZE - sizeof(struct { _INODE_DATA })];
};
/*! In-memory inode. */
struct inode {
    struct list_elem elem;              /*!< Element in inode list. */
    block_sector_t sector;              /*!< Sector number of disk location. */
    int open_cnt;                       /*!< Number of openers. */
    bool removed;                       /*!< True if deleted, false otherwise. */
    int deny_write_cnt;                 /*!< 0: writes ok, >0: deny writes. */
};


void inode_init(void);
bool inode_create(block_sector_t, off_t);
struct inode *inode_open(block_sector_t);
struct inode *inode_reopen(struct inode *);
block_sector_t inode_get_inumber(const struct inode *);
void inode_close(struct inode *);
void inode_remove(struct inode *);
off_t inode_read_at(struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at(struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write(struct inode *);
void inode_allow_write(struct inode *);
off_t inode_length(const struct inode *);

#endif /* filesys/inode.h */
