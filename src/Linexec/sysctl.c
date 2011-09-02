/**
 * $Id: sysctl.c,v 1.2 2001/03/25 21:15:27 mvines Exp $
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
 
#include <stdio.h>

#include "linexec.h"
#include "log.h"
#include "errno.h"
#include "syscall_impl.h"
#include "linux_sysctl.h"


static int do_sysctl(int *name, int nlen, void *oldval, size_t *oldlenp,
                     void *newval, size_t newlen) 
{
  int retval = -ENOTDIR;

  /* TODO! much more goes here */
  do {
    if (2 == nlen) {
      if (name[0] == CTL_KERN && name[1] == KERN_OSRELEASE) {
        if (*oldlenp <= strlen(UNAME_LINUX_RELEASE)) {
          retval = -ENOMEM;
        } else {  
          strcpy(oldval, UNAME_LINUX_RELEASE);
          retval = 0;
        }  
        break;
      }
    }
  } while(0);
    
  return retval; 
}


SYSCALL(l__sysctl)
{
  struct __sysctl_args *s = (struct __sysctl_args *)ebx;

  return do_sysctl(s->name, s->nlen, s->oldval, s->oldlenp,
                   s->newval, s->newlen);
}

