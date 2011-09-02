/*
 * linux/fs/binfmt_elf.c
 *
 * These are the functions used to load ELF format executables as used
 * on SVr4 machines.  Information on the format may be found in the book
 * "UNIX SYSTEM V RELEASE 4 Programmers Guide: Ansi C and Programming Support
 * Tools".
 *
 * Copyright 1993, 1994: Eric Youngdale (ericy@cais.com).
 */

/** 
 * Modified heavily for LINE (from linux 2.2.16)
 * $Id: binfmt_elf.c,v 1.2 2001/03/20 20:24:36 mvines Exp $
 */

/* uncomment this define if you want the ELF loader to be noisy */
//#define __VERBOSE__


#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>

#include "../linexec.h"
#include "../process.h"
#include "../syscall.h"
#include "../errno.h"
#include "../page.h"
#include "../mmap.h"
#include "../asm.h"
#include "elf.h"
#include "a.out.h"
#include "binfmts.h"
#include "../cygwin_errno.h"

#include "log.h"


#define DLINFO_ITEMS 13

#define __put_user(val, addr) ( *(unsigned long*)(addr) = (unsigned long)(val))


static int load_elf_binary(struct linux_binprm * bprm);
static int load_elf_library(int fd);


#ifndef elf_addr_t
#define elf_addr_t unsigned long
#define elf_caddr_t char *
#endif


#define ELF_PAGESTART(_v) ((_v) & ~(unsigned long)(ELF_EXEC_PAGESIZE-1))
#define ELF_PAGEOFFSET(_v) ((_v) & (ELF_EXEC_PAGESIZE-1))
#define ELF_PAGEALIGN(_v) (((_v) + ELF_EXEC_PAGESIZE - 1) & ~(ELF_EXEC_PAGESIZE - 1))

static struct linux_binfmt elf_format = {
  NULL, load_elf_binary, load_elf_library
};


static void set_brk(unsigned long start, unsigned long end)
{
  start = ELF_PAGEALIGN(start);
  end = ELF_PAGEALIGN(end);
  if (end <= start)
    return;
    
  do_mmap(-1, start, end - start,
    PROT_READ | PROT_WRITE | PROT_EXEC,
    MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 0);
}


/* We need to explicitly zero any fractional pages
   after the data section (i.e. bss).  This would
   contain the junk from the file that should not
   be in memory */


static void padzero(unsigned long elf_bss)
{
  unsigned long nbyte;

  nbyte = ELF_PAGEOFFSET(elf_bss);
  if (nbyte) {
    nbyte = ELF_EXEC_PAGESIZE - nbyte;
    bzero((void *) elf_bss, nbyte);
  }
}



