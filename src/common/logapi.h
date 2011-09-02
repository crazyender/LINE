/**
 * $Id: logapi.h,v 1.2 2001/03/18 19:59:29 mvines Exp $
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
#ifndef __LOGAPI_H__
#define __LOGAPI_H__

struct cooked_log_msg {
  struct log_msg *raw;
 
  unsigned char level;
  unsigned char source;
  unsigned short id;
};


typedef int (*type_handlerLoad)(HKEY hKey);
typedef void (*type_handlerMsg)(struct cooked_log_msg *msg);
typedef void (*type_handlerConfig)(HKEY hKey, int argv, char *argc[]);

#endif

