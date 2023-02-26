تمرین گروهی ۱/۰ - آشنایی با pintos
======================

شماره گروه: ۱۱
-----
> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

سروش شرافت sorousherafat@gmail.com 

علی‌پاشا منتصری alipasha.montaseri@gmail.com

کیان بهادری  kkibian@gmail.com

مهدی علیزاده alizademhdi@gmail.com 

مقدمات
----------
> اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت بنویسید.


> لطفا در این قسمت تمامی منابعی (غیر از مستندات Pintos، اسلاید‌ها و دیگر منابع  درس) را که برای تمرین از آن‌ها استفاده کرده‌اید در این قسمت بنویسید.

آشنایی با pintos
============
>  در مستند تمرین گروهی ۱۹ سوال مطرح شده است. پاسخ آن ها را در زیر بنویسید.


## یافتن دستور معیوب

1.

0xc0000008


2.

eip=0x8048757

3.

run this commands:

`objdump -d do-nothing`

`objdump -d do-nothing | grep 8048757`


function: `_start`


```

08048754 <_start>:
 8048754:       83 ec 1c                sub    $0x1c,%esp
 8048757:       8b 44 24 24             mov    0x24(%esp),%eax
 804875b:       89 44 24 04             mov    %eax,0x4(%esp)
 804875f:       8b 44 24 20             mov    0x20(%esp),%eax
 8048763:       89 04 24                mov    %eax,(%esp)
 8048766:       e8 35 f9 ff ff          call   80480a0 <main>
 804876b:       89 04 24                mov    %eax,(%esp)
 804876e:       e8 49 1b 00 00          call   804a2bc <exit>

```


fault in `mov    0x24(%esp),%eax` instruction.


4.

we use `grep -r "_start"` command to find location of _start function in /pintos/src/lib.
function defined in the user/entry.c.


```

#include <syscall.h>

int main (int, char *[]);
void _start (int argc, char *argv[]);

void
_start (int argc, char *argv[])
{
  exit (main (argc, argv));
}

```


`sub    $0x1c,%esp`
This instruction keeps as much free space on the stack as needed and does so by decrementing the stack pointer.
Here, 8 bytes are required to place each argv and argc, as well as 4 bytes for stack_align, which requires a total of 0x16 memory houses.


`mov    0x24(%esp),%eax`

`mov    %eax,0x4(%esp)`

These instruction put the value of ‍`argv‍` in the stack to call the main function whose inputs are `argv`, `argc`.
Note that because in 8086 it is not possible to transfer between two houses of memory, the auxiliary register `eax` is used for transfer.


`mov    0x20(%esp),%eax`

`mov    %eax,(%esp)`

These instructions do the same thing as above for `argc`.


`call   80480a0 <main>`

This instruction puts the return address on the stack and then executes the main function.


`mov    %eax,(%esp)`

After returning, the output of the main function is located in the ‍`eax‍` register, and using this command, we place it in the stack to call the exit function.


`call   804a2bc <exit>`

This instruction puts the return address on the stack and then executes the exit function.


5.

Before calling the `_start` function, the arguments argv and argc must be placed in the stack.
But here, because the values ​​of argv and argc are not placed in the stack before the `_start` function is called, so when executing the instruction `mov 0x24(%esp),%eax` the value of `0x24(%esp)` is not in the user's access range and we get a `rights violation error reading page in user context` error.

## به سوی crash

6.

`main`: `0xc000e000`

```

pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000edec <incomplete sequence \357>, priority = 31,
 allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

```

`idle`: `0xc0104000`

```

pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000
e020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

```

7.
```

#0  process_execute (file_name=file_name@entry=0xc0007d50 "do-nothing") at ../../userprog/process.c:36
#1  0xc0020268 in run_task (argv=0xc00357cc <argv+12>) at ../../threads/init.c:288
#2  0xc0020921 in run_actions (argv=0xc00357cc <argv+12>) at ../../threads/init.c:340
#3  main () at ../../threads/init.c:133

```

`process_execute`:

```

tid_t
process_execute (const char *file_name)
{
  char *fn_copy;
  tid_t tid;

  sema_init (&temporary, 0);
  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy);
  return tid;
}

```

`run_task`:

```

static void
run_task (char **argv)
{
  const char *task = argv[1];

  printf ("Executing '%s':\n", task);
#ifdef USERPROG
  process_wait (process_execute (task));
#else
  run_test (task);
#endif
  printf ("Execution of '%s' complete.\n", task);
}

```

`run_action`:

```

static void
run_actions (char **argv)
{
  /* An action. */
  struct action
    {
      char *name;                       /* Action name. */
      int argc;                         /* # of args, including action name. */
      void (*function) (char **argv);   /* Function to execute action. */
    };

  /* Table of supported actions. */
  static const struct action actions[] =
    {
      {"run", 2, run_task},
#ifdef FILESYS
      {"ls", 1, fsutil_ls},
      {"cat", 2, fsutil_cat},
      {"rm", 2, fsutil_rm},
      {"extract", 1, fsutil_extract},
      {"append", 2, fsutil_append},
#endif
      {NULL, 0, NULL},
    };

  while (*argv != NULL)
    {
      const struct action *a;
      int i;

      /* Find action name. */
      for (a = actions; ; a++)
        if (a->name == NULL)
          PANIC ("unknown action `%s' (use -h for help)", *argv);
        else if (!strcmp (*argv, a->name))
          break;

      /* Check for required arguments. */
      for (i = 1; i < a->argc; i++)
        if (argv[i] == NULL)
          PANIC ("action `%s' requires %d argument(s)", *argv, a->argc - 1);

      /* Invoke action and advance. */
      a->function (argv);
      argv += a->argc;
    }

}

