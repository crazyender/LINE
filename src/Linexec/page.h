/* linux-2.2.5/include/asm/page.h */
#ifndef __PAGE_H__
#define __PAGE_H__

#define PAGE_SHIFT 11
#define PAGE_SIZE  (0x400)
#define PAGE_MASK  (~(PAGE_SIZE-1))

/* align to the (next) page boundary */
#define PAGE_ALIGN(addr) (((addr)+PAGE_SIZE-1) & PAGE_MASK)

#define PAGE_START(addr) ((addr) & PAGE_MASK)
#define PAGE_OFFSET(addr) ((addr) & (PAGE_SIZE-1))


/*
 
 NOTE: PROCESS_BOTTOM and PROCESS_TOP must be 64K aligned!
 
 */

/* win98 can't use memory < 0x00400000 */
#define PROCESS_BOTTOM 0x00400000 

/*
   cygwin1.dll is loaded around 0x61000000, 
   linexec is at 0x65000000, Win32 DLLs are > 0x70000000 
 
   PROCESS_BOTTOM-PROCESS_TOP is the range of memory that LINE will mark
   reserved for it's own use, which prevents other DLLs in the address space
   from grabbing memory behind LINE's back.  Note that you _don't_ want to
   make this number too large because it will take memory from the 
   other DLLs which may cause errors (and prevent MAP_SHARED from working)
   
 */
#define PROCESS_TOP    0x40000000  
//#define PROCESS_TOP    0x60010000  


#define TASK_SIZE (PROCESS_TOP)

#define STACK_TOP      TASK_SIZE
//#define STACK_TOP      0x30000000


#endif

