<div dir="rtl">

## گزارش تمرین گروهی ۳


شماره گروه: 7

-----

> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

سروش شرافت sorousherafat@gmail.com

علی‌پاشا منتصری alipasha.montaseri@gmail.com

کیان بهادری  kkibian@gmail.com

مهدی علیزاده alizademhdi@gmail.com

# بافرکش
## داده ساختار ها

داده ساختار های استفاده شده همانند مستند طراحی پروژه هستند.


```C
#define CACHE_SIZE 64

struct cache_block {
    block_sector_t sector_index;
    char data[BLOCK_SECTOR_SIZE];
    bool valid;
    bool dirty;
    struct list_elem elem; 
    struct lock block_lock;

}

struct cache_block cache_blocks[CACHE_SIZE];
struct list cache_list;
struct lock cache_lock;

```
علاوه بر این ها، تعدادی عدد برای `hit_count` و `miss_count` و تعداد read یا write نیز ذخیره می‌کنیم که در تست کردن کش به ما کمک می‌کنند.
```C
bool cache_initialized;

long long hit_count;
long long miss_count;
long long read_count;
long long write_count;
struct lock stat_lock;
```

همچنین برای استفاده از این اعداد، تابع و #define زیر را استفاده کردیم.
```C
/* Defines used for updating cache stats. */
#define HIT 1
#define MISS 0
#define READ 2
#define WRITE 3

size_t cache_count (int mode);

```
برای دسترسی به این مقادیر، نیاز بود system call مربوط به این تابع نیز پیاده سازی کنیم.

## الگوریتم
ما الگوریتم LRU را برای پیاده سازی انتخاب کرده بودیم. طبق الگوریتمی که در مستند طراحی آورده شد، هر بلوک کش را در یک لیست قرار می‌دهیم و در هر دسترسی بلوک را به ابتدای لیست می‌آوریم. اینگونه بلوک آخر همواره `least-recently-used` بوده و می‌توانیم در صورت نیاز ابتدا آنرا flush کنیم (اگر dirty بود آنرا در دیسک نوشته) و سپس آنرا به عنوان بلوک قابل استفاده به کرنل بدهیم.


# فایل‌های قابل گسترش
## داده ساختار ها
<div dir='ltr'>

```c
#define DIRECT_BLOCK 123
#define INDIRECT_BLOCK 128

#define MIN(a,b) (((a)<(b))?(a):(b))

struct inode_disk
  {
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */

    bool is_dir;

    block_sector_t direct[DIRECT_BLOCK];         /* Direct blocks of inode. */
    block_sector_t indirect;            /* Indirect blocks of inode. */
    block_sector_t double_indirect;     /* Double indirect blocks of indoe. */
  };

struct inode
  {
    ...
    struct lock f_lock;                 /* Synchronization between users of inode. */
  };

``` 

## الگوریتم
در این بخش ما به صورت کلی دو عملی
`deallocate` , `allocate`
را پیاده سازی کردیم که هر کدام از بخش‌ها به سه زیر بخش مربوط به `direct`ها
`indirect`ها
و
 `double indirect`ها
 تقسیم می‌شود که function مربوط به هر کدام را می‌توانید در فایل `inode.c` مشاهده کنید.

# زیرمسیرها
## داده ساختار ها
```C
struct thread {
  ...
  struct dir* work_dir;
  ...
}

struct inode_disk
{
  ...
  bool is_dir;
  ...
};

```
تغییرات اصلی که در ساختار کد داشتیم داخل 
`thread`
و
`inode_disk`
بودند که به برای تغییر اول نیاز داشتیم برای هر
`thread`
ذخیره کنیم در کدام
`directory`
است تا بتوانیم به صورت مناسبی 
`relative pathing`
را انجام دهیم.
و همچنین برای تغییر دوم در تعداد زیادی از 
syscallها
نیاز بود تبعیض بین
dir
و
file
قائل شویم و درنتیجه داخل
`inode_disk`
این را ذخیره می‌کردیم.

برای هندل کردن حالات 
`relative pathing`
از تابع داده شده در داک به نام
`get_next_part`
استفاده کردیم که نام بخش بعدی را به ترتیب به ما می‌دهد و اگر آدرس مطلق بود از
`root`
و اگرنه از 
`work_dir`
که داخل 
`thread`
تعریف کردیم پیمایش می‌کنیم.
برای انجام این کار از تابع کمکی
`split_path_dir`
هم استفاده کردیم که مقدار
`path`
را به عنوان ورودی گرفته و ساختار
`dir`
متناظر با 
`directory`
آن را بازمی‌گرداند.

همچنین تعدادی از 
syscallهایی
که از قبل در فاز 1 
ایجاد کردیم هم تغییرات متناسب اعمال کردیم تا بتوانیم با
directoryها
هم کار کنیم و 
syscallهای
جدید
`chdir`،
`mkdir`،
`readdir`،
`isdir` و  `inumber`
را
هم اضافه کردیم.



