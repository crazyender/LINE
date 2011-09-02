/**
 * execve() syscall
 * $Id: exec.c,v 1.5 2001/04/23 21:32:12 mvines Exp $
 *
 * This file contains a fair chunk of code from the linux/fs/exec.c
 *
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
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>

#include "linexec.h"
#include "syscall.h"
#include "errno.h"
#include "page.h"
#include "mmap.h"
#include "asm.h"
#include "binfmt/binfmts.h"
#include "cygwin_errno.h"

#include "log.h"

static struct linux_binfmt *formats = (struct linux_binfmt *) NULL;

void binfmt_setup(void)
{
  init_exe_binfmt(); 
  init_elf_binfmt();
  init_script_binfmt();
}


int register_binfmt(struct linux_binfmt * fmt)
{
  struct linux_binfmt ** tmp = &formats;

  if (!fmt)
    return -EINVAL;
  if (fmt->next)
    return -EBUSY;
  while (*tmp) {
    if (fmt == *tmp)
      return -EBUSY;
    tmp = &(*tmp)->next;
  }
  fmt->next = formats;
  formats = fmt;
  return 0;
}



/*
 * cycle the list of binary formats handler, until one recognizes the image
 */
int search_binary_handler(struct linux_binprm *bprm)
{
  int retval=0;
  struct linux_binfmt *fmt;

  for (fmt = formats ; fmt ; fmt = fmt->next) {
    int (*fn)(struct linux_binprm *) = fmt->load_binary;
    
    if (!fn) 
      continue;
      
    retval = fn(bprm);
    
    if (retval != -ENOEXEC) 
      break;
  }
  
  return retval;
}


int read_exec(int fd, unsigned long offset,  char *addr, 
              unsigned long count, int to_kmem)
{
  int ret;
  
  if (offset != lseek(fd, offset, SEEK_SET)) 
    return -errno;
    
  ret = read(fd, addr, count);
  
  return ret;  
}             

    
    
static int count(char ** argv, int max)
{
  int i = 0;

  if (argv != NULL) {
    for (;;) {
      if (!*argv) 
        break;
      
      argv++;
      if (++i > max) return -E2BIG;
    }
  }
  return i;
}


unsigned long copy_strings(int argc, char ** argv,unsigned long *page,
                           unsigned long p)
{
  char *str;
  char *pag;
  long offset_i, bytes_to_copy;
  if ((long)p <= 0)
    return p; /* bullet-proofing */
  while( argc-- > 0){
	  int len;
	  unsigned long pos;
	  str = argv[argc];
	  if( !str )
		  return -EFAULT;
	  len = strlen(str)+1;
	  if (!len || len > p) {  /* EFAULT or E2BIG */
		  return len ? -E2BIG : -EFAULT;
	  }
	  p -= len;
	  memcpy((char*)page+p, str, len);
  }
  return p;
}


unsigned long setup_arg_pages(unsigned long p, struct linux_binprm * bprm)
{
  unsigned long stack_base;
  int i;

#define STACK_SIZE (1024*1024)-1
#define STACK_BASE (STACK_TOP - STACK_SIZE)

  stack_base = STACK_TOP - MAX_ARG_PAGES*PAGE_SIZE;


  i = do_mmap(-1, STACK_BASE, STACK_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC,
              MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, 0);
  if (i != STACK_BASE) {
    printf("error mmaping stack\n");
    exit(1);
  }


  p += stack_base;
  if (bprm->loader)
    bprm->loader += stack_base;
  bprm->exec += stack_base;
  
	memcpy((void*)stack_base, (char*)bprm->page, PAGE_SIZE*MAX_ARG_PAGES);
	stack_base += (PAGE_SIZE*MAX_ARG_PAGES);

  
  return p;
}


void remove_arg_zero(struct linux_binprm *bprm)
{
	if (bprm->argc) {
		unsigned long offset;
		char * page;
		offset = bprm->p;
		//page = (char*)bprm->page[bprm->p/PAGE_SIZE];
		while(bprm->p++,*(page+offset++))
			;
		bprm->argc--;
	}
}

 
 
