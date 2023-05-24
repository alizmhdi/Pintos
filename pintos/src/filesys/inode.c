#include "filesys/inode.h"
#include "filesys/cache.h"
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include <debug.h>
#include <list.h>
#include <round.h>
#include <string.h>

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define DIRECT_BLOCK 123
#define INDIRECT_BLOCK 128

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    off_t length;   /* File size in bytes. */
    unsigned magic; /* Magic number. */
    bool is_dir;
    block_sector_t direct[DIRECT_BLOCK]; /* Direct blocks of inode. */
    block_sector_t indirect;             /* Indirect blocks of inode. */
    block_sector_t double_indirect;      /* Double indirect blocks of indoe. */
  };

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* In-memory inode. */
struct inode
  {
    struct list_elem elem; /* Element in inode list. */
    block_sector_t sector; /* Sector number of disk location. */
    int open_cnt;          /* Number of openers. */
    bool removed;          /* True if deleted, false otherwise. */
    int deny_write_cnt;    /* 0: writes ok, >0: deny writes. */
    struct lock f_lock;    /* Synchronization between users of inode. */
  };

struct inode_disk *
read_inode (const struct inode *inode)
{
  struct inode_disk *id = malloc (sizeof *id);
  cache_read (fs_device, inode->sector, (void *) id, 0, BLOCK_SECTOR_SIZE);
  return id;
}

/* Returns the block device sector that contains byte offset POS
   within INODE.
   */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);

  block_sector_t result = -1;
  struct inode_disk *id = read_inode (inode);

  if (pos >= id->length)
    {
      free (id);
      return result;
    }

  off_t block_index = pos / BLOCK_SECTOR_SIZE;
  if (pos < DIRECT_BLOCK * BLOCK_SECTOR_SIZE)
    {
      result = id->direct[block_index];
      free (id);
      return result;
    }

  else if (pos < BLOCK_SECTOR_SIZE * (DIRECT_BLOCK + INDIRECT_BLOCK))
    {
      block_sector_t indirect_blocks[INDIRECT_BLOCK];
      cache_read (fs_device, id->indirect, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
      result = indirect_blocks[block_index - DIRECT_BLOCK];
      free (id);
      return result;
    }
  else
    {
      block_sector_t double_indirect_blocks[INDIRECT_BLOCK];
      cache_read (fs_device, id->double_indirect, &double_indirect_blocks, 0, BLOCK_SECTOR_SIZE);
      off_t indirect_block_index = (block_index - (DIRECT_BLOCK + INDIRECT_BLOCK)) / INDIRECT_BLOCK;
      cache_read (fs_device, double_indirect_blocks[indirect_block_index], &double_indirect_blocks, 0, BLOCK_SECTOR_SIZE);
      result = double_indirect_blocks[(block_index - (DIRECT_BLOCK + INDIRECT_BLOCK)) % INDIRECT_BLOCK];
      free (id);
      return result;
    }
}

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_directory)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      disk_inode->length = length;
      disk_inode->is_dir = is_directory;
      disk_inode->magic = INODE_MAGIC;

      if (disk_allocate (disk_inode, length))
        {
          cache_write (fs_device, sector, disk_inode, 0, BLOCK_SECTOR_SIZE);
          success = true;
        }
      free (disk_inode);
    }
  return success;
}

static bool
sector_allocate (block_sector_t *sector_idx)
{
  static char buffer[BLOCK_SECTOR_SIZE];
  if (!free_map_allocate (1, sector_idx))
    return false;

  cache_write (fs_device, *sector_idx, buffer, 0, BLOCK_SECTOR_SIZE);
  return true;
}

