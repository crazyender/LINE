/**
 * Miscellaneous syscalls
 * $Id: syscall_impl.c,v 1.14 2001/04/26 16:52:07 mvines Exp $
 * $Id: syscall_impl.c,v 1.15 2011/08/31 10:05:30 Ender Zheng $
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
 
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include "linux_utsname.h"

#include "log.h"
#include "linexec.h"
#include "syscall_impl.h"
#include "cygwin_errno.h"
#include "errno.h"
#include "mmap.h"
#include "nativeso.h"
#include "internal_syscalls.h"


/* syscalls that need to be implemented */
SYSCALL(impl_me)
{
  printf("*** SYSCALL needs to be implemented ***\n");
  printf("Syscall: %s (%d)\n", syscall_names[eax], eax);
  printf("Parameters:\n");
  printf("%08X (%d), %08X (%d), %08X (%d), %08X (%d), %08X (%d)\n",
      ebx, ebx, ecx, ecx, edx, edx, esi, esi, edi, edi);
  MessageBox(0, syscall_names[eax], "Unimplemented Syscall", MB_ICONEXCLAMATION);
  return -ENOSYS;
}


/* syscalls that will not be implemented (for now at least) */
SYSCALL(not_impl)
{
  printf("*** SYSCALL %s (%d) not implemented ***\n", 
         syscall_names[eax], eax);
  return -ENOSYS;
}


/* syscalls that will not be implemented, but don't complain about it */
SYSCALL(not_impl_quiet)
{
	//printf("[ender]call no impl sys %d\n", eax);
  return -ENOSYS;
}



/* invoked if we get a syscall out of the valid range (1-256) */
SYSCALL(bad_syscall)
{
	//printf("[ender]call bad_syscall %d\n", eax);
  switch (eax) {
  /* got shared object fixup request */
  case SYSCALL_NSO_FIXUP:
    do_symfixup((struct nativeso_symtable*)ebx, (char*)ecx);
    break;
    
  /* core dump request */
  case SYSCALL_COREDUMP: 
    do_coredump(eax, ebx, ecx, edx, esi, edi, ebp);
    exit(-1);
    break;

  case SYSCALL_DUMPMEMORYMAP:
    dumpMemoryMap();
    exit(-1);
    break;

  default:    
    printf("*** BAD SYSCALL: %d ***\n", eax);
    MessageBox(0, "Bad syscall", "Oops", 0);
    exit(-1);
    break;
  }
  return -ENOSYS;
}

SYSCALL(l_print_num)
{
	//my_print("[syscall] %d: %s\n", eax, syscall_names[eax]);
	return eax;
}

SYSCALL(ll_set_thread_area)
{
	return 0;
}

/*
 * misc syscalls that have no better place to go
 */

SYSCALL(l_exit)
{


  exit(ebx);
  return 0;
}

SYSCALL(l_time)
{
  return time((time_t*)ebx);  
}
 

SYSCALL(l_getpid)
{
  return getpid();  
}

SYSCALL(l_setuid)
{
  return setuid(ebx);
}

SYSCALL(l_getuid)
{
  return getuid();
}


/* ONLY PARTIALLY IMPLEMENTED */
SYSCALL(l_setreuid)
{
  int ret;
  
  log_debug(LOG_LINEXEC_UID, "setreuid(%d, %d)", ebx, ecx);
  log_warning(LOG_LINEXEC_UID, "seteuid() ignores the first parameter");
  
  ret = seteuid(ecx); 
  if (ret != 0) return -errno;
  return ret;
}


/* ONLY PARTIALLY IMPLEMENTED */
SYSCALL(l_setregid)
{
  int ret;
  
  log_debug(LOG_LINEXEC_UID, "setregid(%d, %d)", ebx, ecx);
  log_warning(LOG_LINEXEC_UID, "setregid() ignores the first parameter");
  
  ret = setegid(ecx); 
  if (ret != 0) return -errno;
  return ret;
}

/* ONLY PARTIALLY IMPLEMENTED */
SYSCALL(l_setresuid)
{
  log_warning(LOG_LINEXEC_UID, "setresuid() always returns success");
  return 0;  
}


