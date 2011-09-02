/**
 * mmap() and friends
 * $Id: mmap.c,v 1.11 2001/04/23 21:32:12 mvines Exp $
 * $Id: mmap.c,v 1.12 2011/08/31 10:05:30 Ender Zheng $
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
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "linexec.h"
#include "syscall.h"
#include "mmap.h"
#include "page.h"
#include "errno.h"
#include "cygwin_errno.h"
#include "asm.h"
#include "winver.h"

#include "log.h"

#include "memflags.h"
  
  
/*
 * This is required for Windows 9x systems because they roll over and die when
 * a "int 0x80" is executed (ie. Linux syscall request).  The kludge I've come
 * up with is to rewrite them into "int 0x3" (ie. breakpoints).  I use the 
 * long form (0x03cd) instead of the short form (0xcc) that a real debugger 
 * would likely use.  
 *
 * Note that this function is buggy because it doesn't take immediate 
 * values into consideration.  An instruction like "movl 0x80cd, %eax" 
 * will get rewritten to "movl 0x03cd, %eax".   But this is only a problem
 * for Windows 9x systems.
 */
static void fixupCode(unsigned char *base, long len)
{
  int state = 0;
 
  log_debug(LOG_LINEXEC_MMAP, "Rewriting 'int 0x80' to 'int 0x03' in "
            "memory region %08lX-%08lX",  (unsigned long)base, 
            (unsigned long)base+len);
  
  while (len >= 0) {
    switch (state) {
    case 1:
      if (0x80 == *base) {
        *base = 0x03;
      }
      state = 0;
      break;
      
    case 0:
    default:
      if (0xcd == *base) {
        state = 1;
      }
      break;
    }
    
    len--;
    base++;
  }
}


/**
 * This function outputs a memory map for the currently running process.
 *
 * useful during debugging 
 */
void dumpMemoryMap()
{
   DWORD allocMem = 0;
   MEMORY_BASIC_INFORMATION allocInfo;
   DWORD allocRet;
   int i = 0;

   printf("Process Memory Map:\n");
   while (1) {
     allocRet = VirtualQuery((void*)allocMem, &allocInfo, sizeof(allocInfo));
     
     if (allocRet <= 0) break;

     if (++i > 20) {
     	i = 0;
     	//printf("Press ENTER to continue...\n");
     	//getchar();
     }

     printf("%08lX-%08lX (base %08lX) ",
       (DWORD)allocInfo.BaseAddress,          // base address of region 
       (DWORD)allocInfo.BaseAddress + allocInfo.RegionSize,           // size, in bytes, of region 
       (DWORD)allocInfo.AllocationBase       // allocation base address 
     );
     
     
     switch (allocInfo.State) {
     case MEM_COMMIT:
       printf(" COMMIT");
       break;
     case MEM_FREE:
       printf("   FREE");
       break;
     case MEM_RESERVE:
       printf("RESERVE");
       break;
     default:
       printf("%7ld", allocInfo.State);
       break;
     }
     printf(", prot %5ld/%5ld, ",
       allocInfo.AllocationProtect,    // initial access protection 
       allocInfo.Protect);              // current access protection 
     
     
     switch (allocInfo.Type) {
     case MEM_IMAGE:
       printf("  IMAGE");
       break;
     case MEM_MAPPED:
       printf(" MAPPED");
       break;
     case MEM_PRIVATE:
       printf("PRIVATE");
       break;
     default:
       printf("%7ld", allocInfo.Type);
       break;
     }
     
     printf("\n");
     
       
     allocMem = (DWORD)allocInfo.BaseAddress + allocInfo.RegionSize;       
   }
        
}    


/*
 * Performs a couple simple checks on LINE's memory table.  
 *
 * It is REALLY slow!
 */
