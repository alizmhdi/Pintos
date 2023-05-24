#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"

struct bitmap;

void inode_init (void);
bool inode_create (block_sector_t, off_t, bool);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t, off_t);
off_t inode_write_at (struct inode *, const void *, off_t, off_t);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);
struct inode_disk *get_inode_disk (const struct inode *);
bool inode_get_removed (const struct inode *);
struct lock *inode_lock(struct inode *);
bool inode_isdir_disk (struct inode_disk *);
bool inode_isdir (struct inode *);

struct inode_disk *read_inode (const struct inode *);
static bool disk_allocate (struct inode_disk *, off_t);
static bool sector_allocate (block_sector_t *);
static bool indirect_allocate (block_sector_t, size_t);
static bool disk_deallocate (struct inode *);
static bool indirect_deallocate (block_sector_t, size_t);

#endif /* filesys/inode.h */
