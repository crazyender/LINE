/**
 * List of all known log_msg ids 
 * $Id: logids.h,v 1.9 2001/04/25 18:33:24 mvines Exp $
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
#ifndef __LOGIDS_H__
#define __LOGIDS_H__


enum {
  /*
   * Line log IDs
   */
  LOG_LINE_MISC = 0,
  LOG_LINE_DBGEVENT,
  LOG_LINE_EVENTNAME,
  LOG_LINE_DBGSTRING,
  LOG_LINE_DLLINFO,
  LOG_LINE_PROCESSEXIT,
  LOG_LINE_SYSCALL,
  LOG_LINE_LINEXECINFO,
  LOG_LINE_BREAKPOINT,
  LOG_LINE_COMMITMEM,
  LOG_LINE_INT80DRIVER,
  
  /*
   * Linexec log IDs go here
   */  
  LOG_LINEXEC_MISC,
  LOG_LINEXEC_EXEC, /* messages coming from exec.c */
  LOG_LINEXEC_MMAP, /* messages coming from mmap.c */
  LOG_LINEXEC_GETDENTS,
  LOG_LINEXEC_FILEIO, 
  LOG_LINEXEC_UID, 
  LOG_LINEXEC_SOCKET,
  LOG_LINEXEC_NSO,
  
  
  LOG_ID_COUNT
};


#endif