int memoryCheck(void)
{
  int i;
  
  /* memory < PROCESS_BOTTOM should be unusable */
  for (i = 0; i < (PROCESS_BOTTOM>>16); i++) {
    if (pInfo.fixedMem[i] != MEMFLAG_UNUSABLE) {
      printf("memoryCheck() FAILED:" 
              "%08X < PROCESS_BOTTOM and is not marked unusable\n", (i << 16));
      return -1; 
    }
  }


  for (; i < (PROCESS_TOP>>16); i++) {
    DWORD allocMem = (i << 16);
    MEMORY_BASIC_INFORMATION allocInfo;
    DWORD ret;
  
    //printf("querying %08lX\n", allocMem);
    ret = VirtualQuery((void*)allocMem, &allocInfo, sizeof(allocInfo));
   // printf("ret %ld\n", ret);
  
  
#if 1
    printf("addr:%08lX allocbase:%08lX "
           "size:%08lX state:%ld type:%ld\n",
      (DWORD)allocInfo.BaseAddress,          // base address of region 
      (DWORD)allocInfo.AllocationBase,       // allocation base address 
      allocInfo.RegionSize,           // size, in bytes, of region 
      allocInfo.State,                // committed, reserved, free 
      allocInfo.Type);
#endif      

    if (pInfo.fixedMem[i] == MEMFLAG_FREE) {
      if (allocInfo.State != MEM_RESERVE) {
        printf("memoryCheck() FAILED:" 
               "Windows says that %08X is not reserved, but LINE says that it is\n", 
               (i <<16));
        return -1;
      }
    } else if (pInfo.fixedMem[i] != MEMFLAG_INUSE &&
               pInfo.fixedMem[i] != MEMFLAG_RESERVED) {
      if (allocInfo.Type != MEM_PRIVATE) {
        printf("memoryCheck() FAILED:" 
               "Windows says that %08X is not MEM_PRIVATE, but LINE says that it is\n", 
               (i << 16));
        return -1;
      }
    } 
  }

  printf("memoryCheck(): everything looks ok.\n");
  return 0;
}



#define fixedMemOp_ALLOC(addr, len) __fixedMemOp((addr),(len),1)
#define fixedMemOp_ALLOCANON(addr, len) __fixedMemOp((addr),(len),6)
#define fixedMemOp_FREE(addr, len) __fixedMemOp((addr),(len),2)
#define fixedMemOp_INUSE(addr, len) __fixedMemOp((addr),(len),3)
#define fixedMemOp_MARKBAD(addr, len) __fixedMemOp((addr),(len),4)
#define fixedMemOp_ISFREE(addr, len) __fixedMemOp((addr),(len),5)
#define fixedMemOp_ANYNONCOMMIT(addr, len) __fixedMemOp((addr),(len),7)
#define fixedMemOp_TOUCH(addr, len) __fixedMemOp((addr),(len),8)
 
static int __fixedMemOp(unsigned long addr, long len, int op)
{
  unsigned long org_addr;
  
  if (len < 0) return -EINVAL;
  
  org_addr = addr;
  
  addr >>=16;
  len >>=16;
  
  while (len >= 0) {
    switch (op) {
    /* allocate */
    case 1:
      pInfo.fixedMem[addr] = MEMFLAG_INUSE;
      break;
      
    /* anonymous allocate */
    case 6:
      pInfo.fixedMem[addr] = MEMFLAG_RESERVED;
      break;
      
    /* free */
    case 2:
      pInfo.fixedMem[addr] = MEMFLAG_FREE;
      break;
      
    /* query for inuse */
    case 3:
      if (!MEM_INUSE(pInfo.fixedMem[addr])) return -EINVAL;
      break;
      
    /* mark memory region unusable */
    case 4:      
      pInfo.fixedMem[addr] = MEMFLAG_UNUSABLE;
      break;

    /* query for free */
    case 5:
      if (pInfo.fixedMem[addr] != MEMFLAG_FREE) return -EINVAL;
      break;
      
    /* return 1 if any memory in the region has not been commited */
    case 7:
      if (pInfo.fixedMem[addr] == MEMFLAG_RESERVED) return 1;
      break;
     
    /* do a quick read from every 64k chunk to ensure that it has been commited */
    case 8:
      {
        int idonothing = *(int*)(addr << 16);
      }
      break;
    
    default:
      exit(1);
    }
    
    len--;
    addr++;
  }
  
  return 0;
}


/**
 * This is only used on NT/2000 where anonymous mmap()ed regions are not 
 * committed until they are first used.  This function just makes sure
 * that the specifie region has been commited
 */
void forceCommit(unsigned long addr, long len)
{
  if (fixedMemOp_ANYNONCOMMIT(addr, len)) {
    fixedMemOp_TOUCH(addr, len);
  }
}


