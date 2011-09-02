/**
 * Linexec.  This is the LINE loader/syscall emulator.  It uses the Cygwin API
 *           for most of the syscall functionality.  Linexec should not be 
 *           invoked directly.
 * $Id: linexec.c,v 1.12 2001/04/24 20:59:11 mvines Exp $
 * $Id: linexec.c,v 1.14 2011/08/31 10:05:30 Ender Zheng $
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
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <getopt.h>

#include "linexec.h"

#include "process.h"
#include "errno.h"
#include "syscall.h"
#include "syscall_impl.h"
#include "mmap.h"
#include "page.h"
#include "asm.h"
#include "exec.h"
#include "nativeso.h"
#include "winver.h"
#include "log.h"
#include "version.h"


/* the syscall dispatcher (dispatch.s) */
int syscall_dispatch(void);


void coredump(void);  /* (dispatch.s) */

/* useful info about the currently executing Linux application */
ProcessInfo pInfo;

/* full posix path to linexec.exe */
char linexec_exe[MAX_PATH];

/* full posix path to where we are chrooted too (empty string if not chrooted)*/
char chroot_path[MAX_PATH] = {0,};

void show_help(void)
{
  printf("LINE Program Loader/Syscall Emulator %s\n", line_version_string);
  printf("Copyright (C) 2001  Michael Vines\n"
         "http://line.sourceforge.net/\n\n"
         "This program is distributed in the hope that it will be useful,\n"
         "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
         "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
         "GNU General Public License for more details\n\n");


  printf("Usage: Linexec.exe [options] linux_executable [arg0 arg1 arg2 ...]\n"
         "Options:\n"
         " -f, --force          This option must be specified for Linexec to run.\n"
         "                      it is just a fail safe to ensure that you know what\n"
         "                      you are doing (see the WARNING below)\n"
         " -n, --nodebugger     Specified when Linexec will not be running under the\n"
         "                      LINE debugger.  This assumes that there will be some\n"
         "                      other method for reflecting syscalls back to Linexec\n"
         "                      (perhaps a Windows NT device driver)\n"
         " -p, --pause          Pause for keypress when the program exits normally\n"
         " -l, --linexec=PATH   Specify the full filepath to the Linexec.exe executable\n"
         " -d, --debug          Enable debug mode\n"
         " -c, --chroot=PATH    Change the root directory to the specified path\n"
         " -C, --core           Assume the linux executable is a LINE generated\n" 
         "                      core file.  Any linux executable command line\n"
         "                      arguments that follow the core file are ignored\n"
         " -h, --help           Display this help and exit\n");

  printf("\nWARNING: Linexec is normally not invoked directly!\n");
}