int do_exec(char *filename, char *argv[], char *envp[])
{
  struct linux_binprm bprm;
  int retval = 0;
  int fd;
  int i;
  char fullfile[MAX_PATH];
  char root_path[MAX_PATH];
  char* slash;
  bprm.p = PAGE_SIZE*MAX_ARG_PAGES-sizeof(void *)*PAGE_SIZE;
  //for (i=0 ; i<MAX_ARG_PAGES ; i++)       /* clear page-table */
  //  bprm.page[i] = 0;
  bprm.page = malloc(PAGE_SIZE*MAX_ARG_PAGES);
  memset( bprm.page, 0, PAGE_SIZE*MAX_ARG_PAGES);

  strcpy(root_path, linexec_exe);
  slash = strrchr(root_path, '/');
  if(slash)
      *slash = '\0';

  if( filename[0] == '/' ){
      strcpy(fullfile, root_path);
      strcat(fullfile, filename);
  }else{
      strcpy(fullfile, filename);
  }
  my_print("[do_exec] filename = %s\n", fullfile);
  fd = open(fullfile, O_RDONLY);
  if (fd < 0){
	  /* append /bin /usr/bin */
      strcpy(fullfile, root_path);
	  strcat(fullfile, "/bin/");
	  strcat(fullfile, filename);
      my_print("[do_exec] filename = %s\n", fullfile);
	  fd = open(fullfile, O_RDONLY);
	  if (fd < 0){
          strcpy(fullfile, root_path);
	      strcat(fullfile, "/usr/bin/");
          strcat(fullfile, filename);
          my_print("[do_exec] filename = %s\n", fullfile);
		  fd = open(fullfile, O_RDONLY);
		  if (fd < 0){
			  return -errno;
		  }else{
			  filename = fullfile;
		  }
	  }else{
		  filename = fullfile;
	  }
	  
  }

  bprm.filename = filename;
  bprm.fd = fd;
  
  bprm.sh_bang = 0;
  
  bprm.loader = 0;
  bprm.exec = 0;
  
  /* these fields aren't used right now, but init them anyways */
  bprm.e_uid = geteuid();
  bprm.e_gid = getegid();
  
  if ((bprm.argc = count(argv, bprm.p / sizeof(void *))) < 0) {
    close(fd);
    return bprm.argc;
  }

  if ((bprm.envc = count(envp, bprm.p / sizeof(void *))) < 0) {
    close(fd);
    return bprm.envc;
  }
  
  bzero(bprm.buf, sizeof(bprm.buf));  
  retval = read(bprm.fd, bprm.buf, sizeof(bprm.buf));
  
  if (retval < 0) retval = -errno;
  if (retval >= 0) {
	bprm.p = copy_strings(1, &bprm.filename, bprm.page, bprm.p);
    bprm.exec = bprm.p;
    bprm.p = copy_strings(bprm.envc,envp,bprm.page,bprm.p);
    bprm.p = copy_strings(bprm.argc,argv,bprm.page,bprm.p);
    if ((long)bprm.p < 0) {
      retval = (long)bprm.p;
    }
  }
  if (retval >= 0) {
	  
    retval = search_binary_handler(&bprm);
    /* only returns on error */
  }
  
  /*for (i=0 ; i<MAX_ARG_PAGES ; i++) {
    if (bprm.page[i]) {
      free((void*)bprm.page[i]);
    }
  }*/
  if( bprm.page )
	  free(bprm.page);
  close(bprm.fd);
  return retval;
}



/*
 * The execve() syscall uses the Cygwin execve() to start another instance
 * of Linexec instead of directly calling do_exec() so that Cygwin can 
 * clean up its own mess 
 */
SYSCALL(l_execve)
{
  char **l_argv = (char**)ecx;
  char **l_envp = (char**)edx;
  char **argv;
  int argc;
  int i;
  int base;
  
  if (linexec_exe[0] == '\0') {
    log_error(LOG_LINEXEC_EXEC, "Unable to execve() because linexec.exe is not in the chroot filesystem");
    return -ENOEXEC;  
  }
  
  argc = count(l_argv, 256); /* 256 is just a random number */
  if (argc < 0) return argc;
  
  argv = malloc(sizeof(char*) * (argc+5));
  
  argv[0] = linexec_exe;
  base = 1;
  //argv[1] = "-f";
 /* 
  if (pInfo.lineDebugger) {
    base = 2;
  } else {
    argv[2] = "-n";  
    base = 3;
    
    argc++;
  }*/
  
  //argv[base++] = (char*)ebx;
  
  i = 0;
  while (i < argc) {
    argv[base+i] = l_argv[i];
    i++;
  }
  argv[base+i] = NULL;
  
  log_debug(LOG_LINEXEC_EXEC, "starting execve()");
  for (i = 0; i < argc; i++) {
    log_debug(LOG_LINEXEC_EXEC, "arg %d == '%s'", i, argv[i]);
  }
  
  my_print("[ender]execv argv[0] = %s, linexec = %s\n", l_argv[0], linexec_exe);

  return execve(linexec_exe, argv, l_envp);
}  
