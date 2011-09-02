/**
 * Simple fork test without libc (link with '-e _main_' option)
 */ 
#define __KERNEL_SYSCALLS__
#include <linux/types.h>
#include <linux/unistd.h>

static inline _syscall0(int,fork) 

int errno;

void _main_()
{
  pid_t child;
  
  write(1, "Parent is starting\n", 19);

  child = fork();

  if (0 == child) {
    write(1, "I am the child\n", 15);
  } else if (child < 0) {
    write(1, "Fork failed!\n", 13);
  } else {
    write(1, "I am the parent\n", 16);
  }

  _exit(0); 
}
