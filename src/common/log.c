/**
 * $Id: log.c,v 1.3 2001/03/21 16:37:09 mvines Exp $
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
#include "log.h"

static HWND hLINELog = NULL;
static int connect_flag = 1;


void __do_log(int id, int source, int level, const char *fmt, ...)   
{
  va_list args;
  struct log_msg m;
  COPYDATASTRUCT cds;

  va_start(args, fmt);
  vsnprintf(m.msg, sizeof(m.msg), fmt, args);
  va_end(args);
  
  /* output error messages to the console */
  if (level == LOG_ERROR) {
    printf("ERROR: %s\n", m.msg); 
  } else if (level == LOG_WARN) {
    printf("WARNING: %s\n", m.msg); 
  }
  
  
  m.msg[sizeof(m.msg)-1] = '\0';
  m.id = ((id << LOG_ID_SHIFT) & LOG_ID_MASK) |
         ((source << LOG_SOURCE_SHIFT) & LOG_SOURCE_MASK) |
         ((level << LOG_LEVEL_SHIFT) & LOG_LEVEL_MASK);
  m.pid = GetCurrentProcessId();


  if (connect_flag) {
    hLINELog = FindWindow("LINELog", "LINELog");
    if (NULL == hLINELog) {
      //printf("WARNING: Unable to find LINELog Window (error %ld)\n", 
      //       GetLastError());
    }
    connect_flag = 0;
  }


  cds.dwData = 0;
  cds.cbData = sizeof(m);
  cds.lpData = &m;
  
  SendMessage(hLINELog, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
}

