/**
 * Signal syscalls
 * $Id: signal.c,v 1.2 2001/03/23 19:07:21 mvines Exp $
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
#include "errno.h"

#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>


SYSCALL(l_sigaction)
{
  return sigaction(ebx, (const struct sigaction*)ecx, (struct sigaction*)edx);
}


SYSCALL(l_kill)
{
  return kill(ebx, ecx);  
}


SYSCALL(l_waitpid)
{
  return waitpid(ebx, (int*)ecx, edx);  
}


SYSCALL(l_sigprocmask)
{
  return sigprocmask(ebx, (const sigset_t *)ecx, (sigset_t *)edx);  
}

SYSCALL(l_rt_sigprocmask)
{
  return sigprocmask(ebx, (const sigset_t*)ecx, (sigset_t*)edx);  
}


SYSCALL(l_rt_sigaction)
{
  return sigaction(ebx, (const struct sigaction *)ecx, (struct sigaction *)edx); 
}

SYSCALL(l_rt_sigsuspend)
{
  return sigsuspend((const sigset_t*)ebx);
}


SYSCALL(l_signal)
{
  return (int)signal(ebx, (void*)ecx); 
}



SYSCALL(l_alarm)
{
  return alarm(ebx); 
}


SYSCALL(l_pause)
{
  return pause();
}


SYSCALL(l_wait4)
{
  return wait4(ebx, (int*)ecx, edx, (struct rusage*)esi);  
}

