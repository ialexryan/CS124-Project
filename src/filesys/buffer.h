#ifndef FILESYS_BUFFER_H
#define FILESYS_BUFFER_H

#include "devices/block.h"

void buffer_init(void);
void buffer_flush(void);
void buffer_read(block_sector_t sector, void* buffer);
void buffer_write(block_sector_t sector, const void* buffer);
void buffer_read_bytes(block_sector_t sector, off_t sector_ofs, size_t num_bytes, void* buffer);
void buffer_write_bytes(block_sector_t sector, off_t sector_ofs, size_t num_bytes, const void* buffer);

#endif /* filesys/buffer.h */