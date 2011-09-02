/**
 * $Id: linexec.h,v 1.3 2001/03/21 16:37:42 mvines Exp $
 * $Id: linexec.h,v 1.4 2011/08/31 10:05:30 Ender Zheng $
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
#ifndef __LINEXEC_H__
#define __LINEXEC_H__

/* what Linexec is pretending to be (for 'uname -r') */
#define UMANE_LINE_SYSNAME "LINE-NT-6.1"
#define UNAME_LINUX_RELEASE "2.6.32-31-generic"
#define UNAME_LINE_VERSION "2011-08-31 10:10"


#ifndef MAX_PATH
#define MAX_PATH 260
#endif


/**
 * Full POSIX path to linexe.exe.  This may be the empty string if
 * we are chroot()ed somewhere and linexec.exe is not the chroot area
 */
extern char linexec_exe[MAX_PATH];


/*
 * Uncomment this to enable "Debug mode".  This lets you run linexec as
 * a standalone process (for debugging the startup code).  When in Debug mode,
 * Linexec will not actually invoke the Linux executable.
 */
//#define __DEBUG__

#endif

