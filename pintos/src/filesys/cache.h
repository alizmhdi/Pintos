#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "off_t.h"
#include "devices/block.h"
#include "lib/kernel/list.h"
#include "threads/synch.h"

#define CACHE_SIZE 64


typedef struct cache_block
  {
    block_sector_t sector_index;
    char data[BLOCK_SECTOR_SIZE];
    bool is_valid;
    bool is_dirty;
    struct list_elem elem; 
    struct lock cache_lock;
  } cache_block_t;


cache_block_t cache_blocks[CACHE_SIZE];

struct list cache_list;

struct lock cache_lock;


void cache_init (void);

void cache_write (struct block *fs_device, block_sector_t sector_idx, void *source, off_t offset, int chunk_size);

void cache_read (struct block *fs_device, block_sector_t sector_idx, void *destination, off_t offset, int chunk_size);

void cache_flush (struct block *fs_device);

#endif /* FILESYS_CACHE_H */
