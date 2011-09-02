/**
 * Internal syscalls that Line/Linexec use to communicate with each other
 * $Id: internal_syscalls.h,v 1.1 2001/03/23 17:26:44 mvines Exp $
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
#ifndef __INTERNAL_SYSCALL_H__ 
#define __INTERNAL_SYSCALL_H__ 
 
#define SYSCALL_LINEXEC_HANDLER 0xDEADBEEF
#define SYSCALL_DUMPMEMORYMAP   0xDEADF00E
#define SYSCALL_COREDUMP        0xDEADF00D
#define SYSCALL_NSO_FIXUP       0xDEADFEED

#endif