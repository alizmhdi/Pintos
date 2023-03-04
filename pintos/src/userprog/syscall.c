#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "process.h"

static void syscall_handler(struct intr_frame *);

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_practice(struct intr_frame *, uint32_t *);
static void syscall_exit(struct intr_frame *, int);
static void syscall_write(struct intr_frame *, uint32_t *, struct thread *);
static void syscall_create(struct intr_frame *, uint32_t *);
static void syscall_remove(struct intr_frame *, uint32_t *);
static void syscall_open(struct intr_frame *, uint32_t *, struct thread *);
static void syscall_close(struct intr_frame *, uint32_t *, struct thread *);
static void syscall_filesize(struct intr_frame *, uint32_t *, struct thread *);
static void syscall_read(struct intr_frame *, uint32_t *, struct thread *);
static void syscall_seek(struct intr_frame *, uint32_t *, struct thread *);
static void syscall_tell(struct intr_frame *, uint32_t *, struct thread *);
static void syscall_exec(struct intr_frame *, uint32_t *, struct thread *);
static void syscall_wait(struct intr_frame *, uint32_t *, struct thread *);

static bool
check_fd(struct thread *t, int fd)
{
  if (fd == NULL || fd > MAX_OPEN_FILE || fd < 0 || t->file_descriptors[fd] == NULL)
    return false;
  return true;
}

static bool
check_address(const void *vaddr)
{
  if (vaddr != NULL && is_user_vaddr(vaddr) && pagedir_get_page(thread_current()->pagedir, vaddr) != NULL)
    return true;
  return false;
}


static void
syscall_handler(struct intr_frame *f UNUSED)
{
  uint32_t *args = ((uint32_t *)f->esp);

  if (!check_address(args) || !check_address(args + 4))
    syscall_exit(f, -1);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

  struct thread *current_thread = thread_current();

  switch (args[0])
  {
  case SYS_PRACTICE:
    syscall_practice(f, args);
    break;
  case SYS_EXIT:
    if(!check_address(args + 1))
      syscall_exit(f, -1);
    syscall_exit(f, args[1]);
    break;
  case SYS_WRITE:
    syscall_write(f, args, current_thread);
    break;
  case SYS_CREATE:
    syscall_create(f, args);
    break;
  case SYS_REMOVE:
    syscall_remove(f, args);
    break;
  case SYS_OPEN:
    syscall_open(f, args, current_thread);
    break;
  case SYS_CLOSE:
    syscall_close(f, args, current_thread);
    break;
  case SYS_READ:
    syscall_read(f, args, current_thread);
    break;
  case SYS_FILESIZE:
    syscall_filesize(f, args, current_thread);
    break;
  case SYS_SEEK:
    syscall_seek(f, args, current_thread);
    break;
  case SYS_TELL:
    syscall_tell(f, args, current_thread);
    break;
    case SYS_EXEC:
      syscall_exec(f, args, current_thread);
  default:
    break;
  }
}

static void
syscall_practice(struct intr_frame *f, uint32_t *args)
{
  f->eax = args[1] + 1;
}

static void
syscall_exit(struct intr_frame *f, int code)
{
  f->eax = code;
  printf("%s: exit(%d)\n", thread_current()->name, code);
  thread_exit();
  return;
}

static void
syscall_write(struct intr_frame *f, uint32_t *args, struct thread *current_thread)
{

  int fd = (int)args[1];
  const char *buffer = (char *)args[2];
  unsigned size = (int)args[3];

  if (!check_address(buffer))
  {
    syscall_exit(f, -1);
    return;
  }

  f->eax = -1;
  if (fd == 1 || fd == 2)
  {
    putbuf(buffer, size);
    f->eax = size;
    return;
  }

  if (!check_fd(current_thread, fd) ||
      size < 0 ||
      fd == 0)
    syscall_exit(f, -1);

  f->eax = file_write(current_thread->file_descriptors[fd], buffer, size);
}

