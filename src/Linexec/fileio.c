/**
 * I/O syscalls
 * $Id: fileio.c,v 1.13 2001/05/29 16:05:30 mvines Exp $
 * $Id: fileio.c,v 1.14 2011/08/31 10:05:30 Ender Zheng $
 *
 * Copyright (C) 2000-2001  Michael Vines
 * Copyright (C) 2010-2011  Ender Zheng
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */ 
 
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/utime.h>
#include <sys/poll.h>
#include <sys/termios.h>
#include <sys/vfs.h>
#include <io.h>


#include "cygwin_errno.h"
#include "linux_stat.h"
#include "linux_dirent.h"
#include "linux_termbits.h"
#include "linux_openflags.h"

#include "log.h"
#include "mmap.h"

#include "syscall.h"
#include "errno.h"

#define ROUND_UP(x) (((x)+sizeof(long)-1) & ~(sizeof(long)-1))
#define NAME_OFFSET(de) ((int) ((de)->d_name - (char *) (de)))


/* cygwin does not implement the readdir() or getdents() function for
   raw file descriptors, so we need to emulate that */
#define MAX_DIR_FD 1024
DIR *dir_lookup[MAX_DIR_FD] = {NULL,};
char *dir_name_lookup[MAX_DIR_FD] = {NULL,};
extern char linexec_exe[MAX_PATH];
void change_path_to_relative(char* des, char* src)
{
    char root_path[MAX_PATH] = {0};
    char* slash = NULL;
    if( !src || !*src || !des){
        return;
    }
    if( src[0] != '/' ){
        strcpy(des, src);
        return;
    }
    strcpy(root_path, linexec_exe);
    slash = strrchr(root_path, '/');
    if( !slash ){
        strcpy(des, src);
        return;
    }
    *slash = '\0';
    strcpy(des, root_path);
    strcat(des, src);
    return;


}

void my_print(const char* fmt, ... )
{
#if FILE_NOSISY
	va_list ap;
	char buf[1024];
	int len;
	DWORD n;

	va_start(ap, fmt);
	len = vsprintf(buf, fmt, ap);
	va_end(ap);
	OutputDebugStringA(buf);
	//WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, &n, NULL);
#endif
}

/* assumes fd is valid */
static void free_dir_lookup(int fd)
{
  if (dir_lookup[fd] != NULL) {
    closedir(dir_lookup[fd]);
    dir_lookup[fd] = NULL;
  }

  if (dir_name_lookup[fd] != NULL) {
    free(dir_name_lookup[fd]);
    dir_name_lookup[fd] = NULL;
  }
}    



SYSCALL(l_read)
{
  int ret;

  /* ensure that the memory we are reading to has been
     commited (NT/2000 only) or Cygwin will return EPERM */
  forceCommit(ecx, edx);
    
  ret = read(ebx, (void*)ecx, edx);
  
  //my_print("[fileio]%d = read(%d, %08lX, %d)\n", ret, ebx, ecx, edx);
  if (ret < 0) return -errno;
  return ret;
}


SYSCALL(l_write)
{
  int ret = write(ebx, (void*)ecx, edx);
  //my_print("[fileio]%d = write(%d, %08lX, %d)\n", ret, ebx, ecx, edx);
  if (ret < 0) return -errno;
  return ret;
}


/**
 * Unfortunatly the flags for Cygwin's open() are not bitwise compatable with
 * the flags for Linux's open()
 */
static int map_open_flags(int f)
{
  int nf = 0;
  
  if (f & LINUX_O_CREAT) nf |= O_CREAT;
  if (f & LINUX_O_EXCL) nf |= O_EXCL;
  if (f & LINUX_O_NOCTTY) nf |= O_NOCTTY;
  if (f & LINUX_O_TRUNC) nf |= O_TRUNC;
  if (f & LINUX_O_APPEND) nf |= O_APPEND;
  if (f & LINUX_O_NONBLOCK) nf |= O_NONBLOCK;
  if (f & LINUX_O_SYNC) nf |= O_SYNC;
  
  if (f & LINUX_O_RDONLY) nf |= O_RDONLY;
  if (f & LINUX_O_WRONLY) nf |= O_WRONLY;
  if (f & LINUX_O_RDWR) nf |= O_RDWR;
  
  /* always use O_BINARY or Cygwin will EOL tranlations that 
     confuse some programs */
  nf |= O_BINARY;
  
  return nf;
}