/* returns -errno on failure, 0 on success */
static int allocFixedMem(unsigned long addr, long len, int isAnonymous)
{
  DWORD allocAddr, org_addr; 
  long org_len;
  int zeromem = 1;
  
  org_addr = addr;
  org_len = len;
  
//  printf("preallocFixedMem(%08lX, %lX)\n", addr,len);

  len += (addr & 0x0000FFFF);
  addr &= 0xFFFF0000;
  
  len >>= 16;
  len++;
  len <<= 16;
  len--;
//  printf("allocfixedMem(%08lX, %lX)\n", addr,len);
  
  while (1) {
    
    if (pInfo.fixedMem[addr >> 16] == MEMFLAG_UNUSABLE) return -ENOMEM;
    if (pInfo.fixedMem[addr >> 16] == MEMFLAG_FREE) break;
    
    addr += 0x00010000;
    len  -= 0x00010000;
    
    if (len <= 0) {
//      printf("all allocated. returning %08lX\n", org_addr);
//      printf("zeroing %08lX-%08lX\n", org_addr, org_addr+org_len);
      bzero((char*)org_addr, org_len);
      return org_addr;
    }
  }
//  printf("VirtualAllocing...allocFixedMem(%08lX, %lX, %d)\n", addr, 
//         len, isAnonymous);
  if (isAnonymous) {
    if (winVersion != _WINNT || !pInfo.lineDebugger) {
       allocAddr = (DWORD)VirtualAlloc((void*)addr, len, MEM_COMMIT, 
                                       PAGE_EXECUTE_READWRITE);
    } else {
//      printf("anonymous\n");
      zeromem = 0;
      allocAddr = addr;
    }
  } else {
    allocAddr = (DWORD)VirtualAlloc((void*)addr, len, MEM_COMMIT, 
                                    PAGE_EXECUTE_READWRITE);
  }     
  if (allocAddr != addr) {
    DWORD d = GetLastError();
    
    printf("allocFixedMem failed (err %ld), base addr: %08lX\n", d, addr);
    
    dumpMemoryMap();

    MessageBox(0, "VirtualAlloc() failed", "fixMemOp()", 0);
    exit(1);	
  }  
  
  if (zeromem) {
//    printf("zeroing %08lX-%08lX\n", org_addr, org_addr+org_len);
    bzero((char*)org_addr, org_len);

    return fixedMemOp_ALLOC(addr, len);
  }

  return fixedMemOp_ALLOCANON(addr, len);
}


/* returns -errno on failure, 0 on success */
static int freeFixedMem(unsigned long addr, long len)
{
  unsigned long org_addr = addr;
  long org_len = len;
  
  int ret = fixedMemOp_INUSE(addr, len);
  if (ret < 0) return ret;

  len += addr;
  
  /* cannot deallocate the first 64k page if it is not 64k aligned */
  if ((addr & 0x0000FFFF) != 0) {
    addr &= 0xFFFF0000;
    addr += 0x00010000; 
  }
  
  /* same with the last 64k page */
  if ((len & 0x0000FFFF) != 0) {
    len &= 0xFFFF0000;
    len -= 0x00010000; 
  }
  
  /* nothing to do */
  if (len <= addr) {
    return 0;
  }
  
  len -= addr;
  
//  printf("freeing %08lX (for %08lX)\n", addr, len);
  ret = VirtualFree((void*)addr, len, MEM_DECOMMIT);

  if (!ret) {
    DWORD d = GetLastError();
      
    printf("freeFixedMem failed: (err %ld), addr: %08lX\n", d, addr);
    MessageBox(0, "DECOMMIT failed", "freeFixedMem()", 0);
    exit(1);
  }
  
  return fixedMemOp_FREE(org_addr, org_len);
}

/**
 * returns the region type (MEMFLAG_* constant)
 */
static int getMemRegion(ProcessInfo *p, unsigned long *addr, unsigned long *len)
{
  int i;
  int mtype;
  
  if (*addr < PROCESS_BOTTOM) return -1;
  if (*addr >= PROCESS_TOP) return -1;
  
  *len = 1;
  i = *addr >> 16;
  
  mtype = p->fixedMem[i];
  while (i < sizeof(p->fixedMem)) {
    i++;
    if (p->fixedMem[i] != mtype) {
      break;
    }
    
    (*len)++;
  }
  
  *len = ( *len << 16 ) - 1;
  
  return mtype;
}


