/**
 * Assembly Routines
 * $Id: asm.h,v 1.4 2001/03/23 14:57:33 mvines Exp $
 * $Id: asm.h,v 1.5 2011/08/31 10:05:30 Ender Zheng $
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
#ifndef __ASM_H__
#define __ASM_H__

#include "internal_syscalls.h"

#define ASM_EXEC_JUMP(_r)     \
  asm volatile ("movl %0, %%eax" : : "m" (_r.eip)); \
  asm volatile ("movl %0, %%ebx" : : "m" (_r.ebx)); \
  asm volatile ("movl %0, %%ecx" : : "m" (_r.ecx)); \
  asm volatile ("movl %0, %%edx" : : "m" (_r.edx)); \
  asm volatile ("movl %0, %%esi" : : "m" (_r.esi)); \
  asm volatile ("movl %0, %%edi" : : "m" (_r.edi)); \
  asm volatile ("movl %0, %%esp" : : "m" (_r.esp)); \
  asm volatile ("movl %0, %%ebp" : : "m" (_r.ebp)); \
  asm volatile ("jmp *%eax")
  
       
#define ASM_DISPATCH_SYSCALL  \
  if (_WINNT == winVersion) { \
    asm volatile ("int $0x80" : : "a" (SYSCALL_LINEXEC_HANDLER), \
                                  "b" (syscall_dispatch), "c" (&pInfo)); \
  } else { \
    asm volatile (".byte 0xCD, 0x03" : : "a" (SYSCALL_LINEXEC_HANDLER), \
                                  "b" (syscall_dispatch), "c" (&pInfo)); \
  }
 
#define ASM_SAVE_ESP \
  asm volatile ("movl %%esp, %0" : "=m" (pInfo.syscallSP) );


#define ASM_START_CORE \
  asm volatile ("pushl %0\n" : : "m" (ebp)); \
  asm volatile ("pushl %0\n" : : "m" (edi)); \
  asm volatile ("pushl %0\n" : : "m" (esi)); \
  asm volatile ("pushl %0\n" : : "m" (edx)); \
  asm volatile ("pushl %0\n" : : "m" (ecx)); \
  asm volatile ("pushl %0\n" : : "m" (ebx)); \
  asm volatile ("pushl %0\n" : : "m" (eax)); \
\
  asm volatile ("pop %eax\n"); \
	asm volatile ("pop %ebx\n"); \
	asm volatile ("pop %ecx\n"); \
	asm volatile ("pop %edx\n"); \
	asm volatile ("pop %esi\n"); \
	asm volatile ("pop %edi\n"); \
	asm volatile ("pop %ebp\n"); \
\
  asm volatile ("push %0\n" : : "m" (pInfo.userSP)); \
  asm volatile ("pop %esp\n"); \
\
  asm volatile ("ret\n");


#endif