#ifndef _I386_STAT_H
#define _I386_STAT_H

struct linux___old_kernel_stat {
	unsigned short st_dev;
	unsigned short st_ino;
	unsigned short st_mode;
	unsigned short st_nlink;
	unsigned short st_uid;
	unsigned short st_gid;
	unsigned short st_rdev;
	unsigned long  st_size;
	unsigned long  st_atim;
	unsigned long  st_mtim;
	unsigned long  st_ctim;
};

struct linux_stat {
	unsigned short st_dev;
	unsigned short __pad1;
	unsigned long st_ino;
	unsigned short st_mode;
	unsigned short st_nlink;
	unsigned short st_uid;
	unsigned short st_gid;
	unsigned short st_rdev;
	unsigned short __pad2;
	unsigned long  st_size;
	unsigned long  st_blksize;
	unsigned long  st_blocks;
	unsigned long  st_atim;
	unsigned long  __unused1;
	unsigned long  st_mtim;
	unsigned long  __unused2;
	unsigned long  st_ctim;
	unsigned long  __unused3;
	unsigned long  __unused4;
	unsigned long  __unused5;
};

/* This matches struct stat64 in glibc2.1, hence the absolutely
 * insane amounts of padding around dev_t's.
 */
struct linux_stat64
  {
    __dev_t st_dev;             /* Device.  */

    unsigned int __pad1;
    __ino32_t __st_ino;                   /* 32bit file serial number.    */
    mode_t st_mode;                   /* File mode.  */
    nlink_t st_nlink;                 /* Link count.  */
    __uid_t st_uid;             /* User ID of the file's owner. */
    __gid_t st_gid;             /* Group ID of the file's group.*/

    __dev_t st_rdev;                    /* Device number, if device.  */
    unsigned int __pad2;
    _off64_t st_size;                  /* Size of file, in bytes.  */
    blksize_t st_blksize;     /* Optimal block size for I/O.  */
    __blkcnt64_t st_blocks;     /* Nr. 512-byte blocks allocated.  */
#if 1
    /* Nanosecond resolution timestamps are stored in a format
       equivalent to 'struct timespec'.  This is the type used
       whenever possible but the Unix namespace rules do not allow the
       identifier 'timespec' to appear in the <sys/stat.h> header.
       Therefore we have to handle the use of this header in strictly
       standard-compliant sources special.  */
    struct timespec st_atim;            /* Time of last access.  */
    struct timespec st_mtim;            /* Time of last modification.  */
    struct timespec st_ctim;            /* Time of last status change.  */
# define st_atime st_atim.tv_sec        /* Backward compatibility.  */
# define st_mtime st_mtim.tv_sec
# define st_ctime st_ctim.tv_sec
#else
    __time_t st_atime;                  /* Time of last access.  */
    unsigned long int st_atimensec;     /* Nscecs of last access.  */
    __time_t st_mtime;                  /* Time of last modification.  */
    unsigned long int st_mtimensec;     /* Nsecs of last modification.  */
    __time_t st_ctime;                  /* Time of last status change.  */
    unsigned long int st_ctimensec;     /* Nsecs of last status change.  */
#endif
    __ino64_t st_ino;                   /* File serial number.          */

 };

#endif