SYSCALL(l_open)
{
  struct stat s;
  int fd;
  char file[MAX_PATH];
  int flags = ecx;
  int mode = edx;
  int statret;
  int cyg_flags;

  // redirect / to the folder where line.exe running
  change_path_to_relative(file, (char*)ebx);
  
  cyg_flags = map_open_flags(flags);
  
  statret = stat(file, &s);
 
  /* directories are special */
  if ((flags & LINUX_O_DIRECTORY) || ((0 == statret) &&  (s.st_mode & S_IFDIR))) {
  	
    /* open /dev/null to allocate a file descriptor */
    fd = open("/dev/null", O_RDONLY);
    if (fd < 0) return -errno;
    
    if (fd >= MAX_DIR_FD) {
      my_print("open(%s) ERROR: increase MAX_DIR_FD\n", file);
      close(fd);
      return -EMFILE;
    } 
    
    free_dir_lookup(fd);
    dir_lookup[fd] = opendir(file);
    dir_name_lookup[fd] = strdup(file);
    
  } else {
    fd = open(file, cyg_flags, mode);
	my_print("[fileio] %d = open(%s, %d, %d) --> %d\n", fd, file, cyg_flags, mode, errno);
    if (fd < 0) fd = -errno;
  }
  
  return fd;
}


SYSCALL(l_creat)
{
	//my_print("[ender]create %s", (char*)ebx);
  int ret ;
  char file[MAX_PATH];
  change_path_to_relative(file, (char*)ebx);
  my_print("[ender]create file %s \n", file);
  ret = creat((char*)file, ecx);
  if (ret < 0) return -errno;
  return ret;
}


SYSCALL(l_close)
{
  int fd = ebx;
  
  if (fd >= 0 && fd < MAX_DIR_FD) {
    free_dir_lookup(fd);
  } 
  my_print("[fileio]close(%d)\n", fd);
  return close(fd);
}


SYSCALL(l_access)
{
  //my_print("access(%s, %d)", (char*)ebx, ecx);
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	my_print("[ender]access file %s \n", file);
  if (access((char*)file, ecx)) {
    log_debug(LOG_LINEXEC_FILEIO, "<<access not ok:%d>>", errno);
    return -errno;
  }
  
  log_debug(LOG_LINEXEC_FILEIO, "<<access ok>>");
  return 0;
}


/* the Linux stat structure is not binary compatible with the cygwin version */
static void copy_stat(struct linux_stat *ls, struct stat *s)
{
  if (NULL == ls) return;
  
  ls->st_dev = s->st_dev;
  ls->st_ino = s->st_ino;
  ls->st_mode = s->st_mode;
  ls->st_nlink = s->st_nlink;
  ls->st_uid = s->st_uid;
  ls->st_gid = s->st_gid;
  ls->st_rdev = s->st_rdev;
  ls->st_size = s->st_size;
  ls->st_blksize = s->st_blksize;
  ls->st_blocks = s->st_blocks;
  ls->st_atim = (unsigned long)s->st_atim.tv_sec;
  ls->st_mtim = (unsigned long)s->st_mtim.tv_sec;
  ls->st_ctim = (unsigned long)s->st_ctim.tv_sec;
}

static void copy_stat64(struct linux_stat64* ls, struct stat* s)
{
	ls->st_dev = s->st_dev;
	ls->__st_ino = s->st_ino;
	ls->st_mode = s->st_mode;
    ls->st_nlink = s->st_nlink;
    ls->st_uid = s->st_uid;
	ls->st_gid = s->st_gid;

    ls->st_rdev = s->st_rdev;
	ls->st_size = s->st_size;
    ls->st_blksize = s->st_blksize;
    ls->st_blocks = s->st_blocks;
    ls->st_atim.tv_sec = s->st_atim.tv_sec;
    ls->st_mtim.tv_sec = s->st_mtim.tv_sec;
    ls->st_ctim.tv_sec = s->st_ctim.tv_sec;
	ls->st_atim.tv_nsec = s->st_atim.tv_nsec;
	ls->st_mtim.tv_nsec = s->st_mtim.tv_nsec;
	ls->st_ctim.tv_nsec = s->st_ctim.tv_nsec;

}