/**
 * This is used by do_mmap() to find a free region of memory 
 * of the specified size.   The BASE_FREE_MEM define specifies
 * where in the address space the function will start looking
 * for free memory
 */
#define BASE_FREE_MEM 0x20000000
static long findFreeMem(long wanted_len)
{
  unsigned long addr;
  unsigned long len;
  int type;

//  printf("wanted_len is %08lX (%ld)\n", wanted_len, wanted_len);
  if (wanted_len <= 0) return -EINVAL; 

  addr = BASE_FREE_MEM;
  while (addr < PROCESS_TOP) {
  	
    if ((addr & 0x0000FFFF) != 0) {
      printf("addr (%08lX) must be 64k aligned\n", addr);
      exit(1);  
    }
    
    type = getMemRegion(&pInfo, &addr, &len);
//    printf("got region: %08lX (len=%08lX) (type=%d)\n", addr, len, type);
    
    if (MEMFLAG_FREE == type && len >= wanted_len) {
    	return addr;
    }

    addr += (len+1);
  }      

  return -ENOMEM;
}


int mmap_setup(void)
{
  DWORD allocMem;
  MEMORY_BASIC_INFORMATION allocInfo;
  DWORD ret;
  int i;
  
  /* clear memory allocation table */
  for (i = 0; i < sizeof(pInfo.fixedMem); i++) {
    pInfo.fixedMem[i] = MEMFLAG_FREE;
  }
  
  /* mark the memory < PROCESS_BOTTOM as unusable */
  for (i = 0; i < (PROCESS_BOTTOM>>16); i++) {
    pInfo.fixedMem[i] = MEMFLAG_UNUSABLE; 
  }
  
  allocMem = PROCESS_BOTTOM;
  while (allocMem < PROCESS_TOP) {
    
    if ((allocMem & 0x0000FFFF) != 0) {
      printf("mmap_setup: %08lX is not 64K aligned.\n", allocMem);
      return -1; 
    }
    
    ret = VirtualQuery((void*)allocMem, &allocInfo, sizeof(allocInfo));
     
    if (ret <= 0) {
      printf("VirtualQuery(%08lX) failed (error=%ld)!\n", allocMem, GetLastError());
      return -1;
    }

    
    if (allocMem != (DWORD)allocInfo.BaseAddress) {
     printf("mmap_setup() assumes that allocMem == allocInfo.BaseAddress.\n");
     return -1; 
    }
    
    
    do {
      /* memory region is reserved or commited. */
      if (allocInfo.State != MEM_FREE) {
      	int markbad = 1;
        /*         
        Windows98 seems to mark a number of the memory regions that most
        Linux apps need as RESERVED.  So lets just use those regions as well, 
        this probably isn't too wise!!!
        */
        if (allocInfo.State == MEM_RESERVE && allocInfo.Type == MEM_PRIVATE && 
            allocInfo.Protect == PAGE_NOACCESS) {
        
          /* if the base address isn't 64K aligned, it possible that the
             first part of this 64K chunk has been commited so we want
             to mark the entire chunk bad */
          if (((DWORD)allocInfo.BaseAddress & 0x0000FFFF) != 0) {
            allocInfo.RegionSize = 0x00010000 - 
                                  ((DWORD)allocInfo.BaseAddress & 0x0000FFFF);
          } else {
            /* if the region is smaller than 64K it's no good, if it's 
               larger than 64K then we need to make sure that it's 
               64K aligned (so ignore any remainder...which'll be processed
               on the next iteration */
            if (allocInfo.RegionSize > 0x0000FFFF) {
              allocInfo.RegionSize &= 0xFFFF0000;
              markbad = 0;
            }
          }
        }
		/************************************************************************/
		/* 2011.08.21 by crazyender
		/************************************************************************/
		else if( ( ((DWORD)allocInfo.BaseAddress + (DWORD)allocInfo.RegionSize  ) > 0x08040000 ) &&
			(  (DWORD)allocInfo.BaseAddress < 0x08040000 ) )
		{
			// just let it go, elf needs 0x08040000 nowdays
			/* must be 64K aligned */
			allocInfo.RegionSize &= 0xFFFF0000;
			allocInfo.RegionSize += 0x00010000;
			markbad = 0;
		}
        
        if (markbad) { 
      	  //printf("marking region ususable: %08lX-%08lX\n", (DWORD)allocInfo.BaseAddress,
      	  //    (DWORD)allocInfo.BaseAddress + allocInfo.RegionSize);
          ret = fixedMemOp_MARKBAD((DWORD)allocInfo.BaseAddress, 
                                   allocInfo.RegionSize);
          if (ret < 0) return ret;
        
          /* must be 64K aligned */
          allocInfo.RegionSize &= 0xFFFF0000;
          allocInfo.RegionSize += 0x00010000;
        }
        break;
      }
      
      /* make sure we don't reserve past PROCESS_TOP */
      if ((DWORD)allocInfo.BaseAddress + allocInfo.RegionSize > PROCESS_TOP) {
        //printf("adjusting so we don't to past PROCESS_TOP\n");
        allocInfo.RegionSize = PROCESS_TOP - (DWORD)allocInfo.BaseAddress;
      }
      
//      printf("reserving %08lX-%08lX\n", (DWORD)allocInfo.BaseAddress, 
//             (DWORD)allocInfo.BaseAddress + allocInfo.RegionSize);
      
      
      if (allocInfo.State == MEM_RESERVE) break;
      
      ret = (DWORD)VirtualAlloc(allocInfo.BaseAddress, allocInfo.RegionSize,
                           MEM_RESERVE, PAGE_EXECUTE_READWRITE);
      if (ret != (DWORD)allocInfo.BaseAddress) {
        printf("VirtualAlloc(base=%08lX, len=%08lX, MEM_RESERVE) failed: err %ld\n", 
               (DWORD)allocInfo.BaseAddress, allocInfo.RegionSize, GetLastError());
        return -1;
      }

    } while (0);
     
    allocMem = (DWORD)allocInfo.BaseAddress + allocInfo.RegionSize;       
    
  }

  return 0;
}



