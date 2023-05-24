#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "off_t.h"
#include "devices/block.h"
#include "lib/kernel/list.h"
#include "threads/synch.h"

#define CACHE_SIZE 64

/* Defines used for updating cache stats. */
#define HIT 1
#define MISS 0
#define READ 2
#define WRITE 3


typedef struct cache_block
  {
    block_sector_t sector_index;
    char data[BLOCK_SECTOR_SIZE];
    bool is_valid;
    bool is_dirty;
    struct list_elem elem; 
    struct lock block_lock;
  } cache_block_t;

cache_block_t cache_blocks[CACHE_SIZE];
struct list cache_list;
struct lock cache_lock;

bool cache_initialized;

long long hit_count;
long long miss_count;
long long read_count;
long long write_count;
struct lock stat_lock;

void cache_init (void);

void cache_write (struct block *fs_device, block_sector_t sector_idx, void *source, off_t offset, int chunk_size);

void cache_read (struct block *fs_device, block_sector_t sector_idx, void *destination, off_t offset, int chunk_size);

void flush_block (struct block *fs_device, struct cache_block *cache_block);

void cache_flush (struct block *fs_device);

void cache_shutdown (struct block *fs_device);

size_t cache_count (int mode);

void cache_invalidate (struct block *fs_device);

#endif
