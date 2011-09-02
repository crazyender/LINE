/**
 * Keeps track of all the processes that are running under LINE
 * $Id: process_table.h,v 1.1.1.1 2001/03/07 18:34:07 mvines Exp $
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
 
#ifndef __PROCESS_TABLE_H__
#define __PROCESS_TABLE_H__
 
  
HANDLE get_process_handle(int pid);
HANDLE get_thread_handle(int pid, int tid);
int add_thread(int pid, HANDLE hThread, DWORD dwThreadId);
int add_process(HANDLE hProcess, DWORD dwProcessId, 
		HANDLE hThread, DWORD dwThreadId);


void init_process_table(void);
void find_process_info(DWORD threadId, int *pid, int *tid);
void find_process_info_with_pid(DWORD processId, int *pid);
void remove_thread(int pid, int tid);
int remove_processinfo(int pid);

#endif