/**
 * Duplicate the parent's memory map.  This also used by the loadcore()
 * function (ie. if hParent is set to zero, then fd will be used instead)
 */
static ProcessInfo parentInfo; 
void copy_parent_mem(HANDLE hParent, int fd)
{
  DWORD len;
  DWORD addr;
  int type;
  

  parentInfo = pInfo;
  
  /*
   * Do this because can't guarantee (maybe we can???) that the memory map
   * will be _exactly_ like the parent
   */
  mmap_setup();
  
  /**
   * Now duplicate all the inuse memory regions  
   */
  addr = PROCESS_BOTTOM;
  while (addr < PROCESS_TOP) {
    if ((addr & 0x0000FFFF) != 0) {
      printf("fork() error: addr (%08lX) must be 64k aligned\n", addr);
      exit(1);  
    }
    
    type = getMemRegion(&parentInfo, &addr, &len);
    //printf("got region: %08lX (len=%08lX) (type=%d)\n", addr, len, type);
    
    if (MEM_INUSE(type)) {
      if (fixedMemOp_ISFREE(addr, len)) {
        /* this is bad, a LINE allocated memory region in the parent has been
           marked as unusable in the child.  nothing we can do, just exit */
        printf("fork() error: unable to duplicate parent memory map\n");
        exit(1); 
      }
      
      if (allocFixedMem(addr, len, type != 1) < 0) {
        printf("fork() error: unable to allocate memory %08lX-%08lX in child\n", 
               addr, addr + len);
        exit(1); 
      }
      
      
      if ((type != 2) || (winVersion != _WINNT)) {
      
        if (hParent != 0) {
          if (!ReadProcessMemory(hParent, (void*)addr, (void*)addr, len, NULL)) {
            DWORD d = GetLastError();
            
            printf("fork() error: ReadProcessMemory err=%ld, base addr: %08lX\n", 
                   d, (DWORD)addr);
            MessageBox(0, "ReadProcessMemory() failed", "copy_parent_mem()", 0);
            exit(1);
          }
        } else {
          int ret;
          DWORD a, l;
          
          a = addr;
          l = len;
          
          while (l > 0) {
            ret = read(fd, (void*)a, l);
            if (ret <= 0) {
              printf("error reading %ld bytes from fd %d to %08lX (ret=%d, errno=%d)\n", 
                     l, fd, a, ret, errno);
            }
            a += ret;
            l -= ret;
          }
        }
      }        
    }
    addr += (len+1);
  }
} 