SYSCALL(l_stime)
{
  return -EPERM; // not allowed to play with the system time
}


SYSCALL(l_sync)
{
  // do nothing
  return 0;
}

SYSCALL(l_setgid)
{
  return setgid(ebx); 
}

SYSCALL(l_getgid)
{
  return getgid();
}

SYSCALL(l_geteuid)
{
  return geteuid();
}

SYSCALL(l_getegid)
{
  return getegid();
}

SYSCALL(l_getpgrp)
{
  return getpgrp();
}


SYSCALL(l_umask)
{
  return umask(ebx);
}


SYSCALL(l_setrlimit)
{
  if (!setrlimit(ebx, (struct rlimit*)ecx)) return -errno;
  
  return 0;
}


SYSCALL(l_getrlimit)
{
  if (!getrlimit(ebx, (struct rlimit*)ecx)) return -errno;
  
  return 0;
}

SYSCALL(l_uname)
{
  struct utsname u;
  struct linux_utsname *lu;
  int ret;
  
  lu = (struct linux_utsname*)ebx;
  
  ret = uname(&u);
  if (0 == ret) {
    strcpy(lu->sysname, UMANE_LINE_SYSNAME);
    strcpy(lu->nodename, u.nodename);
    /* glibc complains that "FATAL: kernel too old" if we return the
       cygwin release string (because it's in the 1.x range!) */
    strcpy(lu->release, UNAME_LINUX_RELEASE); // u.release);
    strcpy(lu->version, UNAME_LINE_VERSION);
    strcpy(lu->machine, u.machine);
    //printf("uname: sysname = %s, nodename = %s, release = %s, version = %s, machine = %s\n",
    //    lu->sysname, lu->nodename, lu->release, lu->version, lu->machine);
  }
  
  return ret;
}


SYSCALL(l_ioperm)
{
  return -EPERM;  // this will never be implemented
}


SYSCALL(l_iopl)
{
  return -EPERM;  // this will never be implemented
}


SYSCALL(l_setdomainname)
{
  printf("SYSCALL setdomainname: not allowed: %s\n", (char*)ebx);
  return -EPERM; // not allowed to change the hostname
}


SYSCALL(l_quotactl)
{
  return -ENOPKG;  // no quota support
}


SYSCALL(l_bdflush)
{
  return -EPERM;
}


SYSCALL(l_personality) 
{
  if (0 == ebx) return 0;

  printf("SYSCALL personality: invalid persona %d\n", ebx);
  return -EINVAL;
}


SYSCALL(l_getpgid)
{
  return getpgid(ebx);
}


SYSCALL(l_getppid)
{
  return getppid();
}


SYSCALL(l_getgroups)
{
  return getgroups(ebx, (gid_t*)  ecx);  
}


SYSCALL(l_gettimeofday)
{
  return gettimeofday((struct timeval *)ebx, (struct timezone*)ecx); 
}


SYSCALL(l_setpgid)
{
  return setpgid(ebx, ecx);
}


SYSCALL(l_chroot)
{
  return chroot((char*)ebx);
}


SYSCALL(l_setsid)
{
  return setsid();
}


SYSCALL(l_getrusage)
{
  int ret;
  
  ret = getrusage(ebx, (struct rusage*)ecx);
  if (ret < 0) return -errno;
  
  return ret;  
}


SYSCALL(l_setpriority)
{
  /* ignore setpriority */
  return 0;	
}


SYSCALL(l_nanosleep)
{
  struct timespec *t = (struct timespec *)ebx;
  
  SleepEx(t->tv_sec * 1000 + t->tv_nsec / 1000000, FALSE);
  return 0;
}


#if 0
SYSCALL(l_setitimer)
{
  int ret;
  
  ret = setitimer(ebx, (struct itimerval*)ecx, (struct itimerval*)edx);
  if (ret < 0) return -errno;
  
  return ret;
}


SYSCALL(l_getitimer)
{
  int ret;
  
  ret = getitimer(ebx, (struct itimerval*)ecx); 
  if (ret < 0) return -errno;
  
  return ret;
}

#endif