## الگوریتم

# تست‌های پیاده‌سازی شده
ما برای این قسمت، دو تست اول و سوم را پیاده‌سازی کردیم.

در تست `bf-hit`، می‌خواهیم hitrate کشمان را به دست آوریم. به این صورت که ابتدا یک ورودی رندوم در فایل می‌نویسیم. سپس آن را می‌بندیم و دوباره باز می‌کنیم. حال کش را invalidate می‌کنیم. در این مرحله اگر یک دور فایل گفته شده را بخوانیم؛ سپس بدون invalidate کردن کش دوباره محتویات فایل را بخوانیم، خواهیم دید که درصد hitrate مربوطه در دور دوم بیشتر است که به معنی به کار گرفته شدن کش می‌باشد.

```
Copying tests/filesys/extended/bf-hit to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu-system-i386 -device isa-debug-exit -hda /tmp/4mRYjakLTt.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading............
Kernel command line: -q -f extract run bf-hit
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer...  405,913,600 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 192 sectors (96 kB), Pintos OS kernel (20)
hda2: 243 sectors (121 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'bf-hit' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'bf-hit':
(bf-hit) begin
(bf-hit) create "a"
(bf-hit) open "a"
(bf-hit) write 16384 bytes to "a"
(bf-hit) close "a"
(bf-hit) open "a"
(bf-hit) get baseline cache stats
(bf-hit) invalidate cache
(bf-hit) read 16384 bytes from "a"
(bf-hit) get cold cache stats
(bf-hit) close "a"
(bf-hit) open "a"
(bf-hit) read 16384 bytes from "a"
(bf-hit) get new cache stats
(bf-hit) old hit rate percent: 65, new hit rate percent: 81
(bf-hit) close "a"
(bf-hit) end
bf-hit: exit(0)
Execution of 'bf-hit' complete.
Timer: 132 ticks
Thread: 23 idle ticks, 91 kernel ticks, 18 user ticks
hdb1 (filesys): 79 reads, 525 writes
hda2 (scratch): 242 reads, 2 writes
Console: 1389 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...

```

```
PASS

```
در تست `opt-write`، بهینه‌سازی انجام شده برای عملیات write را بررسی کردیم. زمانی که می‌خواهیم کل یک بلوک از حافظه نهان را بازنویسی کنیم، نیازی نیست ابتدا محتویات آن را از حافظه بخوانیم. بنابراین با بررسی این مورد می‌توانیم عملکرد بهتری داشته باشیم. برای آزمایش این موضوع، یک فایل را باز کرده و ورودی رندومی در آن می‌نویسیم. سپس کش را invalidate کرده و دوباره در این فایل چیزی می‌نویسیم. همانطور که انتظار می‌رود، تعداد عملیات block_write انجام شده بسیار بیشتر از block_read است که به معنای درست کار کردن بهینه‌سازی گفته شده است.

```
Copying tests/filesys/extended/opt-write to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu-system-i386 -device isa-debug-exit -hda /tmp/eRIsVfWMO4.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading............
Kernel command line: -q -f extract run opt-write
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer...  509,542,400 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 192 sectors (96 kB), Pintos OS kernel (20)
hda2: 240 sectors (120 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'opt-write' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'opt-write':
(opt-write) begin
(opt-write) create "a"
(opt-write) open "a"
(opt-write) write 102400 bytes to "a"
(opt-write) invalidate cache
(opt-write) write 102400 bytes to "a"
(opt-write) old reads: 43, old writes: 888, new reads: 49, new writes: 1234
(opt-write) close "a"
(opt-write) end
opt-write: exit(0)
Execution of 'opt-write' complete.
Timer: 205 ticks
Thread: 53 idle ticks, 83 kernel ticks, 70 user ticks
hdb1 (filesys): 53 reads, 1298 writes
hda2 (scratch): 239 reads, 2 writes
Console: 1244 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...

```

```
PASS

```

همچنین توجه کنید که یکی از مشکلاتی که در تست‌های بالا برخوردیم این بود که در ورژن 
persistence
تست‌ها چون فایل سیستم ری‌استارت می‌شود باید در انتهای تست‌ها فایل ساخته شده را 
remove می‌کردیم که با انجام این کار تست‌ها پاس شدند.

# مسئولیت هر فرد
در ابتدا کیان و سروش بخش کش و علیز بخش گسترش دادن فایل‌ها و پاشا بخش دایرکتوری را تکمیل کرد و سپس به باگ‌های عجیبی برخوردیم که هر کدام به صورت جداگانه مشغول دیباگ شدیم و هر کسی بخش خودش را دیباگ کرد تا تمام تست‌ها پاس شدند.