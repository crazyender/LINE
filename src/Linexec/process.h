/**
 * $Id: process.h,v 1.4 2001/04/23 21:32:12 mvines Exp $
 * $Id: process.h,v 1.5 2011/08/31 10:05:30 Ender Zheng $
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

#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <windows.h>
#include "page.h"


/* information about the process that is running */
typedef struct { 
  /** 
   * WARNING!   Do not change the order of the syscallSP and userSP fields.
   *            The order is hardcoded in dispatch.s
   */
  DWORD syscallSP; /* kernel stack */
  DWORD userSP;    /* user stack pointer when the syscall occured */

  int root_pid;  /* the original linexec process */
  
  DWORD start_brk, brk, start_code, end_code, end_data, start_stack;
  
  DWORD arg_start, arg_end, env_start, env_end;
  
  int lineDebugger; /* non-zero if this Linexec process is running under the
                       LINE debugger (Line.exe) */
  
  /* 
     Used for managing memory allocation (in 64K chunks), every 64K region
     between PROCESS_BOTTOM - PROCESS_TOP is represented as a byte in this
     array.  See common/memflags.h for a list of all the possible values for
     each 64K byte.  
      
     Note that the bytes in this array from 0 to (PROCESS_BOTTOM >> 16) are 
     unused (yes, I know that really sucks)
   */
  char fixedMem[PROCESS_TOP >> 16];

} ProcessInfo;

#endif
