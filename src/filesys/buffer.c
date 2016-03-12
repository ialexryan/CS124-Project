#include "filesys/buffer.h"
#include "filesys/filesys.h"

void buffer_read(block_sector_t sector, void* buffer) {


	block_read(fs_device, sector, buffer);
}

void buffer_write(block_sector_t sector, const void* buffer) {



	block_write(fs_device, sector, buffer);
}