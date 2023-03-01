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


static void syscall_exit (struct intr_frame *, uint32_t, struct thread*);
static void syscall_write (struct intr_frame *, uint32_t*, struct thread*);


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
    case SYS_EXIT:
      syscall_exit (f, args, current_thread);
      break;
    case SYS_WRITE:
      syscall_write (f, args, current_thread);
      break;
    default:
        break;
  }
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