static elf_addr_t *
create_elf_tables(char *p, int argc, int envc,
                  struct elfhdr * exec,
                  unsigned long load_addr,
                  unsigned long load_bias,
                  unsigned long interp_load_addr, int ibcs)
{
  elf_caddr_t *argv;
  elf_caddr_t *envp;
  elf_addr_t *sp, *csp;
  char *k_platform, *u_platform;
  long hwcap;
  size_t platform_len = 0;

  /*
   * Get hold of platform and hardware capabilities masks for
   * the machine we are running on.  In some cases (Sparc), 
   * this info is impossible to get, in others (i386) it is
   * merely difficult.
   */

  hwcap = ELF_HWCAP;
  k_platform = ELF_PLATFORM;

  if (k_platform) {
    platform_len = strlen(k_platform) + 1;
    u_platform = p - platform_len;
    memcpy(u_platform, k_platform, platform_len);
  } else
    u_platform = p;

  /*
   * Force 16 byte _final_ alignment here for generality.
   * Leave an extra 16 bytes free so that on the PowerPC we
   * can move the aux table up to start on a 16-byte boundary.
   */
  sp = (elf_addr_t *)((~15UL & (unsigned long)(u_platform)) - 16UL);
  csp = sp;
  csp -= ((exec ? DLINFO_ITEMS*2 : 4) + (k_platform ? 2 : 0));
  csp -= envc+1;
  csp -= argc+1;
  csp -= (!ibcs ? 3 : 1);  /* argc itself */
  if ((unsigned long)csp & 15UL)
    sp -= ((unsigned long)csp & 15UL) / sizeof(*sp);


  /*
   * Put the ELF interpreter info on the stack
   */
#define NEW_AUX_ENT(nr, id, val) \
  __put_user ((id), sp+(nr*2)); \
  __put_user ((val), sp+(nr*2+1)); 

  sp -= 2;
  NEW_AUX_ENT(0, AT_NULL, 0);
  if (k_platform) {
    sp -= 2;
    NEW_AUX_ENT(0, AT_PLATFORM, (elf_addr_t)(unsigned long) u_platform);
  }
  sp -= 2;
  NEW_AUX_ENT(0, AT_HWCAP, hwcap);

  if (exec) {
    sp -= 11*2;

    NEW_AUX_ENT(0, AT_PHDR, load_addr + exec->e_phoff);
    NEW_AUX_ENT(1, AT_PHENT, sizeof (struct elf_phdr));
    NEW_AUX_ENT(2, AT_PHNUM, exec->e_phnum);
    NEW_AUX_ENT(3, AT_PAGESZ, ELF_EXEC_PAGESIZE);
    NEW_AUX_ENT(4, AT_BASE, interp_load_addr);
    NEW_AUX_ENT(5, AT_FLAGS, 0);
    NEW_AUX_ENT(6, AT_ENTRY, load_bias + exec->e_entry);
    NEW_AUX_ENT(7, AT_UID, (elf_addr_t) getuid());
    NEW_AUX_ENT(8, AT_EUID, (elf_addr_t) geteuid());
    NEW_AUX_ENT(9, AT_GID, (elf_addr_t) getgid());
    NEW_AUX_ENT(10, AT_EGID, (elf_addr_t) getegid());
  }
#undef NEW_AUX_ENT

  sp -= envc+1;
  envp = (elf_caddr_t *) sp;
  sp -= argc+1;
  argv = (elf_caddr_t *) sp;
  if (!ibcs) {
    __put_user((elf_addr_t)(unsigned long) envp,--sp);
    __put_user((elf_addr_t)(unsigned long) argv,--sp);
  }

  __put_user((elf_addr_t)argc,--sp);
  current->arg_start = (unsigned long) p;
  while (argc-->0) {
    __put_user((elf_caddr_t)(unsigned long)p,argv++);
    p += strlen(p)+1;
  }
  __put_user(NULL, argv);
  current->arg_end = current->env_start = (unsigned long) p;
  while (envc-->0) {
    __put_user((elf_caddr_t)(unsigned long)p,envp++);
    p += strlen(p)+1;
  }
  __put_user(NULL, envp);
  current->env_end = (unsigned long) p;
  return sp;
}



/* This is much more generalized than the library routine read function,
   so we keep this separate.  Technically the library read function
   is only provided so that we can read a.out libraries that have
   an ELF header */

