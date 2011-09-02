/**
 * $Id: fork.c,v 1.1.1.1 2001/03/07 18:34:12 mvines Exp $
 * $Id: fork.c,v 1.2 2011/08/31 10:05:30 Ender Zheng $
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
#include "linexec.h"
#include "syscall.h"
#include "errno.h"
#include <stdio.h>  
#include <unistd.h>
#include "asm.h"
#include "mmap.h"


SYSCALL(l_fork)
{ 
  HANDLE hSem;
  int child_pid;
  char sName[256];
  DWORD dwParentId;
  HANDLE hParent;
  
  dwParentId = GetCurrentProcessId();
  
  snprintf(sName, sizeof(sName), "LinexecFork%d", getpid());
  hSem = CreateSemaphore(NULL, 0, 1, sName);
 
  /* fork cygwin */
  child_pid = fork();
    
  if (child_pid < 0) {
    CloseHandle(hSem);
    return child_pid;
  }
      
  if (child_pid != 0) {
    /* wait for child to copy what it needs from us */
    WaitForSingleObject(hSem, INFINITE);  // TODO! probably want a timeout here!
    CloseHandle(hSem);
    return child_pid;
  } 

  
  
  hParent = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
                        FALSE, dwParentId);
  if (NULL == hParent) {
    //DWORD d = GetLastError();
    MessageBox(0, "Unable to access parent process", 
              "linexec", MB_ICONERROR);
    exit(1);
  }
  
  copy_parent_mem(hParent, -1);
  
  CloseHandle(hParent);
  
  /* let the parent continue */
  hSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, sName);
  if (!ReleaseSemaphore(hSem, 1, NULL)) {
    MessageBox(0, "Unable to release semaphore", "linexc - fork()", MB_ICONERROR);
    exit(1);
  }
  CloseHandle(hSem);
  
  return 0;
}

