/**
 * $Id: mmap.h,v 1.3 2001/03/20 18:38:25 mvines Exp $
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

#ifndef __MMAP_H__
#define __MMAP_H__

#include <sys/mman.h>

int mmap_setup(void);
void copy_parent_mem(HANDLE hParent, int fd);

void forceCommit(unsigned long addr, long len);

void dumpMemoryMap(void);
int memoryCheck(void);

void loadcore(char *file);
void do_coredump(int eax, int ebx, int ecx, int edx, int esi, int edi, int ebp);


int do_mmap(int fd, unsigned long addr, unsigned long len,
              unsigned long prot, unsigned long flags, unsigned long off);
int do_munmap(unsigned long base, unsigned long size);


#endif

