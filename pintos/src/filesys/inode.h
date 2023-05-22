#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"

struct bitmap;

void inode_init (void);
bool inode_create (block_sector_t, off_t);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);
struct inode_disk *get_inode_disk (const struct inode *);
bool inode_get_removed (const struct inode *);
struct lock *inode_lock(struct inode *);

struct inode_disk *read_inode (const struct inode *);
static bool disk_allocate (struct inode_disk *disk_inode, off_t length);
static bool sector_allocate (block_sector_t *sector_idx);
static bool indirect_allocate (block_sector_t sector_idx, size_t number_of_sectors);
static bool disk_deallocate (struct inode *inode);
static bool indirect_deallocate (block_sector_t sector_num, size_t number_of_sectors);

#endif /* filesys/inode.h */
