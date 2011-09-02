/**
 * Native shared object support
 * $Id: nativeso.c,v 1.4 2001/03/21 19:19:52 mvines Exp $
 * $Id: nativeso.c,v 1.5 2011/08/31 10:05:30 Ender Zheng $
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
 
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <dlfcn.h>

#include "nativeso.h"

#include "log.h"


/* the directory where Linexec tries to load native shared objects from */
static char *nso_dir;
static int nso_dir_len;

void nso_setup(char *linexec_exe)
{
  char *p;
  
  if ('\0' == *linexec_exe) {
    nso_dir = "/nso/"; 
    
  } else {
    nso_dir = strdup(linexec_exe);
    
    p = nso_dir + strlen(nso_dir)-1;
    
    while (p > nso_dir) {
      if ('/' == *p) {
        p++;
        break;
      }
      
      p--;
    }
    
    strcpy(p, "nso/");
  }
    
  nso_dir_len = strlen(nso_dir);
  
  log(LOG_LINEXEC_NSO, "nso_setup(): using nso_dir '%s'", nso_dir);
}


static char *find_nso(char *so)
{
  int len = nso_dir_len + strlen(so) + 4 + 1;  // +4 for the .dll extension
  char *ret;
  
  ret = malloc(len);
  strcpy(ret, nso_dir);
  strcat(ret, so);
  strcat(ret, ".dll");
  
  return ret;
}


static char *line_symname(char *sym)
{
  static char buf[256];
  
  strcpy(buf, "LINE_");
  strncat(buf, sym, sizeof(buf));
  buf[sizeof(buf)-1] = '\0';  
  
  return buf;
}


void do_symfixup(struct nativeso_symtable *sym_table, char *so)
{
  char *nso;
  void *dl;
  void (*sym)(void);
  char *error;
  
  
  if (sizeof(*sym_table) != 8) {
    printf("struct _sym_table is not the right size!\n");
    exit(1); 
  }
  
  nso = find_nso(so);
  
  
  log_debug(LOG_LINEXEC_NSO, "==> NSO fixup request <==");
  log_debug(LOG_LINEXEC_NSO, "shared object: %s", so);
  log_debug(LOG_LINEXEC_NSO, "symbol table: %08lX", (long)sym_table);
  log_debug(LOG_LINEXEC_NSO, "loading from %s", nso);


  dl = dlopen(nso, RTLD_NOW | RTLD_GLOBAL);
  if (NULL == dl) {
    log_error(LOG_LINEXEC_NSO, "unable to dlopen %s (error=%s)", nso, dlerror());
    exit(-1);
  }
  
  while (sym_table->name) {
    log_debug(LOG_LINEXEC_NSO, "loading symbol: %s", sym_table->name);

    sym = dlsym(dl, line_symname(sym_table->name));
    if ((error = dlerror()) != NULL) {
      log_error(LOG_LINEXEC_NSO, "unable to load the symbol %s (error=%s)",  
                                  sym_table->name, error); 
      exit(-1);
    }
    
    sym_table->func = sym;
    sym_table++;
  }
  
  free(nso);
  
  log_debug(LOG_LINEXEC_NSO, "NSO fixup complete");
}
