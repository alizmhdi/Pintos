#include "cache.h"


void
cache_init (void)
{
  lock_init (&cache_lock);

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
}
