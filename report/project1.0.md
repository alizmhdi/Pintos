تمرین گروهی ۱/۰ - آشنایی با pintos
======================

شماره گروه:
-----
> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

سروش شرافت 

علی‌پاشامنتصری 

کیان بهادری  

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

7.

8.

9.

10.

11.

12.

13.


## دیباگ

14.

15.

16.

17.

18.

19.