static unsigned long load_elf_interp(struct elfhdr * interp_elf_ex,
                                     int interpreter_fd,
                                     unsigned long *interp_load_addr)
{
  struct elf_phdr *elf_phdata;
  struct elf_phdr *eppnt;
  unsigned long load_addr = 0;
  int load_addr_set = 0;
  unsigned long last_bss = 0, elf_bss = 0;
  unsigned long error = ~0UL;
  int elf_exec_fileno;
  int retval, i, size;

  /* First of all, some simple consistency checks */
  if (interp_elf_ex->e_type != ET_EXEC &&
      interp_elf_ex->e_type != ET_DYN)
    goto out;
  if (!elf_check_arch(interp_elf_ex->e_machine))
    goto out;

  /*
   * If the size of this structure has changed, then punt, since
   * we will be doing the wrong thing.
   */
  if (interp_elf_ex->e_phentsize != sizeof(struct elf_phdr))
    goto out;

  /* Now read in all of the header information */

  if (interp_elf_ex->e_phnum < 1 || interp_elf_ex->e_phnum >
      ELF_EXEC_PAGESIZE / sizeof(struct elf_phdr))
    goto out;
  size = sizeof(struct elf_phdr) * interp_elf_ex->e_phnum;
  elf_phdata = (struct elf_phdr *) malloc(size);
  if (!elf_phdata)
    goto out;

  retval = read_exec(interpreter_fd, interp_elf_ex->e_phoff,
                     (char *) elf_phdata, size, 1);
  error = retval;
  if (retval < 0)
    goto out_free;

  error = ~0UL;
  elf_exec_fileno = dup(interpreter_fd);
  if (elf_exec_fileno < 0)
    goto out_free;
  
  eppnt = elf_phdata;
  
  for (i=0; i<interp_elf_ex->e_phnum; i++, eppnt++) {
    if (eppnt->p_type == PT_LOAD) {
      int elf_type = MAP_PRIVATE; // | MAP_DENYWRITE;
      int elf_prot = 0;
      unsigned long vaddr = 0;
      unsigned long k, map_addr;

      if (eppnt->p_flags & PF_R) elf_prot =  PROT_READ;
      if (eppnt->p_flags & PF_W) elf_prot |= PROT_WRITE;
      if (eppnt->p_flags & PF_X) elf_prot |= PROT_EXEC;
      vaddr = eppnt->p_vaddr;
      
      if (interp_elf_ex->e_type == ET_EXEC || load_addr_set) {
        elf_type |= MAP_FIXED;
      }

#ifdef __VERBOSE__ 
      printf("mapping: %08lX  (load_addr: %08lX, vaddr:%08lX)\n",  
            load_addr + ELF_PAGESTART(vaddr), load_addr, vaddr);
#endif
  

      map_addr = do_mmap(elf_exec_fileno, load_addr + ELF_PAGESTART(vaddr),
                         eppnt->p_filesz + ELF_PAGEOFFSET(eppnt->p_vaddr),
                         elf_prot, elf_type,
                         eppnt->p_offset - ELF_PAGEOFFSET(eppnt->p_vaddr));

#ifdef __VERBOSE__ 
      printf("map_addr: %08lX\n", map_addr);
#endif      
 
      if (map_addr > -1024UL) /* Real error */
        goto out_close;

      if (!load_addr_set && interp_elf_ex->e_type == ET_DYN) {
        load_addr = map_addr - ELF_PAGESTART(vaddr);
        load_addr_set = 1;
      }

      /*
       * Find the end of the file mapping for this phdr, and keep
       * track of the largest address we see for this.
       */
      k = load_addr + eppnt->p_vaddr + eppnt->p_filesz;
      if (k > elf_bss)
        elf_bss = k;

      /*
       * Do the same thing for the memory mapping - between
       * elf_bss and last_bss is the bss section.
       */
      k = load_addr + eppnt->p_memsz + eppnt->p_vaddr;
      if (k > last_bss)
        last_bss = k;
    }
  }

  /* Now use mmap to map the library into memory. */

  /*
   * Now fill out the bss section.  First pad the last page up
   * to the page boundary, and then perform a mmap to make sure
   * that there are zero-mapped pages up to and including the 
   * last bss page.
   */
  padzero(elf_bss);
  elf_bss = ELF_PAGESTART(elf_bss + ELF_EXEC_PAGESIZE - 1); /* What we have mapped so far */

  /* Map the last of the bss segment */
  if (last_bss > elf_bss)
    do_mmap(-1, elf_bss, last_bss - elf_bss, PROT_READ|PROT_WRITE|PROT_EXEC,
            MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS, 0);

  *interp_load_addr = load_addr;
  /*
   * AUDIT: is everything deallocated properly if this happens
   * to be ~0UL? We'd better switch to out-of-band error reporting.
   * Also for a.out.
   */
  error = ((unsigned long) interp_elf_ex->e_entry) + load_addr;

out_close:
  close(elf_exec_fileno);
  
out_free:
  free(elf_phdata);
  
out:
  return error;
}




