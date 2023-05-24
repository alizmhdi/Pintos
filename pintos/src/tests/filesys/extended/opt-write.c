/* The third test from the project document.
    Fills a 100kB file with random bytes. Then rewrites the file.
    As whole cache blocks are being overwritten, if the optimization is done correctly,
    the block_read() function shouldn't be called too many times. */
   
#include <random.h>
#include <stdio.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"
#include "threads/fixed-point.h"

#define BLOCK_SECTOR_SIZE 512
#define BUF_SIZE (BLOCK_SECTOR_SIZE * 200)

/* From cache.h */
#define READ 2
#define WRITE 3

static char buf[BUF_SIZE];
static long long num_disk_reads;
static long long num_disk_writes;

void
test_main (void)
{
  int test_fd;
  char *file_name = "a";
  CHECK (create (file_name, 0), 
    "create \"%s\"", file_name);
  CHECK ((test_fd = open (file_name)) > 1, 
    "open \"%s\"", file_name);

  /* Write file of size BLOCK_SECTOR_SIZE * 200 */
  random_bytes (buf, sizeof buf);
  CHECK (write (test_fd, buf, sizeof buf) == BUF_SIZE,
   "write %d bytes to \"%s\"", (int) BUF_SIZE, file_name);

  /* Invalidate cache */
  invalidate_cache ();
  msg ("invalidate cache");

  /* Save baseline disk stats for comparison */
  long long base_disk_reads = cache_stat (READ);
  long long base_disk_writes = cache_stat (WRITE);

  /* Write file of size BLOCK_SECTOR_SIZE * 200 */
  random_bytes (buf, sizeof buf);
  CHECK (write (test_fd, buf, sizeof buf) == BUF_SIZE,
   "write %d bytes to \"%s\"", (int) BUF_SIZE, file_name);

  /* Save new disk stats for comparison */
  long long new_disk_reads = cache_stat (READ);
  long long new_disk_writes = cache_stat (WRITE);

  /* Check that writes are much more than reads. */
  CHECK (num_disk_writes >= 5 * num_disk_reads, 
    "old reads: %lld, old writes: %lld, new reads: %lld, new writes: %lld", 
  	base_disk_reads, base_disk_writes, new_disk_reads, new_disk_writes);

  msg ("close \"%s\"", file_name);
  close (test_fd);

  /* Remove the file now that we are done.*/
  remove (file_name);

}
