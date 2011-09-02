/* from linux-2.2.16/include/asm-i386/fcntl.h */
#ifndef _I386_FCNTL_H
#define _I386_FCNTL_H

/* open/fcntl - O_SYNC is only implemented on blocks devices and on files
   located on an ext2 file system */
#define LINUX_O_ACCMODE	   0003
#define LINUX_O_RDONLY	     00
#define LINUX_O_WRONLY	     01
#define LINUX_O_RDWR		     02
#define LINUX_O_CREAT		   0100	/* not fcntl */
#define LINUX_O_EXCL		   0200	/* not fcntl */
#define LINUX_O_NOCTTY	   0400	/* not fcntl */
#define LINUX_O_TRUNC		  01000	/* not fcntl */
#define LINUX_O_APPEND	  02000
#define LINUX_O_NONBLOCK	  04000
#define LINUX_O_SYNC		 010000
#define LINUX_O_DIRECT	 040000	/* direct disk access hint - currently ignored */
#define LINUX_O_LARGEFILE	0100000
#define LINUX_O_DIRECTORY	0200000	/* must be a directory */
#define LINUX_O_NOFOLLOW	0400000 /* don't follow links */
#define LINUX_O_ATOMICLOOKUP	01000000 /* do atomic file lookup */


#endif