/*
 * Stupid Pico/Pilot/Pine!!!  They are assuming that the returned st_size
 * field from a stat() syscall for a directory is the actual size of the 
 * directory.  But Cygwin returns zero and they don't check for that.
 *
 * The unfortunate result is that a diretory stat() is _very_ expensive...
 */
static void get_dir_size(char *name, struct linux_stat *ls)
{
  DIR *dir;
  struct dirent *d;
  int size = 0;
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)name);
  my_print("[ender]get dir size %s \n", file);
  dir = opendir(file);
  if (NULL == dir) return;
  
  while (NULL != (d = readdir(dir))) {
    size += ROUND_UP(NAME_OFFSET(d) + strlen(d->d_name)+1);
  } 
  
  ls->st_size = size;
}


SYSCALL(l_stat)
{
  int ret;
  struct stat s;
  struct linux_stat *ls = (struct linux_stat*)ecx;
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
  ret = stat((char*)file, &s);
  my_print("[ender]stat file %s , ret = %x, error = %x\n", file, ret, errno);

//  my_print("%d = stat(%s) (errno=%d)\n", ret, (char*)ebx, errno);
  if (ret != 0) return -errno;
  
  copy_stat(ls, &s);
  my_print("[ender]ls->mode = %x, s->mode = %x\n", ls->st_mode, s.st_mode);
  
  if (S_ISDIR(s.st_mode)) {
    get_dir_size((char*)ebx, ls);
  }
  return 0;
}



SYSCALL(l_lstat)
{
  int ret;
  struct stat s;
  struct linux_stat *ls = (struct linux_stat*)ecx;
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
  my_print("[ender]lstat file %s \n", file);
  ret = lstat((char*)file, &s);
//  my_print("%d = lstat(%s) (errno=%d)\n", ret, (char*)ebx, errno);
  if (ret != 0) return -errno;
  
  copy_stat(ls, &s);

  if (S_ISDIR(s.st_mode)) {
    get_dir_size((char*)ebx, ls);
  }
  
  return 0;
}


SYSCALL(l_fstat)
{
  int ret;
  struct stat s;
  struct linux_stat *ls = (struct linux_stat*)ecx;
  int fd = ebx;
  
  if (NULL != dir_name_lookup[fd]) {
    ret = stat(dir_name_lookup[fd], &s);
  } else {
    ret = fstat(fd, &s);
  }
  
//  my_print("%d = fstat(%d, %08X) (errno=%d)\n", ret, fd, ls, errno);
  if (ret != 0) return -errno;
    
  copy_stat(ls, &s);
  
  return 0;  
}


SYSCALL(l_ioctl)
{
  int ret;
  
//  my_print("ioctl(%d, %X, %X)\n", ebx, ecx, edx);

  /* convert the ioctl requests to their cygwin equivalents... */  
  switch (ecx) {
  case 0x5401: /* TCGETS */
    {
      struct linux_termios *lt = (struct linux_termios*)edx;
      struct termios t;
      
      ret = tcgetattr(ebx, &t);
      if (ret < 0) return -errno;
      
      bzero((void*)lt, sizeof(*lt));
      
      /* NOTE: this isn't quite correct (especially the c_cc field)!!! */
      lt->c_iflag = t.c_iflag;
      lt->c_oflag = t.c_oflag;
      lt->c_lflag = t.c_lflag;
      lt->c_line = t.c_line;
      memcpy(&lt->c_cc, &t.c_cc, sizeof(lt->c_cc));
      return 0; 
    }
    break;
    
  case 0x5402: /* TCSETS */    
  case 0x5403: /* TCSETSW */
  case 0x5404: /* TCSETSF */
    {
      struct linux_termios *lt = (struct linux_termios*)edx;
      struct termios t;
      int flag;

      bzero((void*)&t, sizeof(t));

      /* NOTE: this isn't quite correct (especially the c_cc field)!!! */
      t.c_iflag = lt->c_iflag;
      t.c_oflag = lt->c_oflag;
      t.c_lflag = lt->c_lflag;
      t.c_line = lt->c_line;
      memcpy(&t.c_cc, &lt->c_cc, sizeof(lt->c_cc));
      
      switch (ecx) {
      case 0x5402:
        flag = TCSANOW;
        break;
      case 0x5403:
        flag = TCSADRAIN;
      default:
        flag = TCSAFLUSH;
        break;
      }
      
      ret = tcsetattr(ebx, flag, &t);
      if (ret < 0) return -errno;
      return 0; 
    }
 // This causes problems! 
 // case 0x5421: /* FIONBIO */
 //   ecx = FIONBIO;
 //   break;
    
  case 0x541B: /* FIONREAD */
    ecx = FIONREAD;
    break;
    
  case 0x5413: /* TIOCGWINSZ */
    ecx = TIOCGWINSZ;
    break;

  case 0x5414: /* TIOCSWINSZ */
    ecx = TIOCSWINSZ;
    break;

    
  /* TODO: probably many more here... */
  default:
    //my_print("WARNING: got an unchecked ioctl request (%X)\n", ecx);
    break;
  }
  
  ret = ioctl(ebx, ecx, (char*)edx);

//  my_print("%d = ioctl(%d, %X)\n", ret, ebx, ecx);
  
  if ( ret < 0) return -errno;
  return ret;
}  




