#ifndef _LINUX_DIRENT_H
#define _LINUX_DIRENT_H

struct linux_dirent {
	long		d_ino;
	long	d_off;
	unsigned short	d_reclen;
	char		d_name[256]; /* We must not include limits.h! */
};

#endif