/*
 * Output a 'LINE core file' which can then be loaded using the 
 * Linexec /core command line option.
 */
void do_coredump(int eax, int ebx, int ecx, int edx, int esi, int edi, int ebp) 
{
  int fd;
  int ok;
  DWORD len;
  DWORD addr;
  int type;
    
  printf("\n\nDumping core (linecore)...\n");
  
  ok = 0;
  do {
    fd = open("linecore", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);
    if (fd < 0) {
      printf("Unable to create core file: errno %d\n", errno);
      break;
    }

    write(fd, &eax, sizeof(eax));
    write(fd, &ebx, sizeof(ebx));
    write(fd, &ecx, sizeof(ecx));
    write(fd, &edx, sizeof(edx));
    write(fd, &esi, sizeof(esi));
    write(fd, &edi, sizeof(edi));
    write(fd, &ebp, sizeof(ebp));
    write(fd, &pInfo, sizeof(pInfo));
    
    addr = PROCESS_BOTTOM;
    while (addr < PROCESS_TOP) {
      if ((addr & 0x0000FFFF) != 0) {
        printf("addr (%08lX) must be 64k aligned\n", addr);
        exit(1);  
      }
    
      type = getMemRegion(&pInfo, &addr, &len);
      printf("got region: %08lX (len=%08lX) (type=%d)\n", addr, len, type);
    
      if (type == MEMFLAG_INUSE) { /* allocated memory */
        write(fd, (void*)addr, len);
      }

      addr += (len+1);
    }    
      
    close(fd);  
    
    ok = 1;
  } while (0);
  
  if (!ok) {
    printf("Failed.\n");
  }

  //printf("\n\n[Press ENTER to exit]");
  //getchar();
  exit(1);
}



/*
 * Loads a core file that was created by the coredump() function.  
 * This function will not return if it is successful.
 */
void loadcore(char *core)
{
  int eax, ebx, ecx, edx, esi, edi, ebp;
  int fd;
  int ok;
  
  printf("Loading linecore (%s)...\n", core);
    
  ok = 0;
  do {
    fd = open("linecore", O_RDONLY | O_BINARY);
    if (fd < 0) {
      printf("Unable to open core file: errno %d\n", errno);
      break;
    }

    read(fd, &eax, sizeof(eax));
    read(fd, &ebx, sizeof(ebx));
    read(fd, &ecx, sizeof(ecx));
    read(fd, &edx, sizeof(edx));
    read(fd, &esi, sizeof(esi));
    read(fd, &edi, sizeof(edi));
    read(fd, &ebp, sizeof(ebp));
    read(fd, &pInfo, sizeof(pInfo));
    
    copy_parent_mem(0, fd);
      
    close(fd);  
    
    ok = 1;
  } while (0);
  
  if (!ok) {
    printf("Failed.\n");
    exit(1);
  }

  printf("Load successful.\n");
  //printf("\n\n[Press ENTER to transfer control to core]");
  //getchar();
  
  ASM_START_CORE;

  printf("you will never see this message\n");
  exit(1);
}




int do_munmap(unsigned long base, unsigned long size)
{
  log_verbose(LOG_LINEXEC_MMAP, "do_munmap(0x%08lX, 0x%08lX)", base, size);
  if (fixedMemOp_INUSE(base, size) == 0) {
    return freeFixedMem(base, size);
  }
  
  log_debug(LOG_LINEXEC_MMAP, "do_munmmap(): invoking Cygwin munmap()");
  return munmap((void*)base, size);
}



