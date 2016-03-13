#ifndef FILESYS_BUFFER_H
#define FILESYS_BUFFER_H

#include "devices/block.h"
#include "filesys/off_t.h"

// The type of some member of a struct type.
#define memberof(STRUCT, MEMBER) (((STRUCT *)NULL)->MEMBER)

void buffer_init(void);
void buffer_flush(void);
void buffer_read(block_sector_t sector, void* buffer);
void buffer_write(block_sector_t sector, const void* buffer);
void buffer_read_bytes(block_sector_t sector, off_t sector_ofs, size_t num_bytes, void* buffer);
void buffer_write_bytes(block_sector_t sector, off_t sector_ofs, size_t num_bytes, const void* buffer);

// Evaluates to the data from the given sector as interpreted
// as the specified struct type.
#define buffer_read_struct(SECTOR, STRUCT) \
    ({ \
        typeof(STRUCT) temp; \
        buffer_read_bytes(SECTOR, 0, sizeof(STRUCT), (void *)&temp); \
        temp; \
    })

// Evaluates to the specified member of the struct if the data from
// the given sector is interpreted as the type of that struct.
#define buffer_read_member(SECTOR, STRUCT, MEMBER) \
    ({ \
        typeof(memberof(STRUCT, MEMBER)) temp; \
        buffer_read_bytes(SECTOR, offsetof(STRUCT, MEMBER), \
            sizeof(memberof(STRUCT, MEMBER)), (void *)&temp); \
        temp; \
    })

// Write to the buffer the given data member of the struct.
#define buffer_write_member(SECTOR, STRUCT, MEMBER, DATA) \
    ({ \
        typeof(memberof(STRUCT, MEMBER)) temp = DATA; \
        buffer_write_bytes(SECTOR, offsetof(STRUCT, MEMBER), \
            sizeof(memberof(STRUCT, MEMBER)), (const void *)&temp); \
        ; \
    })

#endif /* filesys/buffer.h */