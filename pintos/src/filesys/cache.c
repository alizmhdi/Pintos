#include <debug.h>
#include <string.h>
#include "cache.h"
#include "filesys/filesys.h"

static void
stat_update (int mode)
{
  lock_acquire (&stat_lock);
  switch (mode)
  {
  case HIT:
    hit_count++;
    break;
  
  case MISS:
    miss_count++;
    break;
  }
  lock_release (&stat_lock);
}

/* Function used to initialize the cache. */
void 
cache_init(void)
{
  /* Initialize the locks. */
  lock_init(&cache_lock);
  lock_init (&stat_lock);

  /* Initialize the blocks. */
  lock_acquire(&cache_lock);
  list_init(&cache_list);
  for (int i = 0; i < CACHE_SIZE; i++)
  {
    lock_init(&cache_blocks[i].block_lock);
    cache_blocks[i].is_dirty = false;
    cache_blocks[i].is_valid = false;
    list_push_back(&cache_list, &cache_blocks[i].elem);
  }

  lock_release(&cache_lock);

  /* Initialize the stats. */
  lock_acquire (&stat_lock);
  miss_count = 0;
  hit_count = 0;
  access_count = 0;
  lock_release (&stat_lock);

  cache_initialized = true;
}

/* Find cache block with current sector index, acquire the lock and return the block.
  If no blocks found, evict least recently used block from cache list, acquire its lock and return the block.*/
struct cache_block *
get_cache_block(struct block *fs_device, block_sector_t sector_idx)
{
  lock_acquire(&cache_lock);

  for (int i = 0; i < CACHE_SIZE; i++)
  {
    lock_acquire (&cache_blocks[i].block_lock);

    /* Cache hit case*/
    if (cache_blocks[i].is_valid && cache_blocks[i].sector_index == sector_idx)
    {
      list_remove(&cache_blocks[i].elem);
      list_push_back(&cache_list, &cache_blocks[i].elem);

      lock_release(&cache_lock);
      stat_update(HIT);
      return &cache_blocks[i];
    }
    
    lock_release (&cache_blocks[i].block_lock);
  }

  /* Cache miss case*/
  struct cache_block *lru_block = list_entry(list_pop_front(&cache_list), struct cache_block, elem);
  lock_acquire(&lru_block->block_lock);

  flush_block(fs_device, lru_block);
  block_read(fs_device, sector_idx, lru_block->data);
  lru_block->sector_index = sector_idx;
  lru_block->is_valid = true;
  list_push_back(&cache_list, &lru_block->elem);

  lock_release(&cache_lock);
  stat_update(MISS);
  return lru_block;
}

void 
cache_write(struct block *fs_device, block_sector_t sector_idx, void *source, off_t offset, int chunk_size)
{
  ASSERT(cache_initialized);
  ASSERT(fs_device != NULL);
  ASSERT(offset + chunk_size <= BLOCK_SECTOR_SIZE);

  struct cache_block *cache_block = get_cache_block(fs_device, sector_idx);
  ASSERT(lock_held_by_current_thread(&cache_block->block_lock));
  ASSERT(cache_block->is_valid);
  memcpy(cache_block->data + offset, source, chunk_size);
  cache_block->is_dirty = true;
  lock_release(&cache_block->block_lock);
}

void 
cache_read(struct block *fs_device, block_sector_t sector_idx, void *destination, off_t offset, int chunk_size)
{
  ASSERT(cache_initialized);
  ASSERT(fs_device != NULL);
  ASSERT(offset + chunk_size <= BLOCK_SECTOR_SIZE);

  struct cache_block *cache_block = get_cache_block(fs_device, sector_idx);
  ASSERT(lock_held_by_current_thread(&cache_block->block_lock));
  ASSERT(cache_block->is_valid);

  memcpy(destination, cache_block->data + offset, chunk_size);
  lock_release(&cache_block->block_lock);
}

void 
flush_block(struct block *fs_device, struct cache_block *cache_block)
{
  if (cache_block->is_valid && cache_block->is_dirty)
  {
    block_write(fs_device, cache_block->sector_index, cache_block->data);
  }

  cache_block->is_dirty = false;
}

/* (Not yet) used for write-behind functionality. */
void 
cache_flush(struct block *fs_device)
{
  for (int i = 0; i < CACHE_SIZE; i++)
  {
    flush_block (fs_device, &cache_blocks[i]);
  }
}

/* Flush and invalidate all cache blocks.*/
void 
cache_shutdown(struct block *fs_device)
{
  for (int i = 0; i < CACHE_SIZE; i++)
  {
    flush_block (fs_device, &cache_blocks[i]);
    cache_blocks[i].is_valid = false;
  }
}

/* To be used in cache stat syscalls. */
size_t
cache_count (int mode)
{
  lock_acquire(&stat_lock);
  switch (mode)
  {
  case HIT:
    return hit_count;
    break;
  
  case MISS:
    return miss_count;
    break;
  }
  lock_release(&stat_lock);
}

/* Invalidate all cache blocks. */
void 
cache_invalidate(void)
{
  lock_acquire (&cache_lock);
  
  for (int i = 0; i < CACHE_SIZE; i++)
    {
      cache_blocks[i].is_valid = false;
    }
  
  lock_release (&cache_lock);
}