static unsigned long load_aout_interp(struct exec * interp_ex,
                                      int interpreter_fd)
{
  unsigned long text_data, offset, elf_entry = ~0UL;
  char * addr;
  int retval;
  
  printf("WARNING: load_aout_interp() has not been tested at all!\n");

  current->end_code = interp_ex->a_text;
  text_data = interp_ex->a_text + interp_ex->a_data;
  current->end_data = text_data;
  current->brk = interp_ex->a_bss + text_data;

  switch (N_MAGIC(*interp_ex)) {
  case OMAGIC:
    offset = 32;
    addr = (char *) 0;
    break;
  case ZMAGIC:
  case QMAGIC:
    offset = N_TXTOFF(*interp_ex);
    addr = (char *) N_TXTADDR(*interp_ex);
    break;
  default:
    goto out;
  }

  if ((unsigned long)addr + text_data < text_data)
    goto out;

  do_mmap(-1, 0, text_data, PROT_READ|PROT_WRITE|PROT_EXEC, 
          MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS, 0);
          
  retval = read_exec(interpreter_fd, offset, addr, text_data, 0);
  if (retval < 0)
    goto out;
    
#if 0    
  flush_icache_range((unsigned long)addr,
                     (unsigned long)addr + text_data);
#endif

  do_mmap(-1, ELF_PAGESTART(text_data + ELF_EXEC_PAGESIZE - 1),
          interp_ex->a_bss,  PROT_READ|PROT_WRITE|PROT_EXEC, 
          MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS, 0);
  elf_entry = interp_ex->a_entry;

out:
  return elf_entry;
}


/*
 * These are the functions used to load ELF style executables and shared
 * libraries.  There is no binary dependent code anywhere else.
 */

#define INTERPRETER_NONE 0
#define INTERPRETER_AOUT 1
#define INTERPRETER_ELF 2


