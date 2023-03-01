#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_practice (struct intr_frame *, uint32_t*);
static void syscall_exit (struct intr_frame*, uint32_t, struct thread*);
static void syscall_write (struct intr_frame*, uint32_t*, struct thread*);
static void syscall_create (struct intr_frame*, uint32_t*);
static void syscall_remove (struct intr_frame*, uint32_t*);
static void syscall_open (struct intr_frame*, uint32_t*, struct thread*);
static void syscall_close (struct intr_frame*, uint32_t*, struct thread*);
static void syscall_filesize (struct intr_frame*, uint32_t*, struct thread*);
static void syscall_read (struct intr_frame*, uint32_t*, struct thread*);
static void syscall_seek (struct intr_frame*, uint32_t*, struct thread*);
static void syscall_tell (struct intr_frame*, uint32_t*, struct thread*);
static void syscall_exec (struct intr_frame*, uint32_t*, struct thread*);
static void syscall_wait (struct intr_frame*, uint32_t*, struct thread*);



static bool
check_fd (struct thread *t, int fd)
{
  if (fd == NULL || fd > MAX_OPEN_FILE || fd < 0  || t->file_descriptors[fd] == NULL) 
    return false;

  return true;
}

static bool
check_address (struct thread *t, const void *vaddr)
{
  if (vaddr == NULL || !is_user_vaddr (vaddr))
    return false;

#ifdef USERPROG
  return pagedir_get_page (t->pagedir, vaddr) == NULL ? false : true;
#endif

  return true;
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t* args = ((uint32_t*) f->esp);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

  struct thread * current_thread = thread_current ();

  switch (args[0])
  {
    case SYS_PRACTICE:
      syscall_practice (f, args);
      break;
    case SYS_EXIT:
      syscall_exit (f, args, current_thread);
      break;
    case SYS_WRITE:
      syscall_write (f, args, current_thread);
      break;
    case SYS_CREATE:
      syscall_create (f, args);
      break;
    case SYS_REMOVE:
      syscall_remove (f, args);
      break;
    case SYS_OPEN:
      syscall_open (f, args, current_thread);
      break;
    case SYS_CLOSE:
      syscall_close (f, args, current_thread);
      break;
    default:
      break;
  }
}


static void
syscall_practice (struct intr_frame *f, uint32_t* args)
{
  f->eax = args[1] + 1;
}

static void
syscall_exit (struct intr_frame *f, uint32_t code, struct thread* current_thread)
{
  f->eax = code;
  current_thread->tstatus->exit_code = code;
  thread_exit();
}

static void
syscall_write (struct intr_frame *f, uint32_t* args, struct thread* current_thread)
{
  int fd = args[1];
  const char * buffer = (const char *) args[2];
  unsigned size = args[3];

  if (!check_fd (current_thread, fd) || 
      !check_address(current_thread, buffer) || 
      !check_address(current_thread, buffer + size) ||
      size < 0 ||
      fd == 0)
    syscall_exit (f, -1, current_thread);
  
  f->eax = -1;
  if (fd == 1 || fd == 2)
  {
    putbuf (buffer, size);
    f->eax = size;
  }
  
  f->eax = file_write (current_thread->file_descriptors[fd]->pfile, buffer, size);
}

static void
syscall_create (struct intr_frame *f, uint32_t* args)
{
  char * name = args[1];
  unsigned size = args[2];

  f->eax = filesys_create (name, size);
}

static void
syscall_remove (struct intr_frame * f, uint32_t * args)
{
  char * name = args[1];

  f->eax = filesys_remove (name);
}

static void
syscall_open (struct intr_frame * f, uint32_t * args, struct thread * current_thread)
{
  int fd = 3;
  for (fd; fd <= MAX_OPEN_FILE; fd ++)
  {
    if (current_thread->file_descriptors[fd] == NULL)
      break;
  }

  if (fd == MAX_OPEN_FILE)
  {
    f->eax = -1;
    return;
  }

  if (fd > 0)
  {
    current_thread->file_descriptors[fd]->pfile = filesys_open ((const char *) args[1]);
    if (current_thread->file_descriptors[fd]->pfile == NULL){
      f->eax = -1;
      return;
    }
  }

  f->eax = fd;
}

static void
syscall_close (struct intr_frame * f, uint32_t * args, struct thread * current_thread)
{

  int fd = args[1];

  if (!check_fd (current_thread, fd) || 
      fd < 3)
    syscall_exit (f, -1, current_thread);

  file_close(current_thread->file_descriptors[args[1]]);
  current_thread->file_descriptors[args[1]] = NULL;

  f->eax = 1;
}