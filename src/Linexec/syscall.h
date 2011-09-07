/**
 * $Id: syscall.h,v 1.2 2001/03/24 22:50:49 mvines Exp $
 * $Id: syscall.h,v 1.3 2011/08/31 10:05:30 Ender Zheng $
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

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "process.h"

/* from linexec.c */
extern ProcessInfo pInfo;
#define current (&pInfo)


typedef int (*syscall)(int,int,int,int,int,int,int);
#define SYSCALL(syscall_name)  \
            int syscall_name(int eax, int ebx, int ecx, int edx, \
                             int esi, int edi, int ebp, int eax) 

#include "syscall_names.h" 

/**
 * BEWARE: if you touch this structure, you will have to fix the 
 * assembly in dispatch.s.  It assumes that every element
 * in this array is 4 bytes long.
 */
extern syscall syscall_tbl[NUM_SYSCALLS];
#define FILE_NOSISY 1

extern void my_print(const char* fmt, ... );
extern void change_path_to_relative(char* des, char* src);

#endif
