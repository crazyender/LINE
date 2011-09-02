/**
 * Native shared object support
 * $Id: nativeso.h,v 1.1.1.1 2001/03/07 18:34:13 mvines Exp $
 * $Id: nativeso.h,v 1.2 2011/08/31 10:05:30 Ender Zheng $
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
 
#ifndef __NATIVESO_H__
#define __NATIVESO_H__


struct nativeso_symtable {
  void (*func)(void);  
  char *name;
};


/* this header file is also used on Linux to build the NativeSO stub library */
#ifdef __CYGWIN__

void nso_setup(char *linexec_exe);
void do_symfixup(struct nativeso_symtable *sym_table, char *module);

#endif

#endif