static int load_elf_binary(struct linux_binprm *bprm)
{
  struct pt_regs regs; 
  int interpreter_fd = -1;
   unsigned long load_addr = 0, load_bias;
  int load_addr_set = 0;
  char * elf_interpreter = NULL;
  unsigned int interpreter_type = INTERPRETER_NONE;
  unsigned long error;
  struct elf_phdr * elf_ppnt, *elf_phdata;
  unsigned long elf_bss, k, elf_brk;
  int elf_exec_fileno;
  int retval, size, i;
  unsigned long elf_entry, interp_load_addr = 0;
  unsigned long start_code, end_code, end_data;
  struct elfhdr elf_ex;
  struct elfhdr interp_elf_ex;
    struct exec interp_ex;
  char passed_fileno[6];
  
  /* Get the exec-header */
  elf_ex = *((struct elfhdr *) bprm->buf);
  //printf("[ender]here to run elf\n");
  retval = -ENOEXEC;
  /* First of all, some simple consistency checks */
  if (elf_ex.e_ident[0] != 0x7f ||
      strncmp(&elf_ex.e_ident[1], "ELF", 3) != 0)
    goto out;
  //printf("[ender]1\n");
  if (elf_ex.e_type != ET_EXEC && elf_ex.e_type != ET_DYN)
    goto out;
  if (!elf_check_arch(elf_ex.e_machine))
    goto out;
  //printf("[ender]2\n");

  /* Now read in all of the header information */

  if (elf_ex.e_phentsize != sizeof(struct elf_phdr) ||
      elf_ex.e_phnum < 1 ||
      elf_ex.e_phnum > 65536 / sizeof(struct elf_phdr))
    goto out;
  //printf("[ender]3\n");
  retval = -ENOMEM;
  size = elf_ex.e_phentsize * elf_ex.e_phnum;
  elf_phdata = (struct elf_phdr *) malloc(size);
  if (!elf_phdata)
    goto out;

  retval = read_exec(bprm->fd, elf_ex.e_phoff, (char *) elf_phdata, size, 1);
  if (retval < 0)
    goto out_free_ph;
  //printf("[ender]4\n");

  elf_exec_fileno = dup(bprm->fd);
  lseek(elf_exec_fileno, 0, SEEK_SET);

  elf_ppnt = elf_phdata;
  elf_bss = 0;
  elf_brk = 0;

  start_code = ~0UL;
  end_code = 0;
  end_data = 0;

  /* look for interpreter */
  for (i = 0; i < elf_ex.e_phnum; i++) {
    if (elf_ppnt->p_type == PT_INTERP) {
      retval = -ENOEXEC;
        if (elf_interpreter ||
          elf_ppnt->p_filesz < 2 ||
          elf_ppnt->p_filesz > PAGE_SIZE)
        goto out_free_dentry;

      /* This is the program interpreter used for
       * shared libraries - for now assume that this
       * is an a.out format binary
       */

      retval = -ENOMEM;
      elf_interpreter = (char *)malloc(elf_ppnt->p_filesz);
      if (!elf_interpreter)
        goto out_free_file;

      retval = read_exec(bprm->fd, elf_ppnt->p_offset,
             elf_interpreter, elf_ppnt->p_filesz, 1);
      if (retval < 0)
        goto out_free_interp;
      elf_interpreter[elf_ppnt->p_filesz - 1] = 0;
      
#if 0
      /* If the program interpreter is one of these two,
       * then assume an iBCS2 image. Otherwise assume
       * a native linux image.
       */
      if (strcmp(elf_interpreter,"/usr/lib/libc.so.1") == 0 ||
          strcmp(elf_interpreter,"/usr/lib/ld.so.1") == 0)
        ibcs2_interpreter = 1;
#endif

      log_debug(LOG_LINEXEC_EXEC, "Using ELF interpreter: %s", elf_interpreter);
	  if( elf_interpreter[0] == '/'){
		  char tmp [MAX_PATH];
          change_path_to_relative(tmp, elf_interpreter);
		  free(elf_interpreter);
		  //elf_interpreter = (char *)malloc(elf_ppnt->p_filesz);
          elf_interpreter = (char *)malloc(strlen(tmp)+1);
		  if (!elf_interpreter)
			  goto out_free_file;
		  strcpy(elf_interpreter, tmp);
	  }
      interpreter_fd = open(elf_interpreter, O_RDONLY);

      if (interpreter_fd < 0) {
        retval = -errno;
        goto out_free_interp;
      }

#if 0        
      retval = permission(interpreter_dentry->d_inode, MAY_EXEC);
      if (retval < 0)
        goto out_free_dentry;
#endif
        
      retval = read_exec(interpreter_fd, 0, bprm->buf, 128, 1);
      if (retval < 0)
        goto out_free_dentry;

      /* Get the exec headers */
      interp_ex = *((struct exec *) bprm->buf);
      interp_elf_ex = *((struct elfhdr *) bprm->buf);
    }
    elf_ppnt++;
	//printf("[ender]6\n");
  }

  /* Some simple consistency checks for the interpreter */
  if (elf_interpreter) {
    interpreter_type = INTERPRETER_ELF | INTERPRETER_AOUT;

    /* Now figure out which format our binary is */
    if ((N_MAGIC(interp_ex) != OMAGIC) &&
        (N_MAGIC(interp_ex) != ZMAGIC) &&
        (N_MAGIC(interp_ex) != QMAGIC))
      interpreter_type = INTERPRETER_ELF;

    if (interp_elf_ex.e_ident[0] != 0x7f ||
        strncmp(&interp_elf_ex.e_ident[1], "ELF", 3) != 0)
      interpreter_type &= ~INTERPRETER_ELF;

    retval = -ELIBBAD;
    if (!interpreter_type)
      goto out_free_dentry;

    /* Make sure only one type was selected */
    if ((interpreter_type & INTERPRETER_ELF) &&
         interpreter_type != INTERPRETER_ELF) {
      printf("ELF: Ambiguous type, using ELF\n");
      interpreter_type = INTERPRETER_ELF;
    }
  }
  //printf("[ender]7\n");
  /* OK, we are done with that, now set up the arg stuff,
     and then start this sucker up */
  if (!bprm->sh_bang) {
    char * passed_p;

    if (interpreter_type == INTERPRETER_AOUT) {
      sprintf(passed_fileno, "%d", elf_exec_fileno);
      passed_p = passed_fileno;

      if (elf_interpreter) {
        bprm->p = copy_strings(1,&passed_p,bprm->page,bprm->p);
        bprm->argc++;
      }
    }
    retval = -E2BIG;
    if (!bprm->p)
      goto out_free_dentry;
  }


#if 0
  /* Flush all traces of the currently running executable */
  retval = flush_old_exec(bprm);
  if (retval)
    goto out_free_dentry;
#endif  

  /* OK, This is the point of no return */
  current->end_data = 0;
  current->end_code = 0;
#if 0
  current->mm->mmap = NULL;
  current->flags &= ~PF_FORKNOEXEC;

#endif  
  elf_entry = (unsigned long) elf_ex.e_entry;
  //printf("[ender]8\n");

#if 0
  /* Do this immediately, since STACK_TOP as used in setup_arg_pages
     may depend on the personality.  */
  SET_PERSONALITY(elf_ex, ibcs2_interpreter);
#endif  

  /* Do this so that we can load the interpreter, if need be.  We will
     change some of these later */
//  current->mm->rss = 0;
  bprm->p = setup_arg_pages(bprm->p, bprm);
  
  current->start_stack = bprm->p;

  /* Try and get dynamic programs out of the way of the default mmap
     base, as well as whatever program they might try to exec.  This
     is because the brk will follow the loader, and is not movable.  */

  load_bias = ELF_PAGESTART(elf_ex.e_type==ET_DYN ? ELF_ET_DYN_BASE : 0);
#ifdef __VERBOSE__
  printf("load_bias: %08lX\n", load_bias);
#endif

  /* Now we do a little grungy work by mmaping the ELF image into
     the correct location in memory.  At this point, we assume that
     the image should be loaded at fixed address, not at a variable
     address. */

  for(i = 0, elf_ppnt = elf_phdata; i < elf_ex.e_phnum; i++, elf_ppnt++) {
    int elf_prot = 0, elf_flags;
    unsigned long vaddr;

    if (elf_ppnt->p_type != PT_LOAD)
      continue;

    if (elf_ppnt->p_flags & PF_R) elf_prot |= PROT_READ;
    if (elf_ppnt->p_flags & PF_W) elf_prot |= PROT_WRITE;
    if (elf_ppnt->p_flags & PF_X) elf_prot |= PROT_EXEC;

    elf_flags = MAP_PRIVATE; // |MAP_DENYWRITE|MAP_EXECUTABLE;

    vaddr = elf_ppnt->p_vaddr;
    if (elf_ex.e_type == ET_EXEC || load_addr_set) {
      elf_flags |= MAP_FIXED;
    }
	//printf("[ender]9\n");
#ifdef __VERBOSE__
    printf("mapping: %08lX\n", ELF_PAGESTART(load_bias + vaddr));
#endif
    error = do_mmap(bprm->fd, ELF_PAGESTART(load_bias + vaddr),
                    (elf_ppnt->p_filesz + ELF_PAGEOFFSET(elf_ppnt->p_vaddr)),
                    elf_prot, elf_flags, 
                    (elf_ppnt->p_offset - ELF_PAGEOFFSET(elf_ppnt->p_vaddr)));

#ifdef __VERBOSE__
    printf("error: %08lX\n", error);
#endif

    if (!load_addr_set) {
      load_addr_set = 1;
      load_addr = (elf_ppnt->p_vaddr - elf_ppnt->p_offset);
#ifdef __VERBOSE__
      printf("load_addr: %08lX, vaddr: %08lX\n", load_addr, vaddr);
#endif      
      if (elf_ex.e_type == ET_DYN) {
        load_bias += error - ELF_PAGESTART(load_bias + vaddr);
        load_addr += error;
      
#ifdef __VERBOSE__
        printf("new\nload_bias: %08lX, load_addr: %08lX\n", load_bias, load_addr);
#endif        
      }
    }
    k = elf_ppnt->p_vaddr;
    if (k < start_code) start_code = k;
    k = elf_ppnt->p_vaddr + elf_ppnt->p_filesz;
    if (k > elf_bss)
      elf_bss = k;
    if ((elf_ppnt->p_flags & PF_X) && end_code <  k)
      end_code = k;
    if (end_data < k)
      end_data = k;
    k = elf_ppnt->p_vaddr + elf_ppnt->p_memsz;
    if (k > elf_brk)
      elf_brk = k;
  }
  
  close(bprm->fd);
  
  elf_entry += load_bias;
  elf_bss += load_bias;
  elf_brk += load_bias;
  start_code += load_bias;
  end_code += load_bias;
  end_data += load_bias;

  if (elf_interpreter) {
    if (interpreter_type == INTERPRETER_AOUT) {
      elf_entry = load_aout_interp(&interp_ex, interpreter_fd);
    } else {
      elf_entry = load_elf_interp(&interp_elf_ex, interpreter_fd, 
                                  &interp_load_addr);
    }                                  
    close(interpreter_fd);
    if (elf_entry == ~0UL) {
      printf("Unable to load interpreter %.128s\n", elf_interpreter);
      free(elf_interpreter);
      free(elf_phdata);
      
      //send_sig(SIGSEGV, current, 0);
      exit(1);
      return 0;
    }

    free(elf_interpreter);
  }
  
  free(elf_phdata);

  if (interpreter_type != INTERPRETER_AOUT)
    close(elf_exec_fileno);
  
#if 0
#ifndef VM_STACK_FLAGS
  current->executable = dget(bprm->dentry);
#endif
#endif
  bprm->p = (unsigned long)create_elf_tables((char *)bprm->p,
                        bprm->argc, bprm->envc,
                        (interpreter_type == INTERPRETER_ELF ? &elf_ex : NULL),
                        load_addr, load_bias, interp_load_addr,
                        (interpreter_type == INTERPRETER_AOUT ? 0 : 1));

#if 0                        
  /* N.B. passed_fileno might not be initialized? */
  if (interpreter_type == INTERPRETER_AOUT)
    current->arg_start += strlen(passed_fileno) + 1;
#endif  
    
  current->start_brk = current->brk = elf_brk;
  current->end_code = end_code;
  current->start_code = start_code;
  current->end_data = end_data;
  current->start_stack = bprm->p;

  /* Calling set_brk effectively mmaps the pages that we need
   * for the bss and break sections
   */
  set_brk(elf_bss, elf_brk);
  padzero(elf_bss);
  log_debug(LOG_LINEXEC_EXEC,"start_brk: %lx" , current->start_brk);
  log_debug(LOG_LINEXEC_EXEC,"end_code: %lx" , current->end_code);
  log_debug(LOG_LINEXEC_EXEC,"start_code: %lx" , current->start_code);
  log_debug(LOG_LINEXEC_EXEC,"end_data: %lx" , current->end_data);
  log_debug(LOG_LINEXEC_EXEC,"start_stack: %lx" , current->start_stack);
  log_debug(LOG_LINEXEC_EXEC,"brk: %lx" , current->brk);

  /*
   * The ABI may specify that certain registers be set up in special
   * ways (on i386 %edx is the address of a DT_FINI function, for
   * example.  This macro performs whatever initialization to
   * the regs structure is required.
   */
  ELF_PLAT_INIT((&regs));

  regs.eip = elf_entry;
  regs.esp = bprm->p;

#if 0
  if (current->flags & PF_PTRACED)
    send_sig(SIGTRAP, current, 0);
#endif

#ifndef __DEBUG__


//  dumpMemoryMap();
  log_verbose(LOG_LINEXEC_EXEC, "[transfering control to Linux executable]");
  //getchar();
  //printf("[ender]11\n");
  ASM_EXEC_JUMP(regs);
  
  printf("You should never see this message!\n");
 
#else

  printf("execve() finished, but in debug mode. exiting...\n");
 
#endif
    
  retval = 0;
out:
  return retval;

  /* error cleanup */
out_free_dentry:
  close(interpreter_fd);
  
out_free_interp:
  if (elf_interpreter) {
    free(elf_interpreter);
  }

out_free_file:
  close(elf_exec_fileno);
  
out_free_ph:
  free(elf_phdata);
  goto out;
}




