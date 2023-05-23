/* ASSERT_CONDITIONs growing of hit rate after a file has been accessed.
   To ASSERT_CONDITION this, a file is created with random data, and hit
   rate is calculated twice for accessing. The hit rate should
   grow in the second run. */

#include <syscall.h>
#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"

#define FILE_SIZE 8196
#define BLOCK_SIZE 512

/* from `cache.h` */
#define HIT 1
#define MISS 0

void
test_main (void)
{
  char file_buffer[FILE_SIZE];
  int res;

  res = create ("a", FILE_SIZE);
  ASSERT_CONDITION (res > 0, "failed to create a file called 'a'.");
  msg ("created file named 'a'.");

  int fd = open("a");
  ASSERT_CONDITION (fd > 2, "failed to open the file, expected a `fd` value greater than 2 but got: '%d'.", fd);
  msg ("opened the file.");

  random_init (3853);
  random_bytes (file_buffer, FILE_SIZE - 1);
  file_buffer[FILE_SIZE - 1] = '\0';
  res = write (fd, file_buffer, FILE_SIZE);
  ASSERT_CONDITION (res == FILE_SIZE, "failed to write to file, expected to write '%d' bytes but only got '%d' bytes.", FILE_SIZE, res);
  msg ("wrote random bytes to file.");

  close (fd);
  msg ("closed file.");

  invalidate_cache ();

  fd = open("a");
  ASSERT_CONDITION (fd > 2, "failed to open the file, expected a `fd` value greater than 2 but got: '%d'.", fd);
  msg ("opened the file.");

  for (size_t i = 0; i < FILE_SIZE / BLOCK_SIZE; i++)
    {
      res = read (fd, file_buffer, BLOCK_SIZE);
      ASSERT_CONDITION (res == BLOCK_SIZE, "reading the file must read a block of size '%d', but got '%d'", FILE_SIZE, res);
    }
  msg ("read the file for the first time.");

  close(fd);
  msg ("closed file.");

  int first_hit = cache_stat (HIT);
  int first_miss = cache_stat (MISS);
  msg ("stored hit and miss rate for the first read.");

  fd = open("a");
  ASSERT_CONDITION (fd > 2, "failed to open the file, expected a `fd` value greater than 2 but got: '%d'.", fd);
  msg ("opened the file.");

  for (size_t i = 0; i < FILE_SIZE / BLOCK_SIZE; i++)
    {
      res = read (fd, file_buffer, BLOCK_SIZE);
      ASSERT_CONDITION (res == BLOCK_SIZE, "reading the file must read a block of size '%d', but got '%d'", FILE_SIZE, res);
    }
  msg ("read the file for the second time.");

  close(fd);
  msg ("closed file.");

  int second_hit = cache_stat (HIT);
  int second_miss = cache_stat (MISS);
  msg ("stored hit and miss rate for the second read.");

  
  ASSERT_CONDITION (second_hit * (first_hit + first_miss) > first_hit * (second_hit + second_miss),
          "hit rate must be larger for the second read.");
  msg ("hit rate increased in the second read.");

  res = remove ("a");
  ASSERT_CONDITION (res > 0, "failed to remove the file.");
  msg ("removed the file.");
}