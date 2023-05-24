#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include <list.h>
#include <stdio.h>
#include <string.h>

/* A directory. */
struct dir
  {
    struct inode *inode; /* Backing store. */
    off_t pos;           /* Current position. */
  };

/* A single directory entry. */
struct dir_entry
  {
    block_sector_t inode_sector; /* Sector number of header. */
    char name[NAME_MAX + 1];     /* Null terminated file name. */
    bool in_use;                 /* In use or free? */
  };

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
  if (!inode_create (sector, entry_cnt * sizeof (struct dir_entry), true))
    return false;

  struct inode *inode_dir = inode_open (sector);

  struct dir_entry entry;
  entry.inode_sector = sector;
  entry.name[0] = '.';
  entry.name[1] = '.';
  entry.name[2] = '\0';
  entry.in_use = true;

  bool succeeded = (sizeof entry == inode_write_at (inode_dir, &entry, sizeof entry, 0));

  inode_close (inode_dir);

  return succeeded;
}

static int
get_next_part (char part[NAME_MAX + 1], char **srcp)
{
  const char *src = *srcp;
  char *dst = part;

  while (*src == '/')
    src++;
  if (*src == '\0')
    return 0;

  while (*src != '/' && *src != '\0')
    {
      if (dst < part + NAME_MAX)
        *dst++ = *src;
      else
        return -1;
      src++;
    }
  *dst = '\0';

  *srcp = src;
  return 1;
}

bool
split_path_dir (char *path, char *last, struct dir **par)
{
  if (path[0] == '/')
    *par = dir_open_root ();
  else
    {
      if (thread_current ()->work_dir == NULL)
        *par = dir_open_root ();
      else
        {
          if (inode_get_removed (thread_current ()->work_dir->inode))
            {
              *par = NULL;
              *last = '\0';
              return false;
            }
          *par = dir_reopen (thread_current ()->work_dir);
        }
    }
  *last = '\0';
  char *current_path = path;
  for (;;)
    {
      bool lookup_result = true;
      struct inode *inode_next = NULL;
      if (last[0] != '\0')
        lookup_result = dir_lookup (*par, last, &inode_next);

      int next_part = get_next_part (last, &current_path);
      if (next_part == 0)
        {
          inode_close (inode_next);
          break;
        }
      else if (next_part < 0)
        {
          *par = NULL;
          *last = '\0';
          return false;
        }

      if (!lookup_result)
        {
          *par = NULL;
          *last = '\0';
          return false;
        }

      if (inode_next)
        {
          if (inode_get_removed (inode_next))
            {
              *par = NULL;
              *last = '\0';
              return false;
            }

          struct dir *dir_next = dir_open (inode_next);
          if (dir_next == NULL)
            {
              *par = NULL;
              *last = '\0';
              return false;
            }
          dir_close (*par);
          *par = dir_next;
        }
    }
  return true;
}

struct dir *
open_dir_path (char *path)
{
  if (path == NULL)
    return NULL;

  if (path[0] == '/' && path[1] == '\0')
    return dir_open_root ();

  struct dir *par;
  struct inode *dir_inode;
  char last[NAME_MAX + 1];
  split_path_dir (path, last, &par);

  if (!dir_lookup (par, last, &dir_inode))
    {
      dir_close (par);
      return NULL;
    }
  if (inode_get_removed (dir_inode))
    {
      inode_close (dir_inode);
      dir_close (par);
      return NULL;
    }

  return dir_open (dir_inode);
}

struct dir *
convert_file_to_dir (struct file *file)
{
  if (file == NULL)
    return NULL;

  struct inode *inode_file = file_get_inode (file);

  return ((inode_file == NULL || !inode_isdir (inode_file)) ? NULL : ((struct dir *) file));
}

/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode)
{
  struct dir *dir = calloc (1, sizeof *dir);
  if (inode != NULL && dir != NULL)
    {
      dir->inode = inode;
      dir->pos = sizeof (struct dir_entry);
      ;
      return dir;
    }
  else
    {
      inode_close (inode);
      free (dir);
      return NULL;
    }
}

/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir)
{
  ASSERT (NULL != dir);
  return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir)
{
  if (dir != NULL)
    {
      inode_close (dir->inode);
      free (dir);
    }
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir)
{
  ASSERT (NULL != dir);
  return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp)
{
  struct dir_entry e;
  size_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  int start = sizeof (struct dir_entry);
  for (ofs = start; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e)
    if (e.in_use && !strcmp (name, e.name))
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode)
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (name[0] == '.' && name[1] == '\0')
    *inode = inode_reopen (dir->inode);
  else if (name[0] == '.' && name[1] == '.' && name[2] == '\0')
    {
      inode_read_at (dir->inode, &e, sizeof e, 0);
      *inode = inode_open (e.inode_sector);
    }
  else if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
    *inode = NULL;
  if (*inode == NULL)
    return false;
  return true;
}

bool
dir_add_dir (struct dir *dir, struct inode *inode_dir)
{
  struct dir_entry child;
  child.in_use = false;
  child.name[0] = '.';
  child.name[1] = '.';
  child.name[2] = '\0';
  struct inode *dir_inode = dir_get_inode (dir);
  child.inode_sector = inode_get_inumber (dir_inode);
  int amount_written = inode_write_at (inode_dir, &child, sizeof child, 0);
  if (amount_written != sizeof (child))
    {
      inode_close (inode_dir);
      return false;
    }
  return true;
}
/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector)
{

  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  struct inode *inode_dir = inode_open (inode_sector);
  if (inode_dir == NULL)
    return false;

  bool is_dir = inode_isdir_disk (read_inode (inode_dir));
  if (is_dir)
    {
      if (!dir_add_dir (dir, inode_dir))
        return false;
    }
  inode_close (inode_dir);
  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.

     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  int start = sizeof (struct dir_entry);
  for (ofs = start; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e)
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

done:
  return success;
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name)
{

  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dir, name, &e, &ofs))
    goto done;

  /* Open inode. */
  inode = inode_open (e.inode_sector);
  if (inode == NULL)
    goto done;

  bool is_dir = inode_isdir (inode);
  if (is_dir)
    {
      struct dir *cur_dir = dir_open (inode);
      struct dir_entry ent;

      off_t offst;

      bool has_child = false;
      for (offst = sizeof ent; inode_read_at (cur_dir->inode, &ent, sizeof ent, offst) == sizeof ent;
           offst += sizeof ent)
        {
          if (ent.in_use)
            {
              has_child = true;
              break;
            }
        }
      dir_close (cur_dir);

      if (has_child)
        goto done;
    }

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e)
    goto done;

  /* Remove inode. */
  inode_remove (inode);
  success = true;

done:
  inode_close (inode);
  return success;
}

/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{

  struct dir_entry e;

  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e)
    {
      dir->pos += sizeof e;
      if (e.in_use)
        {
          strlcpy (name, e.name, NAME_MAX + 1);
          return true;
        }
    }
  dir->pos = sizeof (struct dir_entry);
  return false;
}
