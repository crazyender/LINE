/**
 * Windows EXE binary format (so LINE apps can exec native Windows apps)
 * $Id: binfmt_exe.c,v 1.1.1.1 2001/03/07 18:34:14 mvines Exp $
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
 
#include <stdlib.h>
#include <unistd.h>
#include "binfmts.h"
#include "../errno.h"
#include "../cygwin_errno.h"


static int load_exe_binary(struct linux_binprm *bprm);


static struct linux_binfmt exe_format = {
	NULL, load_exe_binary, NULL
};


int init_exe_binfmt(void)
{
  return register_binfmt(&exe_format);
}


static int load_exe_binary(struct linux_binprm *bprm)
{
  int result;
  char **argv, **envp;
  unsigned long p;
  int i;
  
  if ('M' != bprm->buf[0] || 'Z' != bprm->buf[1]) 
    return -ENOEXEC;

  argv = malloc(sizeof(char*) * (bprm->argc+1));
  envp = malloc(sizeof(char*) * (bprm->envc+1));
  
  bprm->p = setup_arg_pages(bprm->p, bprm);

  p = bprm->p;
  
  for (i = 0; i < bprm->argc; i++) {
    argv[i] = (char*)p;
    p += strlen((char*)p)+1;
  }
  argv[i] = NULL;
  

  for (i = 0; i < bprm->envc; i++) {
    envp[i] = (char*)p;
    p += strlen((char*)p)+1;
  }
  envp[i] = NULL;

  result = execve(bprm->filename, argv, envp);
  if (result < 0) result = -errno;
  
  free(argv);
  free(envp);
  return result;
}

