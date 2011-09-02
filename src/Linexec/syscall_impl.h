/**
 * $Id: syscall_impl.h,v 1.7 2001/03/21 19:20:45 mvines Exp $
 * $Id: syscall_impl.h,v 1.8 2011/08/31 10:05:30 Ender Zheng $
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
 
#ifndef __SYSCALL_IMPL_H__
#define __SYSCALL_IMPL_H__

#include "syscall.h"


SYSCALL(impl_me);    
SYSCALL(not_impl);      
SYSCALL(not_impl_quiet);      
SYSCALL(bad_syscall);

#define l_null bad_syscall
SYSCALL(l_print_num);
SYSCALL(l_exit);
SYSCALL(l_fork);
SYSCALL(l_read);
SYSCALL(l_write);
SYSCALL(l_open);
SYSCALL(l_close);
SYSCALL(l_waitpid);
SYSCALL(l_creat);
SYSCALL(l_link);
SYSCALL(l_unlink);
SYSCALL(l_execve);
SYSCALL(l_chdir);
SYSCALL(l_time);
SYSCALL(l_mknod);
SYSCALL(l_chmod);
SYSCALL(l_lchown);
#define l_break impl_me
#define l_oldstat		impl_me
SYSCALL(l_lseek);
SYSCALL(l_getpid);
#define l_mount		impl_me
#define l_umount		impl_me
SYSCALL(l_setuid);
SYSCALL(l_getuid);
SYSCALL(l_stime);
#define l_ptrace		not_impl
SYSCALL(l_alarm);
#define l_oldfstat		impl_me
SYSCALL(l_pause);
SYSCALL(l_utime);
#define l_stty		not_impl
#define l_gtty		not_impl
SYSCALL(l_access);
#define l_nice not_impl
#define l_ftime		not_impl
SYSCALL(l_sync);
SYSCALL(l_kill);
SYSCALL(l_rename);
SYSCALL(l_mkdir);
SYSCALL(l_rmdir);
SYSCALL(l_dup);
SYSCALL(l_pipe);
#define l_times		impl_me
#define l_prof		not_impl
SYSCALL(l_brk);
SYSCALL(l_setgid);
SYSCALL(l_getgid);
SYSCALL(l_signal);
SYSCALL(l_geteuid);
SYSCALL(l_getegid);
#define l_acct		impl_me
#define l_umount2		impl_me
#define l_lock		not_impl
SYSCALL(l_ioctl);
SYSCALL(l_fcntl);
#define l_mpx		not_impl
SYSCALL(l_setpgid);
#define l_ulimit		not_impl
#define l_oldolduname		impl_me
SYSCALL(l_umask);
SYSCALL(l_chroot);
#define l_ustat		impl_me
SYSCALL(l_dup2);
SYSCALL(l_getppid);
SYSCALL(l_getpgrp);
SYSCALL(l_setsid);
SYSCALL(l_sigaction);
#define l_sgetmask		impl_me
#define l_ssetmask		impl_me
SYSCALL(l_setreuid);
SYSCALL(l_setregid);
#define l_sigsuspend  impl_me
#define l_sigpending  impl_me
#define l_sethostname impl_me
SYSCALL(l_setrlimit);
SYSCALL(l_getrlimit);
SYSCALL(l_getrusage);
SYSCALL(l_gettimeofday);
#define l_settimeofday		impl_me
SYSCALL(l_getgroups);
#define l_setgroups		impl_me
SYSCALL(l_select);
SYSCALL(l_symlink);
#define l_oldlstat		impl_me
SYSCALL(l_readlink);
#define l_uselib		impl_me
#define l_swapon		not_impl
#define l_reboot		not_impl
#define l_readdir not_impl
SYSCALL(l_mmap);
SYSCALL(ll_mmap2);
SYSCALL(l_munmap);
SYSCALL(l_truncate);
SYSCALL(l_ftruncate);
SYSCALL(l_fchmod);
SYSCALL(l_fchown);
#define l_getpriority		impl_me
SYSCALL(l_setpriority);
#define l_profil		not_impl
SYSCALL(l_statfs);
SYSCALL(l_fstatfs);
SYSCALL(l_ioperm);
SYSCALL(l_socketcall);
#define l_syslog		impl_me
#define l_setitimer		impl_me
#define l_getitimer		impl_me
SYSCALL(l_stat);
SYSCALL(l_lstat);
SYSCALL(l_fstat);
#define l_olduname		impl_me
SYSCALL(l_iopl);
#define l_vhangup		impl_me
#define l_idle		not_impl
#define l_vm86old		not_impl
SYSCALL(l_wait4);
#define l_swapoff		not_impl
#define l_sysinfo		impl_me
#define l_ipc		impl_me
SYSCALL(l_fsync);
#define l_sigreturn		impl_me
#define l_clone		impl_me
SYSCALL(l_setdomainname);
SYSCALL(l_uname);
#define l_modify_ldt		not_impl
#define l_adjtimex		impl_me
SYSCALL(l_mprotect);
SYSCALL(l_sigprocmask);
#define l_create_module		not_impl
#define l_init_module	  not_impl	
#define l_delete_module	  not_impl	
#define l_get_kernel_syms		not_impl
SYSCALL(l_quotactl);
SYSCALL(l_getpgid);
SYSCALL(l_fchdir);
SYSCALL(l_bdflush);
#define l_sysfs		impl_me
SYSCALL(l_personality);		
#define l_afs_syscall		not_impl
#define l_setfsuid		impl_me
#define l_setfsgid		impl_me
SYSCALL(l__llseek);
SYSCALL(l_getdents);
SYSCALL(l_newselect);
#define l_flock		impl_me
#define l_msync		impl_me
SYSCALL(l_readv);
SYSCALL(l_writev);
#define l_getsid		impl_me
#define l_fdatasync		impl_me
SYSCALL(l__sysctl);
#define l_mlock		impl_me
#define l_munlock		impl_me
#define l_mlockall		impl_me
#define l_munlockall		impl_me
#define l_sched_setparam		impl_me
#define l_sched_getparam		impl_me
#define l_sched_setscheduler		impl_me
#define l_sched_getscheduler		impl_me
#define l_sched_yield		impl_me
#define l_sched_get_priority_max		impl_me
#define l_sched_get_priority_min		impl_me
#define l_sched_rr_get_interval		impl_me
SYSCALL(l_nanosleep);
SYSCALL(l_mremap);
SYSCALL(l_setresuid);
#define l_getresuid		impl_me
#define l_vm86		not_impl
#define l_query_module		not_impl
SYSCALL(l_poll);
#define l_nfsservctl		impl_me
#define l_setresgid		impl_me
#define l_getresgid		impl_me
#define l_prctl		impl_me
#define l_rt_sigreturn		impl_me
SYSCALL(l_rt_sigaction);
SYSCALL(l_rt_sigprocmask);
#define l_rt_sigpending		impl_me
#define l_rt_sigtimedwait		impl_me
#define l_rt_sigqueueinfo		impl_me
SYSCALL(l_rt_sigsuspend);
#define l_pread		impl_me
#define l_pwrite		impl_me
SYSCALL(l_chown);
SYSCALL(l_getcwd);
#define l_capget		impl_me
#define l_capset		impl_me
#define l_sigaltstack		impl_me
#define l_sendfile		impl_me
#define l_getpmsg		impl_me
#define l_putpmsg		impl_me
/* TODO: cygwin has a real vfork */
#define l_vfork l_fork
SYSCALL(l_stat64);
SYSCALL(l_lstat64);
SYSCALL(l_fstat64);
SYSCALL(ll_set_thread_area);
#define l_unknown not_impl_quiet

#endif
