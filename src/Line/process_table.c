/**
 * Keeps track of all the processes that are running under LINE
 * $Id: process_table.c,v 1.1.1.1 2001/03/07 18:34:07 mvines Exp $
 *
 * The reason all this is necessary is because Windows only gives 
 * the processId,threadId on a debug event.  But the 
 * process/thread handle is needed to do anything useful.  
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
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "syscall_names.h"


/* hardcoded max number of Linux processses */
#define MAX_SPAWN 64

/* hardcoded max number of threads per process */
#define MAX_THREADS 8


/*
 * TODO: stop hardcoding the number of threads/processes and/or 
 * make the thread/process lookup functions faster than O(n)
 */
typedef struct _thread_info {
  HANDLE hThread;
  DWORD dwThreadId;
  int valid;
} thread_info;


struct _process_info {
  HANDLE hProcess;
  DWORD dwProcessId;
  thread_info thread[MAX_THREADS];
  int valid;
} process_info[MAX_SPAWN];


static int find_free_process(void)
{
  int i;

  for (i = 0; i < MAX_SPAWN; i++) {
    if (!process_info[i].valid) return i;
  } 

  return -1;
}


static int find_free_thread(int pid)
{
  int i;

  for (i = 0; i < MAX_THREADS; i++) {
    if (!process_info[pid].thread[i].valid) return i;
  } 

  return -1;
}


static void init_thread_table(int pid)
{
  int q;

  for (q = 0;  q < MAX_THREADS; q++) {
    process_info[pid].thread[q].valid = 0;
  }
}


HANDLE get_process_handle(int pid)
{
  return process_info[pid].hProcess;
}

HANDLE get_thread_handle(int pid, int tid)
{
  return process_info[pid].thread[tid].hThread;
}


/**
 * Returns -1 on error
 */
int add_thread(int pid, HANDLE hThread, DWORD dwThreadId)
{
  int tid = find_free_thread(pid);
  
  if (tid < 0) return -1;
  
  process_info[pid].thread[tid].hThread = hThread;
  process_info[pid].thread[tid].dwThreadId = dwThreadId;
  process_info[pid].thread[tid].valid = 1;	
  return 0;
}

/**
 * Returns -1 on error
 */
int add_process(HANDLE hProcess, DWORD dwProcessId, 
		HANDLE hThread, DWORD dwThreadId)
{
  int pid;
  
  pid = find_free_process();
  if (pid < 0) return -1;
  
  process_info[pid].hProcess = hProcess;
  process_info[pid].dwProcessId = dwProcessId;
  init_thread_table(pid);
  process_info[pid].valid = 1;
        
  return add_thread(pid, hThread, dwThreadId);
}



/**
 * Returns the internal processid (pid) and threadid (tid) 
 * for the specified Windows threadId
 */
void find_process_info(DWORD threadId, int *pid, int *tid)
{
  int i, q;

  for (i = 0; i < MAX_SPAWN; i++) {
    if (!process_info[i].valid) continue;

    for (q = 0; q < MAX_THREADS; q++) {
      if (threadId == process_info[i].thread[q].dwThreadId) {
        *pid = i;
        *tid = q;
        return;
      }
    }
  }

  printf("Unable to find process associated with thread %08lX\n", threadId);
  exit(1);
}


/**
 * Returns the internal processid (pid) for the specified Windows processId
 */
void find_process_info_with_pid(DWORD processId, int *pid)
{
  int i;

  for (i = 0; i < MAX_SPAWN; i++) {
    if (!process_info[i].valid) continue;

    if (processId == process_info[i].dwProcessId) {
      *pid = i;
      return;
    }
  }

  printf("Unable to find process %08lX\n", processId);
  exit(1);
}


void init_process_table(void)
{
  int i, q;

  for (i = 0; i < MAX_SPAWN; i++) {
    process_info[i].valid = 0;

    for (q = 0;  q < MAX_THREADS; q++) {
      process_info[i].thread[q].valid = 0;
    }
  } 
}


void remove_thread(int pid, int tid)
{
  if (!process_info[pid].valid) {
    printf("WARNING: trying to remove a thread '%d' from invalid process '%d'", tid, pid);
  }

  if (!process_info[pid].thread[tid].valid) {
    printf("WARNING: trying to remove thread '%d' from process '%d', "
            "but it has already been removed", tid, pid);
  }
  
  
  process_info[pid].thread[tid].valid = 0;
}


/**
 * Marks the indicated process as invalid.  If there are no more valid processes, 
 * the function returns 0, otherwise it returns 1
 */
int remove_processinfo(int pid)
{
  int i;

  if (!process_info[pid].valid) {
    printf("WARNING: trying to remove process '%d', "
            "but it has already been removed", pid);
  }
  process_info[pid].valid = 0;

  for (i = 0; i < MAX_SPAWN; i++) {
    if (process_info[i].valid) return 1;
  } 

  return 0;
}