#if 0
typedef unsigned long int __fd_mask;

#define __FD_SET_SIZE 1024
#define __NFDBITS (8 * sizeof(__fd_mask))
#define BIT(i) (1UL << ((i)&(__NFDBITS-1)))
#endif

struct sel_arg_struct {
  unsigned long n;
  fd_set *inp, *outp, *exp;
  struct timeval *tvp;
};


SYSCALL(l_select)
{
  int ret;
  struct sel_arg_struct *a = (struct sel_arg_struct *)ebx;

#if 0  
  fd_set test;
  
  my_print("select (%ld, %08lX, %08lX, %08lX, %08lX)\n", a->n, (DWORD)a->inp, 
         (DWORD)a->outp, (DWORD)a->exp, (DWORD)a->tvp);
  my_print("timeval: s:%ld, us:%ld\n", a->tvp->tv_sec, a->tvp->tv_usec);
  my_print("in:%08lX, out:%08lX, exp:%08lX\n", a->inp != 0 ? *(DWORD*)a->inp : 0, a->outp != 0 ? *(DWORD*)a->outp : 0,
          a->exp != 0 ? *(DWORD*)a->exp : 0);
          
  FD_ZERO(&test);
  FD_SET(a->n-1, &test);
  my_print("test:%08lX\n", *(DWORD*)&test);

#endif
  ret = select(a->n, a->inp, a->outp, a->exp, a->tvp);
  
#if 0  
  my_print("exit select (%d)\n", ret);
  getchar();
#endif
             
  if (ret < 0) return -errno;
  return ret;
}


SYSCALL(l_newselect)
{
  int ret = select(ebx, (fd_set*)ecx, (fd_set*)edx, 
             (fd_set*)esi, (struct timeval*)edi);
  if (ret < 0) {
    return -errno;
  }
  return ret;
}


SYSCALL(l_truncate)
{
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	my_print("[ender]truncate file %s \n", file);
  if (truncate((char*)file, ecx) < 0) return -errno;
  return 0;
}


SYSCALL(l_ftruncate)
{
  if (ftruncate(ebx, ecx) < 0) return -errno;
  return 0;
}

  
SYSCALL(l_dup)
{
  int ret = dup(ebx);
  if (ret < 0) return -errno;
  return ret;
}

