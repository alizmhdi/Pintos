#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "devices/block.h"
#include "threads/synch.h"
#include "lib/kernel/list.h"

#define CACHE_SIZE 64


typedef struct cache_block
{
  block_sector_t sector_index;
  char data[BLOCK_SECTOR_SIZE];
  bool valid;
  bool dirty;
  struct list_elem elem; 
  struct lock block_lock;
} cache_block_t;


cache_block_t cache_blocks[CACHE_SIZE];

struct list cache_list;

struct lock cache_lock;

#endif /* FILESYS_CACHE_H */
