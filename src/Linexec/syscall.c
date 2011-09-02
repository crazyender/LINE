/**
 * $Id: syscall.c,v 1.2 2001/03/24 22:50:49 mvines Exp $
 *
 * Copyright (C) 2001  Michael Vines
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
 
#include "syscall.h"
#include "syscall_impl.h"

syscall syscall_tbl[NUM_SYSCALLS] = {
  l_print_num,
  l_exit,
  l_fork,
  l_read,
  l_write,
  l_open,
  l_close,
  l_waitpid,
  l_creat,
  l_link,
  l_unlink,
  l_execve,
  l_chdir,
  l_time,
  l_mknod,
  l_chmod,
  l_lchown,
  l_break,
  l_oldstat,
  l_lseek,
  l_getpid,
  l_mount,
  l_umount,
  l_setuid,
  l_getuid,
  l_stime,
  l_ptrace,
  l_alarm,
  l_oldfstat,
  l_pause,
  l_utime,
  l_stty,
  l_gtty,
  l_access,
  l_nice,
  l_ftime,
  l_sync,
  l_kill,
  l_rename,
  l_mkdir,
  l_rmdir,
  l_dup,
  l_pipe,
  l_times,
  l_prof,
  l_brk,
  l_setgid,
  l_getgid,
  l_signal,
  l_geteuid,
  l_getegid,
  l_acct,
  l_umount2,
  l_lock,
  l_ioctl,
  l_fcntl,
  l_mpx,
  l_setpgid,
  l_ulimit,
  l_oldolduname,
  l_umask,
  l_chroot,
  l_ustat,
  l_dup2,
  l_getppid,
  l_getpgrp,
  l_setsid,
  l_sigaction,
  l_sgetmask,
  l_ssetmask,
  l_setreuid,
  l_setregid,
  l_sigsuspend,
  l_sigpending,
  l_sethostname,
  l_setrlimit,
  l_getrlimit,
  l_getrusage,
  l_gettimeofday,
  l_settimeofday,
  l_getgroups,
  l_setgroups,
  l_select,
  l_symlink,
  l_oldlstat,
  l_readlink,
  l_uselib,
  l_swapon,
  l_reboot,
  l_readdir,
  l_mmap,
  l_munmap,
  l_truncate,
  l_ftruncate,
  l_fchmod,
  l_fchown,
  l_getpriority,
  l_setpriority,
  l_profil,
  l_statfs,
  l_fstatfs,
  l_ioperm,
  l_socketcall,
  l_syslog,
  l_setitimer,
  l_getitimer,
  l_stat,
  l_lstat,
  l_fstat,
  l_olduname,
  l_iopl,
  l_vhangup,
  l_idle,
  l_vm86old,
  l_wait4,
  l_swapoff,
  l_sysinfo,
  l_ipc,
  l_fsync,
  l_sigreturn,
  l_clone,
  l_setdomainname,
  l_uname,
  l_modify_ldt,
  l_adjtimex,
  l_mprotect,
  l_sigprocmask,
  l_create_module,
  l_init_module,
  l_delete_module,
  l_get_kernel_syms,
  l_quotactl,
  l_getpgid,
  l_fchdir,
  l_bdflush,
  l_sysfs,
  l_personality,
  l_afs_syscall,
  l_setfsuid,
  l_setfsgid,
  l__llseek,
  l_getdents,
  l_newselect,
  l_flock,
  l_msync,
  l_readv,
  l_writev,
  l_getsid,
  l_fdatasync,
  l__sysctl,
  l_mlock,
  l_munlock,
  l_mlockall,
  l_munlockall,
  l_sched_setparam,
  l_sched_getparam,
  l_sched_setscheduler,
  l_sched_getscheduler,
  l_sched_yield,
  l_sched_get_priority_max,
  l_sched_get_priority_min,
  l_sched_rr_get_interval,
  l_nanosleep,
  l_mremap,
  l_setresuid,
  l_getresuid,
  l_vm86,
  l_query_module,
  l_poll,
  l_nfsservctl,
  l_setresgid,
  l_getresgid,
  l_prctl,
  l_rt_sigreturn,
  l_rt_sigaction,
  l_rt_sigprocmask,
  l_rt_sigpending,
  l_rt_sigtimedwait,
  l_rt_sigqueueinfo,
  l_rt_sigsuspend,
  l_pread,
  l_pwrite,
  l_chown,
  l_getcwd,
  l_capget,
  l_capset,
  l_sigaltstack,
  l_sendfile,
  l_getpmsg,
  l_putpmsg,
  l_vfork,       //
  l_unknown,
  ll_mmap2,      //192
  l_unknown,     //193
  l_unknown,     //194
  l_unknown,//l_stat64,      //195
  l_unknown,//l_lstat64,     //196
  l_unknown,//l_fstat64,     //197
  l_unknown,     //198
  l_unknown,     //199
  l_unknown,     //200
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,     //205
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,     //210
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,     //215
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,     //220
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,     //225
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,     //230
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,     //235
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,     //240
  l_unknown,
  l_unknown,
  ll_set_thread_area,
  l_unknown,
  l_unknown,     //245
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,     //250
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown,
  l_unknown     //255
};
