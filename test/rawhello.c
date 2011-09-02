/**
 * Hello World without libc (link with '-e _main_' option)
 */ 
#define __KERNEL_SYSCALLS__
#include <linux/types.h>
#include <linux/unistd.h>

int errno;

void _main_()
{
  write(1, "Hello Windows\n", 14);

  _exit(0); 
}