int main(int argc, char *argv[], char *envp[])
{
  int ret;
  int isroot = 1;
  int iscore = 0;
  int forceMe = 1;
  int len;
  int lineDebugger = 0;
  int index=0;

  char** tmpenvp = envp;
  while( *tmpenvp ){
	  char* env = *tmpenvp;
	  if( strstr(env, "PATH=") )
		  *tmpenvp = "PATH=/bin:/usr/bin";
	  tmpenvp++;
  }
  /*
  while (1) {
    int option_index = 0;
    int c;
    
    static struct option long_options[] = {
      {"pause", 0, 0, 'p'},
      {"force", 0, 0, 'f'},
      {"help", 0, 0, 'h'},
      {"debug", 0, 0, 'd'},
      {"nodebugger", 0, 0, 'n'},
      {"chroot", 1, 0, 'c'},
      {"core", 0, 0, 'C'},
      {0,0,0,0}
    };
    
    c = getopt_long(argc, argv, "pfnhdc:C", long_options, &option_index);
    
    if (-1 == c) break;
    
    switch (c) {
    case 'f':
      forceMe = 1;
      break;

    case 'c':
      if (chroot(optarg) != 0) {
        printf("Linexec: unable to chroot: %s\n", optarg);
        return -EINVAL;
      }
      cygwin_conv_to_full_posix_path(optarg, chroot_path);
      chdir ("/");
      
      log(LOG_LINEXEC_MISC, "chroot(%s)", chroot_path);
      break;
      
    case 'h':
      show_help();
      exit(0);
      
    case 'd':
      printf("(debug command line option is currently ignored)\n");
      break;      

    case 'p':
      isroot = 1;
      break;      
      
    case 'C':
      iscore = 1;
      break;
      
    case 'n':
      lineDebugger = 0;
      break;
      
    default:
      printf("Unknown option '%c'\n", c);
      //exit(-1);
    }
      
  }
  */
  optind = 1;
  if (!forceMe || (optind > argc-1)) {
    show_help();
    exit(1);
  }
 
   
  if (iscore && (optind < argc-1)) {
    printf("Note: command line arguments are ignored when loading core\n");
  } 
  
  get_winver(0);
  cygwin_conv_to_full_posix_path(argv[0], linexec_exe);
  for( index = 0 ; index < argc; index++ )
    my_print("[linexec_exe]****argv[%d] = %s\n", index, argv[index]);

  
  if ((len = strlen(chroot_path)) > 0) {
    /* cygwin_conv_to_full_posix_path seems to convert '.' to '/path/.', so
       strip the trailing period */
    if ('.' == chroot_path[len-1]) {
      len--;
      chroot_path[len] = '\0';
    }
      
    if (0 != strncmp(linexec_exe, chroot_path, len)) {
      char *msg = "Linexec.exe is not in the chroot()ed filesystem";

      log_warning(LOG_LINEXEC_MISC, msg);
      linexec_exe[0] = '\0';
    } else {
      if ('/' == chroot_path[len-1]) {
      	len--;
      }
      my_print("[linexec_exe]+++ %s\n", linexec_exe);
      memmove(linexec_exe, linexec_exe+len, strlen(linexec_exe)-len+1);
      my_print("[linexec_exe]--- %s\n", linexec_exe);
    }
  }
  
//  printf("linexec_exe is (%s), chroot is (%s)\n", linexec_exe, chroot_path);
  
  dlfork(FORK_RELOAD);
 
 
  /*
   * Note that the core is loaded before ASM_DISPATCH_SYSCALL.  This is because
   * you probably aren't actaully be running under LINE when loading a coredump
   */
  if (iscore) {
    loadcore(argv[optind]); 
    return -1;
  }
  
 
  log_debug(LOG_LINEXEC_MISC, "Running %s", argv[optind]);

  /*
   * Tell LINE about the Linexec syscall handler.
   */
#ifndef __DEBUG__

  if (lineDebugger && !IsDebuggerPresent()) {
    printf("LINE debugger not detected.  Refusing to continue.\n");
    return -1;  
  }
  
  ASM_DISPATCH_SYSCALL;
#endif

  pInfo.lineDebugger = lineDebugger;
  
  if (isroot) {
    pInfo.root_pid = getpid();
  }	  
  
  nso_setup(linexec_exe);
  if (mmap_setup() < 0) {
    //printf("\n[Press ENTER to exit...]\n");
    //getchar();	
    return -1;
  }
  binfmt_setup();

 
  ASM_SAVE_ESP;
  //printf("optind = %d\n", optind);
  //for( index = 0; index < argc; index++ )
	//  printf("argv[%d] = %s\n", index, argv[index]);
  /* start the executable */
  if (optind >= argc-1) {
    char *fake_argv[optind];
    
    fake_argv[0] = argv[optind];
    fake_argv[1] = NULL;
    
    ret = do_exec(argv[optind], fake_argv, envp);
  } else {
    ret = do_exec(argv[optind], &argv[optind], envp);
  }

  printf("linexec: error running %s: %d\n", argv[optind], ret);
  
  if (isroot) {
    //printf("\n[Press ENTER to exit...]\n");
    //getchar();	
  }
  return ret; 
}