bool
indirect_allocate (block_sector_t sector_idx, size_t number_of_sectors)
{
  block_sector_t indirect_blocks[INDIRECT_BLOCK];
  cache_read (fs_device, sector_idx, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  for (size_t i = 0; i < MIN (INDIRECT_BLOCK, number_of_sectors); i++)
    if (indirect_blocks[i] == 0 && !sector_allocate (&indirect_blocks[i]))
      return false;

  cache_write (fs_device, sector_idx, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  return true;
}

static bool
indirect_deallocate (block_sector_t sector_num, size_t number_of_sectors)
{
  block_sector_t indirect_blocks[INDIRECT_BLOCK];
  cache_read (fs_device, sector_num, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  for (size_t i = 0; i < MIN (INDIRECT_BLOCK, number_of_sectors); i++)
    free_map_release (indirect_blocks[i], 1);

  free_map_release (sector_num, 1);
  return true;
}

static size_t
dbindirect_allocate (struct inode_disk *disk_inode, size_t number_of_sectors)
{
  block_sector_t double_blocks[INDIRECT_BLOCK];
  cache_read (fs_device, disk_inode->double_indirect, &double_blocks, 0, BLOCK_SECTOR_SIZE);

  size_t max_sector = DIV_ROUND_UP (number_of_sectors, INDIRECT_BLOCK);
  size_t chunk;
  for (size_t i = 0; i < max_sector; i++)
    {
      if (number_of_sectors < INDIRECT_BLOCK)
        chunk = number_of_sectors;
      else
        chunk = INDIRECT_BLOCK;
      if (double_blocks[i] == 0 && !sector_allocate (&double_blocks[i]))
        return false;
      if (!indirect_allocate (double_blocks[i], chunk))
        return false;
      number_of_sectors -= chunk;
    }

  cache_write (fs_device, disk_inode->double_indirect, &double_blocks, 0, BLOCK_SECTOR_SIZE);
  return number_of_sectors;
}

static bool
disk_allocate (struct inode_disk *disk_inode, off_t length)
{
  if (length < 0)
    return false;
  size_t number_of_sectors = DIV_ROUND_UP (length, BLOCK_SECTOR_SIZE);
  size_t i;
  for (i = 0; i < MIN (DIRECT_BLOCK, number_of_sectors); i++)
    if (!disk_inode->direct[i] && !sector_allocate (&disk_inode->direct[i]))
      return false;

  number_of_sectors -= i;
  if (number_of_sectors == 0)
    return true;

  if (!disk_inode->indirect && !sector_allocate (&disk_inode->indirect))
    return false;

  if (!indirect_allocate (disk_inode->indirect, number_of_sectors))
    return false;
  if (number_of_sectors > INDIRECT_BLOCK)
    number_of_sectors -= INDIRECT_BLOCK;
  else
    return true;

  if (number_of_sectors > INDIRECT_BLOCK * INDIRECT_BLOCK)
    number_of_sectors = INDIRECT_BLOCK * INDIRECT_BLOCK;

  if (disk_inode->double_indirect == 0 && !sector_allocate (&disk_inode->double_indirect))
    return false;

  number_of_sectors = dbindirect_allocate (disk_inode, number_of_sectors);

  if (number_of_sectors <= INDIRECT_BLOCK * INDIRECT_BLOCK)
    return true;
  return false;
}

static size_t
deallocate_directs (struct inode_disk *disk_inode, size_t number_of_sectors)
{
  size_t i;
  for (i = 0; i < MIN (DIRECT_BLOCK, number_of_sectors); i++)
    free_map_release (disk_inode->direct[i], 1);
  return i;
}

static size_t
deallocate_dbindirects (struct inode_disk *disk_inode, size_t number_of_sectors)
{
  block_sector_t blocks[INDIRECT_BLOCK];
  cache_read (fs_device, disk_inode->double_indirect, &blocks, 0, BLOCK_SECTOR_SIZE);

  if (number_of_sectors > INDIRECT_BLOCK * INDIRECT_BLOCK)
    return false;

  size_t i;
  for (i = 0; i < (size_t) DIV_ROUND_UP (number_of_sectors, INDIRECT_BLOCK); i++)
    {
      block_sector_t double_indirect_blocks[INDIRECT_BLOCK];
      cache_read (fs_device, blocks[i], &double_indirect_blocks, 0, BLOCK_SECTOR_SIZE);
      size_t j;
      for (j = 0; j < MIN (INDIRECT_BLOCK, number_of_sectors); j++)
        free_map_release (double_indirect_blocks[j], 1);
      free_map_release (blocks[i], 1);
      number_of_sectors -= j;
    }

  free_map_release (disk_inode->double_indirect, 1);
  free (disk_inode);
  return true;
}

static bool
disk_deallocate (struct inode *inode)
{
  struct inode_disk *disk_inode = read_inode (inode);

  if (disk_inode == NULL)
    return true;

  size_t number_of_sectors = DIV_ROUND_UP (disk_inode->length, BLOCK_SECTOR_SIZE);

  number_of_sectors -= deallocate_directs (disk_inode, number_of_sectors);
  if (number_of_sectors == 0)
    {
      free (disk_inode);
      return true;
    }

  if (!indirect_deallocate (disk_inode->indirect, number_of_sectors))
    return false;

  if (number_of_sectors < INDIRECT_BLOCK)
    number_of_sectors -= number_of_sectors;
  else
    number_of_sectors -= INDIRECT_BLOCK;

  if (number_of_sectors == 0)
    {
      free (disk_inode);
      return true;
    }

  return deallocate_dbindirects (disk_inode, number_of_sectors);
}

struct lock *
inode_lock (struct inode *inode)
{
  ASSERT (inode);
  return &(inode->f_lock);
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  lock_init (&inode->f_lock);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Returns INODE's remove status. */
bool
inode_get_removed (const struct inode *inode)
{
  return inode->removed;
}

bool
inode_isdir_disk (struct inode_disk *inode_disk)
{
  return inode_disk->is_dir;
}

bool
inode_isdir (struct inode *inode)
{
  if (inode == NULL)
    return false;
  struct inode_disk *inode_disk = read_inode (inode);
  if (inode_disk == NULL)
    return false;
  bool res = inode_isdir_disk (inode_disk);
  free (inode_disk);
  return res;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      if (inode->removed)
        {
          free_map_release (inode->sector, 1);
          disk_deallocate (inode);
        }

      free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  lock_acquire (&inode->f_lock);

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      cache_read (fs_device, sector_idx, (void *) (buffer + bytes_read), sector_ofs, chunk_size);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }

  lock_release (&inode->f_lock);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  if (inode->deny_write_cnt)
    return 0;

  lock_acquire (&inode->f_lock);

  if (byte_to_sector (inode, offset + size - 1) == (size_t) -1)
    {
      struct inode_disk *id = read_inode (inode);
      if (!disk_allocate (id, offset + size))
        {
          free (id);
          return bytes_written;
        }

      id->length = size + offset;
      cache_write (fs_device, inode_get_inumber (inode), (void *) id, 0, BLOCK_SECTOR_SIZE);
      free (id);
    }
  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      cache_write (fs_device, sector_idx, (void *) (buffer + bytes_written),
                   sector_ofs, chunk_size);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }

  lock_release (&inode->f_lock);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  struct inode_disk *id = read_inode (inode);
  off_t length = id->length;
  free (id);
  return length;
}