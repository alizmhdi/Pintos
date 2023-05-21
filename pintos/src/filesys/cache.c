#include <string.h>
#include "cache.h"
#include "filesys.h"

/* Defines used for updating cache stats. */
#define hit 1
#define miss 0
#define access -1

static void
stat_update (int mode)
{
  lock_acquire (&stat_lock);
  switch (mode)
  {
  case hit:
    hit_count++;
    break;
  
  case miss:
    miss_count++;
    break;

  case access:
    access_count++;
    break;
  }
  lock_release (&stat_lock);
}

/* Function used to initialize the cache. */
void
cache_init (void)
{
  /* Initialize the locks. */
  lock_init (&cache_lock);
  lock_init (&stat_lock);
  
  /* Initialize the blocks. */
  lock_acquire (&cache_lock);
  list_init (&cache_list);
  for (int i = 0; i < CACHE_SIZE; i++)
    {
      lock_init (&cache_blocks[i].block_lock);
      cache_blocks[i].is_dirty = false;
      cache_blocks[i].is_valid = false;
      list_push_back (&cache_list, &cache_blocks[i].elem);
    }

  lock_release (&cache_lock);

  /* Initialize the stats. */
  lock_acquire (&stat_lock);
  miss_count = 0;
  hit_count = 0;
  access_count = 0;
  lock_release (&stat_lock);

  cache_initialized = 1;
}

/* Write back the cache block if is_dirty, then set is_valid and is_dirty. */
void 
cache_evict (struct block *fs_device, struct cache_block *cache_block)
{
  ASSERT (lock_held_by_current_thread (&cache_block->block_lock));
  if (cache_block->is_valid && cache_block->is_dirty)
  {
    block_write (fs_device, cache_block->sector_index, cache_block->data);
  }

  cache_block->is_dirty = 0;
  cache_block->is_valid = 0;
}

/* Find cache block with current sector index, acquire the lock and return the block.
  If no blocks found, evict least recently used block from cache list, acquire its lock and return the block.*/
static struct cache_block *
get_cache_block (struct block *fs_device, block_sector_t sector_idx)
{
  stat_update (access);

  for (int i = 0; i < CACHE_SIZE; i++)
  {
    lock_acquire (&cache_blocks[i].block_lock);
    /* Cache hit case*/
    if (cache_blocks[i].sector_index == sector_idx && cache_blocks[i].is_valid)
    {
      lock_acquire (&cache_lock);
      list_remove (&cache_blocks[i].elem);
      list_push_back (&cache_list, &cache_blocks[i].elem);
      lock_release (&cache_lock);

      stat_update (hit);

      return &cache_blocks[i];
    }
    lock_release (&cache_blocks[i].block_lock);
  }

  /* Cache miss case*/
  stat_update (miss);
  lock_acquire (&cache_lock);
  struct cache_block *lru_block = list_entry(list_pop_front (&cache_list), struct cache_block, elem);
  lock_acquire (&lru_block->block_lock);

  cache_evict (fs_device, lru_block);
  ASSERT (!lru_block->is_dirty && !lru_block->is_valid);
  block_read (fs_device, sector_idx, lru_block->data);
  lru_block->sector_index = sector_idx;
  lru_block->is_valid = 1;
  list_push_back (&cache_list, &lru_block->elem);
  lock_release (&cache_lock);

  return &lru_block;
}

void
cache_read (struct block *fs_device, block_sector_t sector_idx, void *destination, off_t offset, int chunk_size)
{
  ASSERT (fs_device != NULL);
  ASSERT (offset + chunk_size <= BLOCK_SECTOR_SIZE);

  struct cache_block *cache_block =  get_cache_block (fs_device, sector_idx);
  ASSERT (lock_held_by_current_thread (&cache_block->block_lock));
  ASSERT (cache_block->is_valid);

  memcpy (destination, cache_block->data + offset, chunk_size);
  lock_release (&cache_block->block_lock);
}
