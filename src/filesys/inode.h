#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <list.h>
#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include "threads/synch.h"

struct bitmap;

struct indirect_sector_entry {
    // True if the sector has been loaded.
    bool loaded : 1;
    
    // Invalid unless loaded is true.
    // We could've used a specific sector value to represent the unloaded state,
    // but this was less bug prone through also less efficient.
    block_sector_t sector;
};

// The names levels of indirection coupled with the number
// of sectors for each level of indirection.
#define _INDIRECTION \
    LEVEL(DIRECT, 12) \
    LEVEL(INDIRECT, 1) \
    LEVEL(DOUBLE_INDIRECT, 1) \
    LEVEL(TRIPLE_INDIRECT, 1)

#define LEVEL(_, COUNT) COUNT +
#define TOTAL_INDIRECTION (_INDIRECTION 0)

static const size_t total_num_inode_root_sectors = TOTAL_INDIRECTION;
typedef struct indirect_sector_entry root_sector_entries[TOTAL_INDIRECTION];

#undef TOTAL_INDIRECTION
#undef LEVEL

// The data (non-padding) potion of the inode_disk. Defined in a macro
// so it can be easily embedded as an anonymous struct while still easily
// supporting sizeof.
#define _INODE_DATA \
    /* NOTE THAT THE SECTOR ENTRIES ARE ASSUMED TO BE FIRST! */\
    /* SO THAT INODE INDIRECTION SECTORS CAN BE TREATED THE SAME AS THE HEADER */\
    root_sector_entries sectors;\
    off_t length;                       /*!< File size in bytes. */\
    unsigned magic;                     /*!< Magic number. */\
    bool is_directory;                  /*!< True if directory, false if file. */\

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
    struct lock extend_lock;                 /*!< Lock that must be acquired to extend. */
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
