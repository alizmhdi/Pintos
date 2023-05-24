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

# زیرمسیرها
## داده ساختار ها

## الگوریتم

## مسئولیت هر فرد
در ابتدا کیان و سروش بخش کش و علیز بخش گسترش دادن فایل‌ها و پاشا بخش دایرکتوری را تکمیل کرد و سپس به باگ‌های عجیبی برخوردیم که هر کدام به صورت جداگانه مشغول دیباگ شدیم و هر کسی بخش خودش را دیباگ کرد تا تمام تست‌ها پاس شدند.