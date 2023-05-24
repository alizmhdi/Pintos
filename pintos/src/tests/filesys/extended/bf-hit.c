#include <random.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BUF_SIZE 16384

#define HIT 1
#define MISS 0

static char buf[BUF_SIZE];

void
test_main (void)
{
  int fd;
  
  invalidate_cache ();
  msg ("invalidate cache");
 
  CHECK (create ("a", BUF_SIZE), "create \"a\"");
  CHECK ((fd = open ("a")) > 1, "open \"a\"");

  random_init (2020);
  random_bytes (buf, sizeof(buf));
  CHECK (write (fd, buf, sizeof(buf)) == BUF_SIZE, "write to \"a\"");

  close (fd);
  msg ("close \"a\"");

  CHECK ((fd = open ("a")) > 1, "open \"a\"");

  CHECK (read (fd, buf, sizeof(buf)) == BUF_SIZE, "read from \"a\"");

  int first_hit = cache_stat (HIT);
  int first_miss = cache_stat (MISS);

  close (fd);
  msg ("close \"a\"");

  CHECK ((fd = open ("a")) > 1, "open \"a\"");

  CHECK (read (fd, buf, sizeof(buf)) == BUF_SIZE, "read from \"a\"");

  int second_hit = cache_stat (HIT);
  int second_miss = cache_stat (MISS);

  CHECK (second_hit * (first_hit + first_miss) > first_hit * (second_hit + second_miss), "hit rate must increase");

  msg ("close \"a\"");
  close (fd);
}
