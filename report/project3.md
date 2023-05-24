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

## الگوریتم


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
deallocate , allocate
را پیاده سازی کردیم که هر کدام از بخش‌ها به سه زیر بخش مربوط به directها
indirectها
و
 double indirectها
 تقسیم می‌شود که function مربوط به هر کدام را می‌توانید در فایل inode.c مشاهده کنید.

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

## مسئولیت هر فرد
در ابتدا کیان و سروش بخش کش و علیز بخش گسترش دادن فایل‌ها و پاشا بخش دایرکتوری را تکمیل کرد و سپس به باگ‌های عجیبی برخوردیم که هر کدام به صورت جداگانه مشغول دیباگ شدیم و هر کسی بخش خودش را دیباگ کرد تا تمام تست‌ها پاس شدند.