static void
syscall_create(struct intr_frame *f, uint32_t *args)
{
  
  char *name = (char *)args[1];
  unsigned size = (int)args[2];

  if (!check_address(name) || strlen(name) <= 0)
  {
    syscall_exit(f, -1);
    return;
  }

  f->eax = filesys_create(name, size);
}

static void
syscall_remove(struct intr_frame *f, uint32_t *args)
{
  char *name = args[1];

  f->eax = filesys_remove(name);
}

static void
syscall_open(struct intr_frame *f, uint32_t *args, struct thread *current_thread)
{
  char *name = (char *) args[1];
  if (!check_address(name))
  {
    syscall_exit(f, -1);
    return;
  }

  int fd = 3;
  for (fd; fd < MAX_OPEN_FILE; fd++)
  {
    if (current_thread->file_descriptors[fd] == NULL)
      break;
  }

  if (fd == MAX_OPEN_FILE)
  {
    f->eax = -1;
    return;
  }

  current_thread->file_descriptors[fd] = filesys_open(name);
  if (current_thread->file_descriptors[fd] == NULL)
  {
    f->eax = -1;
    return;
  }

  f->eax = fd;
}

static void
syscall_close(struct intr_frame *f, uint32_t *args, struct thread *current_thread)
{

  int fd = args[1];

  if (!check_fd(current_thread, fd) ||
      fd < 3)
    syscall_exit(f, -1);

  file_close(current_thread->file_descriptors[args[1]]);
  current_thread->file_descriptors[args[1]] = NULL;

  f->eax = 1;
}

static unsigned
get_input_buffer(char *buffer, unsigned size)
{
  size_t i = 0;

  while (i < size)
  {
    buffer[i] = input_getc();
    if (buffer[i++] == '\n')
    {
      buffer[i-1] = '\0';
      break;
    }
  }
  return i;
}

static void
syscall_read(struct intr_frame *f, uint32_t *args, struct thread *current_thread)
{
  int fd = (int)args[1];
  char *buffer = (char *)args[2];
  int size = (int)args[3];

  if (!check_address(buffer))
  {
    syscall_exit(f, -1);
    return;
  }

  if (fd == 0)
  {
    f->eax = get_input_buffer(buffer, size);
    return;
  }

  if (!check_fd(current_thread, fd) ||
      fd == 1 || 
      fd == 2)
    syscall_exit(f, -1);

  f->eax = file_read(current_thread->file_descriptors[fd], buffer, size);
}

static void
syscall_filesize(struct intr_frame *f, uint32_t *args, struct thread *current_thread)
{
  int fd = (int) args[1];

  if (!check_fd(current_thread, fd) || 
      fd == 0 || 
      fd == 1 ||
      fd == 2)
      syscall_exit(f, -1);

  f->eax = file_length(current_thread->file_descriptors[fd]);
}

static void
syscall_seek(struct intr_frame *f, uint32_t *args, struct thread *current_thread)
{
  int fd = (int) args[1];
  int location = (int) args[2];

  if (!check_fd(current_thread, fd) || 
      fd == 0 || 
      fd == 1 ||
      fd == 2)
      syscall_exit(f, -1);

  file_seek(current_thread->file_descriptors[fd], location);
  f->eax = 0;
}

static void
syscall_tell(struct intr_frame *f, uint32_t *args, struct thread *current_thread)
{
  int fd = (int) args[1];

  if (!check_fd(current_thread, fd) || 
      fd == 0 || 
      fd == 1 ||
      fd == 2)
      syscall_exit(f, -1);

  f->eax = file_tell(current_thread->file_descriptors[fd]);
}

static void
syscall_exec(struct intr_frame *f, uint32_t *args, struct thread *current_thread)
{ 
  char *file_name = (char *) args[1];
  
  if (!check_address (file_name))
    syscall_exit(f, -1);

  f->eax = process_execute(file_name);
} 
