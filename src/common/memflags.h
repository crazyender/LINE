/**
 * Valid values for the elements of the ProcessInfo.fixedMem array
 * $Id: memflags.h,v 1.1 2001/03/23 19:09:52 mvines Exp $
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
#ifndef __MEMFLAGS_H__ 
#define __MEMFLAGS_H__ 
 

/* This 64K region is in use by somebody else (maybe a system DLL) */
#define MEMFLAG_UNUSABLE -1

/* free region */
#define MEMFLAG_FREE 0

/* in use by the application */
#define MEMFLAG_INUSE 1

/* used for MAP_ANONYMOUS on WinNT/2000, this 64K chunk will not be
   allocated until the program tries to use it. */
#define MEMFLAG_RESERVED 2


#define MEM_INUSE(chunk) (chunk > MEMFLAG_FREE)

#endif