/* This is really simpleminded and specialized - we are loading an
   a.out library that is given an ELF header. */

static int load_elf_library(int fd)
{
  /* uselib syscall isn't implemented so we don't need this function 
     just yet... */
  return -EINVAL;
  
#if 0
  struct file * file;
  struct dentry * dentry;
  struct inode * inode;
  struct elf_phdr *elf_phdata;
  unsigned long elf_bss = 0, bss, len, k;
  int retval, error, i, j;
  struct elfhdr elf_ex;
  loff_t offset = 0;

  error = -EACCES;
  file = fget(fd);
  if (!file || !file->f_op)
    goto out;
  dentry = file->f_dentry;
  inode = dentry->d_inode;

  /* seek to the beginning of the file */
  error = -ENOEXEC;

  /* N.B. save current DS?? */
  set_fs(KERNEL_DS);
  retval = file->f_op->read(file, (char *) &elf_ex, sizeof(elf_ex), &offset);
  set_fs(USER_DS);
  if (retval != sizeof(elf_ex))
    goto out_putf;

  if (elf_ex.e_ident[0] != 0x7f ||
      strncmp(&elf_ex.e_ident[1], "ELF", 3) != 0)
    goto out_putf;

  /* First of all, some simple consistency checks */
  if (elf_ex.e_type != ET_EXEC || elf_ex.e_phnum > 2 ||
     !elf_check_arch(elf_ex.e_machine) ||
     (!inode->i_op || !inode->i_op->default_file_ops->mmap))
    goto out_putf;

  /* Now read in all of the header information */

  j = sizeof(struct elf_phdr) * elf_ex.e_phnum;
  if (j > ELF_EXEC_PAGESIZE)
    goto out_putf;

  error = -ENOMEM;
  elf_phdata = (struct elf_phdr *) kmalloc(j, GFP_KERNEL);
  if (!elf_phdata)
    goto out_putf;

  /* N.B. check for error return?? */
  retval = read_exec(dentry, elf_ex.e_phoff, (char *) elf_phdata,
         sizeof(struct elf_phdr) * elf_ex.e_phnum, 1);

  error = -ENOEXEC;
  for (j = 0, i = 0; i<elf_ex.e_phnum; i++)
    if ((elf_phdata + i)->p_type == PT_LOAD) j++;
  if (j != 1)
    goto out_free_ph;

  while (elf_phdata->p_type != PT_LOAD) elf_phdata++;

  /* Now use mmap to map the library into memory. */
  error = do_mmap(file,
      ELF_PAGESTART(elf_phdata->p_vaddr),
      (elf_phdata->p_filesz +
       ELF_PAGEOFFSET(elf_phdata->p_vaddr)),
      PROT_READ | PROT_WRITE | PROT_EXEC,
      MAP_FIXED | MAP_PRIVATE | MAP_DENYWRITE,
      (elf_phdata->p_offset -
       ELF_PAGEOFFSET(elf_phdata->p_vaddr)));
  if (error != ELF_PAGESTART(elf_phdata->p_vaddr))
    goto out_free_ph;

  k = elf_phdata->p_vaddr + elf_phdata->p_filesz;
  if (k > elf_bss)
    elf_bss = k;
  padzero(elf_bss);

  len = ELF_PAGESTART(elf_phdata->p_filesz + elf_phdata->p_vaddr + 
        ELF_EXEC_PAGESIZE - 1);
  bss = elf_phdata->p_memsz + elf_phdata->p_vaddr;
  if (bss > len)
    do_mmap(NULL, len, bss - len,
      PROT_READ|PROT_WRITE|PROT_EXEC,
      MAP_FIXED|MAP_PRIVATE, 0);
  error = 0;

out_free_ph:
  kfree(elf_phdata);
out_putf:
  fput(file);
out:
  return error;
#endif  
}



int init_elf_binfmt(void)
{
  return register_binfmt(&elf_format);
}

