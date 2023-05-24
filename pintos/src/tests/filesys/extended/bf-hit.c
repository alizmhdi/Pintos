/* The first test from the project document. 
  We first write some data to a file, and then invalidate cache.
  Then we read the data from the file. Since the cache is cold, all
  the blocks need to be reread. The next time we read the data, we don't invalidate the cache.
  We can see a marked improvement in the hitrate. */

#include <random.h>
#include <stdio.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define CACHE_NUM_ENTRIES 64
#define BLOCK_SECTOR_SIZE 512
#define BUF_SIZE (BLOCK_SECTOR_SIZE * CACHE_NUM_ENTRIES / 2)

#define HIT 1
#define MISS 0

static char buf[BUF_SIZE];

void
test_main (void)
{
  int test_fd;
  char *file_name = "a";
  CHECK (create (file_name, 0),
    "create \"%s\"", file_name);
  CHECK ((test_fd = open (file_name)) > 1,
    "open \"%s\"", file_name);

  /* Write file of size CACHE_NUM_ENTRIES * BLOCK_SECTOR_SIZE
     to fill half of cold cache. The rest of the cache is left empty for the kernel to use. */
  random_bytes (buf, sizeof buf);
  CHECK (write (test_fd, buf, sizeof buf) == BUF_SIZE,
   "write %d bytes to \"%s\"", (int) BUF_SIZE, file_name);

  /* Close file and reopen */
  close (test_fd);
  msg ("close \"%s\"", file_name);
  CHECK ((test_fd = open (file_name)) > 1,
    "open \"%s\"", file_name);

  /* Save baseline cache stats for comparison */
  long long base_cache_misses = cache_stat (MISS);
  long long base_cache_hits = cache_stat (HIT);
  msg ("get baseline cache stats");

  /* Invalidate cache */
  invalidate_cache ();
  msg ("invalidate cache");

  /* Read full file (cold cache) */
  CHECK (read (test_fd, buf, sizeof buf) == BUF_SIZE,
  "read %d bytes from \"%s\"", (int) BUF_SIZE, file_name);

  /* Save cold cache stats for comparison */
  long long cold_cache_misses = cache_stat (MISS) - base_cache_misses;
  long long cold_cache_hits = cache_stat (HIT) - base_cache_hits;
  msg ("get cold cache stats");

  /* Close file and reopen */
  close (test_fd);
  msg ("close \"%s\"", file_name);
  CHECK ((test_fd = open (file_name)) > 1,
    "open \"%s\"", file_name);

  /* Read full file again */
  CHECK (read (test_fd, buf, sizeof buf) == BUF_SIZE,
   "read %d bytes from \"%s\"", (int) BUF_SIZE, file_name);

  /* Save new cache stats for comparison */
  long long new_cache_misses = cache_stat (MISS) - cold_cache_misses;
  long long new_cache_hits = cache_stat (HIT) - cold_cache_hits;
  msg ("get new cache stats");

  /* Convert to percent for display */
  int old_rate_int = (int) (100 * cold_cache_hits) / (cold_cache_hits + cold_cache_misses);
  int new_rate_int = (int) (100 * new_cache_hits) / (new_cache_hits + new_cache_misses);

  /* Check that hit rate improved */
  CHECK (new_rate_int > old_rate_int,
    "old hit rate percent: %d, new hit rate percent: %d",
    old_rate_int, new_rate_int);

  msg ("close \"%s\"", file_name);
  close (test_fd);

  /* Remove the file now that we are done.*/
  remove (file_name);
  
}