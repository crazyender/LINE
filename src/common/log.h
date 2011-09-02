/**
 * $Id: log.h,v 1.6 2001/03/23 17:42:46 mvines Exp $
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
#ifndef __LOG_H__
#define __LOG_H__

#include "logids.h"

#define LOG_MSGSIZE 1024

struct log_msg {
  unsigned long id;       /* message source */
  unsigned long pid;       /* win32 processId that the message is coming from */
  char msg[LOG_MSGSIZE];  /* null terminated string data */
};


/**
 * The id field of log_msg is composed of...
 */

/* log levels */
#define LOG_LEVEL_MASK   0xF0000000
#define LOG_LEVEL_SHIFT  28

#define LOG_ERROR        8
#define LOG_WARN         4
#define LOG_NORMAL       2
#define LOG_VERBOSE      1
#define LOG_DEBUG        0


/* log sources */
#define LOG_SOURCE_MASK   0x0F000000
#define LOG_SOURCE_SHIFT  24

#define LOG_LINE          0x4
#define LOG_LINEXEC       0x2
#define LOG_UNKNOWN       0x1

/* all these bits are belong to us! */
#define LOG_RESERVED_MASK 0x00FF0000

/* log ids */
#define LOG_ID_MASK  0x0000FFFF
#define LOG_ID_SHIFT 0


#ifndef LOG_SOURCE
#error LOG_SOURCE not defined. You probably want either LOG_LINE or LOG_LINEXEC
#endif


// Any log messages that are numerically lower than LOG_MIN_LEVEL will be 
// preprocessed out of the code
#ifndef LOG_MIN_LEVEL
#define LOG_MIN_LEVEL    LOG_DEBUG
#endif


#if (LOG_MIN_LEVEL > LOG_ERROR) 
#define log_err(id, msg...)
#else
#define log_err(id, msg...) __do_log(id, LOG_SOURCE, LOG_ERROR, ##msg)
#endif


#if (LOG_MIN_LEVEL > LOG_WARN) 
#define log_warn(id, msg...)
#else
#define log_warn(id, msg...) __do_log(id, LOG_SOURCE, LOG_WARN, ##msg)
#endif


#if (LOG_MIN_LEVEL > LOG_NORMAL) 
#define log(id, msg...)
#else
#define log(id, msg...) __do_log(id, LOG_SOURCE, LOG_NORMAL, ##msg)
#endif


#if (LOG_MIN_LEVEL > LOG_VERBOSE) 
#define log_verb(id, msg...)
#else
#define log_verb(id, msg...) __do_log(id, LOG_SOURCE, LOG_VERBOSE, ##msg)
#endif


#if (LOG_MIN_LEVEL > LOG_DEBUG) 
#define log_dbg(id, msg...)
#else
#define log_dbg(id, msg...) __do_log(id, LOG_SOURCE, LOG_DEBUG, ##msg)
#endif


#define log_error log_err
#define log_debug log_dbg
#define log_verbose log_verb
#define log_warning log_warn


/* used the macros instead of invoking this function directly */
void __do_log(int id, int source, int level, const char *fmt, ...)   
              __attribute ((format (printf, 4, 5)));


#endif