```

`main`:

```

main (void)
{
  char **argv;

  /* Clear BSS. */
  bss_init ();

  /* Break command line into arguments and parse options. */
  argv = read_command_line ();
  argv = parse_options (argv);

  /* Initialize ourselves as a thread so we can use locks,
     then enable console locking. */
  thread_init ();
  console_init ();

  /* Greet user. */
  printf ("Pintos booting with %'"PRIu32" kB RAM...\n",
          init_ram_pages * PGSIZE / 1024);

  /* Initialize memory system. */
  palloc_init (user_page_limit);
  malloc_init ();
  paging_init ();

  /* Segmentation. */
#ifdef USERPROG
  tss_init ();
  gdt_init ();
#endif

  /* Initialize interrupt handlers. */
  intr_init ();
  timer_init ();
  kbd_init ();
  input_init ();
#ifdef USERPROG
  exception_init ();
  syscall_init ();
#endif

  /* Start thread scheduler and enable interrupts. */
  thread_start ();
  serial_init_queue ();
  timer_calibrate ();

#ifdef FILESYS
  /* Initialize file system. */
  ide_init ();
  locate_block_devices ();
  filesys_init (format_filesys);
#endif

  printf ("Boot complete.\n");

  /* Run actions specified on kernel command line. */
  run_actions (argv);

  /* Finish up. */
  shutdown ();
  thread_exit ();
}

```

8.

`main`:

```

pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000eeac "\001", priority = 31, allelem = {prev = 0
xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0037314 <temporary+4>, next = 0xc003731c <temporary+12>}, pagedir = 0x0, magic = 3446325067}


```

`idle`:

```

pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000
e020, next = 0xc010a020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}


```

`do-nothing`:

```

pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_RUNNING, name = "do-nothing\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc010
4020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

```
9.

```

tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
if (tid == TID_ERROR)
  palloc_free_page (fn_copy);
return tid;

```
10.

afer running load:

```

${edi = 0x0, esi = 0x0, ebp = 0x0, esp_dummy = 0x0, ebx = 0x0, edx = 0x0, ecx = 0x0, eax = 0x0, gs = 0x23, fs = 0x23, es = 0x23, ds = 0x23, vec_no = 0x0, error_code = 0x0,
frame_pointer = 0x0, eip = 0x8048754, cs = 0x1b, eflags = 0x202, esp = 0xc0000000, ss = 0x23}

```

11.

In fact, the job of the `start_process` function is to load the user process and start its execution.
to implement this, it does so by simulating the return from an interrupt.
In this function, the values ​​of the registers are first calculated to run the user program and then stored in `intr_frame` struct and after that pushed into stack.
Then in intr_exit function the registers are initialized using the values ​​in the stack, and finally, by executing the `iret` command, the address of the beginning of the user's program is stored in the `eip` register and jumps to that address.
Therefore, after returning from this function(‍‍`inter_frame`), we will be in user mode.


12.

similar to if_ values.

```

eax            0x0      0
ecx            0x0      0
edx            0x0      0
ebx            0x0      0
esp            0xc0000000       0xc0000000
ebp            0x0      0x0
esi            0x0      0
edi            0x0      0
eip            0x8048754        0x8048754
eflags         0x202    [ IF ]
cs             0x1b     27
ss             0x23     35
ds             0x23     35
es             0x23     35
fs             0x23     35
gs             0x23     35

```

13.

```
#0  _start (argc=<unavailable>, argv=<unavailable>) at ../../lib/user/entry.c:9
```


## دیباگ

14.

we add `if_.esp -= 36;` into `start_process` function after calling `load`.
This is because the total amount of data we put on the stack before calling the start function is 36 bytes.
(11 byte: filename, 9 byte: stack align, 4 byte: argv[0], 4 byte: argv, 4 byte: argc, 4 byte: return address)


15.

answer: 12

As mentioned in the reference doc, in pintos, the stack pointer must be aligned in 16 bytes (4 bytes of memory).
So the stack pointer, which always points to one house before the last house used, has a remainder of 12 when divided by 16.


16.

`0xbfffff98:     0x00000001      0x000000a2`

17.

```

args[0] = 0x00000001

args[1] = 0x000000a2

```
These values ​​are the same as above.


18.

After the ‍`process_execute` function is executed, the address of the thread created for the execution of the user program is given with the `process_wait` function to wait until the end of its execution.
To do this, a `semaphore` is defined, which initially has a value of zero. now the `process_wait` function tries to decrease the value of the `semaphore`, but because its initial value is zero, it cannot do this, and this means that the execution of that thread has not finished and the kernel must wait.
Now, when the `process_exit` function is executed, which means that the thread's work is finished, the `semaphore` value increases, and in this case, the `process_wait` function can decrease its value, and this means the end of the thread work.
Note that this method will be problematic if you run several programs.


19.

thread `main`‍ has executed this function.

all threads:

```

pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000edec "\375\003", priority = 31, allelem = {prev
 = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000
e020, next = 0xc010a020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_READY, name = "do-nothing\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc01040
20, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

```