SYSCALL(l_dup2)
{  
  int ret = dup2(ebx, ecx);
  if (ret < 0) return -errno;
  return ret;
}
  
  
SYSCALL(l_link)
{
    char file[MAX_PATH];
    char file2[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
    change_path_to_relative(file2, (char*)ecx);
  return link((char*)file, (char*)file2);
}


SYSCALL(l_unlink)
{
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
  return unlink((char*)file);  
}

SYSCALL(l_chdir)
{
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	my_print("[ender]chdir file %s \n", file);
  if (chdir((char*)file)) return -errno;
  return 0;
}

SYSCALL(l_mknod)
{
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	my_print("[ender]mknod file %s \n", file);
  return mknod((char*)file, ecx, edx);  
}

SYSCALL(l_chmod)
{
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	my_print("[ender]chmod file %s \n", file);
  return chmod((char*)file, ecx);  
}


SYSCALL(l_lchown)
{
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	my_print("[ender]lchown file %s \n", file);
  return lchown((char*)file, ecx, edx);
}


SYSCALL(l_chown)
{
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	my_print("[ender]chown file %s \n", file);
  return chown((char*)file, (uid_t)ecx, (gid_t)edx);
}


SYSCALL(l_lseek)
{
  int fd = ebx;
  
  if (fd >= 0 && fd < MAX_DIR_FD) {
    if (dir_lookup[fd] != NULL) {
      switch (edx) {
        case SEEK_SET:
          break;
        case SEEK_CUR:
          ecx += telldir(dir_lookup[fd]);
          break;
        default:
        case SEEK_END:
          return -EBADF; /* don't support SEEK_END right now */
      }
      
      seekdir(dir_lookup[fd], ecx);

      return telldir(dir_lookup[fd]);
    }
  }
  return lseek(fd, ecx, edx);
}


SYSCALL(l_rename)
{
    char file[MAX_PATH];
    char file2[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
    change_path_to_relative(file2, (char*)ecx);
	my_print("[ender]rename file %s \n", file);
  if (rename((char*)file, (char*)file2)) return -errno;
  return 0;
}


SYSCALL(l_mkdir)
{
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	my_print("[ender]mkdir file %s \n", file);
  if (mkdir((char*)file, ecx)) return -errno;
  return 0;
}

SYSCALL(l_rmdir)
{
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	my_print("[ender]rmdir file %s \n", file);
  if (rmdir((char*)file)) return -errno;
  return 0;
}


SYSCALL(l_symlink)
{
    char file[MAX_PATH];
    char file2[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
    change_path_to_relative(file2, (char*)ecx);
  if (symlink((char*)file, (char*)file2) < 0) return -errno;
  return 0;
}

SYSCALL(l_readlink)
{
    int ret;
    char file[MAX_PATH];
    char file2[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
    change_path_to_relative(file2, (char*)ecx);

  
  ret = readlink((char*)file, (char*)file, edx);
  
  if (ret < 0) return -errno;
  return ret; 
}


SYSCALL(l_fchmod)
{
  if (fchmod(ebx, ecx) < 0) return -errno;
  return 0;
}

SYSCALL(l_fchown)
{
  if (fchown(ebx, ecx, edx) < 0) return -errno;
  return 0;
}


SYSCALL(l_fcntl)
{
  
  int ret = fcntl(ebx, ecx, edx);
  //my_print("%d = fcntl(%d,%d,%d)\n", ret, ebx, ecx, edx);
  if (ret < 0) return -errno;
  return ret;
}


SYSCALL(l_getdents)
{
  struct dirent *d;
  DIR *dir;
  int fd = ebx;
  int count = edx;
  int retcount;
  int dirpos;
  struct linux_dirent dent;
  
  if (fd < 0 || fd >= MAX_DIR_FD) {
    return -EBADF;
  }
  
//  log_debug(LOG_LINEXEC_GETDENTS, "getdents(%d, %08X, %d)\n", fd, ecx, count);
  
  dir = dir_lookup[fd];
  
  if (NULL == dir) {
    return -ENOTDIR;
  }

  if (count < sizeof(struct linux_dirent)) {
    return -EINVAL;   
  }

  retcount = 0;
  while (count > 0) {
//    my_print ("%08X\n", ecx);

    dirpos = telldir(dir);

    d = readdir(dir);
    if (NULL == d) {
//      my_print("end of dir\n");
      break;
    }

//    log_debug(LOG_LINEXEC_GETDENTS, "(%s:%ld)", d->d_name, d->d_ino);

    dent.d_ino = d->d_ino;
    dent.d_off = dirpos;
    strncpy(dent.d_name, d->d_name, sizeof(dent.d_name));
	
 //   dent.d_reclen = sizeof(dent.d_ino) + sizeof(dent.d_off) + 
//                    sizeof(dent.d_reclen) + strlen(dent.d_name)+1;
    dent.d_reclen = ROUND_UP(NAME_OFFSET(&dent) + strlen(dent.d_name)+1);

    if (count <= dent.d_reclen) {
      seekdir(dir, dirpos); /* restore dir position */
      break;
    }

    memcpy((char*)ecx, (char*)&dent, dent.d_reclen);

    retcount += dent.d_reclen;
    count -= dent.d_reclen;
    ecx += dent.d_reclen;
  }

//  log_debug(LOG_LINEXEC_GETDENTS, "returning: %d", retcount);
  
  return retcount;
}


SYSCALL(l_fchdir)
{
  int fd = ebx;
  
  if (fd < 0 || fd >= MAX_DIR_FD) {
    return -EBADF;
  }
  
  if (NULL == dir_name_lookup[fd]) {
    return -EBADF; 
  }
  
  /* cygwin doesn't implement fchdir() */
  return chdir(dir_name_lookup[fd]);  
}


SYSCALL(l_getcwd)
{
  char cwd[PATH_MAX];
  
  if (NULL == getcwd(cwd, sizeof(cwd))) {
    return -ENOENT;
  }
  
  if (ecx < strlen(cwd)) {
    return -ERANGE; 
  }
  
  strcpy((char*)ebx, cwd);
  
  return strlen(cwd)+1;
}

SYSCALL(l_utime)
{
  if (utime((char*)ebx, (struct utimbuf*)ecx) < 0) return -errno;
  return 0;
}


SYSCALL(l_pipe)
{
  if (pipe((int*)ebx) < 0) return -errno;
  return 0;
}
  

SYSCALL(l_readv)
{
  int ret = readv(ebx, (const struct iovec*)ecx, edx);
  if (ret < 0) return -errno;
  return ret;
}

SYSCALL(l_writev)
{
  int ret = writev(ebx, (const struct iovec*)ecx, edx);
  if (ret < 0) return -errno;
  return ret;
}


SYSCALL(l_poll)
{
  int ret = poll((struct pollfd*)ebx, ecx, edx);
  
  if (ret < 0) return -errno;
  return ret;
}


SYSCALL(l__llseek)
{
  int fd = ebx;  
  unsigned long offset_high = ecx;
  unsigned long offset_low = edx;
  long long *result = (long long*)esi;
  unsigned int whence = edi;
  long long offset = 0;
  int ret;
  long long fp;
  fp = (long long)lseek(fd, 0, SEEK_CUR);
  offset = ((long long)offset_high << 32)+(long long)offset_low;
  if( whence != SEEK_SET ) /*change to abs*/
	offset += fp;
  ret = lseek(fd, offset, SEEK_SET);
	//my_print("[fileio]%d = lseek(%d, %lld, %d) --> %d\n", ret, fd, offset, SEEK_SET, errno);
  if (ret < 0) return -errno;
  
  
  if (result) {
    *result = ret;
  }
  
  return 0;
}


SYSCALL(l_fsync)
{
  int ret = fsync(ebx); 
  
  if (ret < 0) return -errno;
  return ret;
}


SYSCALL(l_statfs)
{
  int ret = statfs((const char *)ebx, (struct statfs*)ecx);
  //my_print("[ender]statfs %s \n", ebx);;

  if (ret < 0) return -errno;
  return ret; 
}


SYSCALL(l_fstatfs)
{
  int ret = fstatfs(ebx, (struct statfs*)ecx);
  //my_print("[ender]fstatfs %s \n", ebx);;

  if (ret < 0) return -errno;
  return ret; 
}

SYSCALL(l_stat64)
{
	struct stat r;
	int ret;
	struct linux_stat64* sta_64 = (struct linux_stat64*)ecx;
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	ret = stat(file, &r);
	if( ret < 0 ) return -errno;
	copy_stat64(sta_64, &r);
	return ret;
}

SYSCALL(l_lstat64)
{
	struct stat r;
	int ret;
	struct linux_stat64* sta_64 = (struct linux_stat64*)ecx;
    char file[MAX_PATH];
    change_path_to_relative(file, (char*)ebx);
	ret = lstat(file, &r);
	if( ret < 0 ) return -errno;
	copy_stat64(sta_64, &r);
	return ret;
}

SYSCALL(l_fstat64)
{
	struct stat r;
	int ret;
	struct linux_stat64* sta_64 = (struct linux_stat64*)ecx;
	ret = fstat(ebx, &r);
	if( ret < 0 ) return -errno;
	copy_stat64(sta_64, &r);
	return ret;

}