int do_mmap(int fd, unsigned long addr, unsigned long len,
              unsigned long prot, unsigned long flags, unsigned long off)
{ 
  int ret;

  log_verbose(LOG_LINEXEC_MMAP, 
              "do_mmap(addr=0x%lX, len=0x%08lX, fd=%d, prot=%ld, flags=%ld, ofs=%ld)", 
              addr, len, fd, prot, flags, off);

  /* 
   * don't support MAP_SHARED so pass the request along to Cygwin
   */
  if (flags & MAP_SHARED) {
    log_verbose(LOG_LINEXEC_MMAP, "do_mmap(): using Cygwin mmap() for MAP_SHARED...");
    return (int)mmap((void*)addr, len, prot, flags, fd, off);
  }  
  
  /* MAP_FIXED hasn't been specified...find an address */
  if (! (flags & MAP_FIXED)) {
    addr = findFreeMem(len);
    if (addr < 0) {
      return addr;
    }
  }
  ret = allocFixedMem(addr, len, flags & MAP_ANONYMOUS); 
  if (ret < 0) {
    printf("allocFixedMem failed (err %d)\n", ret);
    return ret;
  }
  
 
  /* don't support demand paging, load everything from disk right away */
  if (! (flags & MAP_ANONYMOUS)) {

  //	printf("%08lX, offset %ld (len %ld) (fd:%d)\n", addr, off, len, fd);
 
    if (lseek(fd, off, SEEK_SET) != off) {
      printf("do_mmap(): error seeking to %ld (%d)\n", off, -errno);
      getchar();
      return -errno;
    }    
    
    if (read(fd, (char*)addr, len) < 0) {
      printf("do_mmap(): error reading %ld bytes from file (errno=%d)\n",
              len, -errno);
      return -errno;
    }
    
    if ( (prot & PROT_EXEC) && winVersion != _WINNT) {
      fixupCode((void*)addr, len);
    }
  }
  
  return addr;
}


struct mmap_arg_struct {
  unsigned long addr;
  unsigned long len;
  unsigned long prot;
  unsigned long flags;
  unsigned long fd;
  unsigned long offset;
};


SYSCALL(l_mmap)
{
  struct mmap_arg_struct a = *(struct mmap_arg_struct *)ebx;
  return do_mmap(a.fd, a.addr, a.len, a.prot, a.flags, a.offset);
}

SYSCALL(ll_mmap2)
{
	unsigned long addr = ebx;
	unsigned long len = ecx;
	unsigned long prot = edx;
	unsigned long flags = esi;
	unsigned long fd = edi;
	unsigned long offset = ebp;
	return do_mmap(fd, addr, len, prot, flags, offset*4096);
	//return -ENOMEM;
}


SYSCALL(l_munmap)
{
  return do_munmap(ebx, ecx);
}


SYSCALL(l_mprotect)
{
  //printf("mprotect(0x%08X, 0x%X, %d)", ebx, ecx, edx);
          

  /* don't support changing the protection on MAP_FIXED memory yet, so just
     return ok to the process :) */
  if (fixedMemOp_INUSE(ebx, ecx) == 0) {
    return 0;
  }
  
  return mprotect((void*)ebx, ecx, edx);
}


SYSCALL(l_mremap)
{
  DWORD old_address = ebx;
  long old_size = ecx;
  long new_size = edx;
  DWORD flags = esi;
  
  //printf( "mremap(0x%08lX, 0x%lX. 0x%lX, %ld", 
  //            old_address, old_size, new_size, flags);
  
  if (fixedMemOp_INUSE(ebx, ecx) == 0) {
    return do_mmap(-1, old_address, new_size, PROT_READ|PROT_WRITE,
   	             MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS,0);
  }
  
  /* can't mremap() a Cygwin memory region */
  return -EINVAL; 
}


/* l_brk() is mostly from linux-2.2.5/mm/mmap.c */
SYSCALL(l_brk)
{
  int newbrk, oldbrk;
  //int rlim;

  if (ebx < pInfo.start_brk){
	  //pInfo.brk = 0x88b4000;//PAGE_ALIGN(pInfo.brk);
	  goto out;
  }

  newbrk = PAGE_ALIGN(ebx);
  oldbrk = PAGE_ALIGN(pInfo.brk);

  if (newbrk == oldbrk) goto set_brk;

  /* shrinking brk */
  if (ebx < pInfo.brk) {
  	
    (void) do_munmap(newbrk, oldbrk-newbrk);
    goto set_brk;
  }

#if 0
  /* check against rlimit */
  rlim = pInfo.rlim[RLIMIT_DATA].rlim_cur;
  if (rlim < RLIM_INFINITY &&  (ebx - pInfo.end_code > rlim)) {
    goto out;
  }
#endif
  if (do_mmap(-1, oldbrk, newbrk-oldbrk, PROT_READ|PROT_WRITE,
  	       MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS,0) < 0) {
    goto out;
  }

set_brk:
  pInfo.brk = ebx;
out:
  //printf("alloc memory %x, pInfo.start_brk %x, pInfo.brk=%x\n", ebx, pInfo.start_brk, pInfo.brk);

  return pInfo.brk;
}
