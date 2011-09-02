/**
 * More complex fork test.  
 */ 
#include <stdio.h>
#include <unistd.h>

int main()
{
  int i;
  char strbuf[256];
  int magicvalue = 0xF00;
  
  sprintf(strbuf, "Parent is starting (pid %d).  The magic value is %03X\n", 
                  getpid(), magicvalue);
  printf(strbuf);

  for (i = 0; i < 10; i++) {
    int child = fork();

    if (child < 0) {
      sprintf(strbuf, "Fork %d failed!\n", i);
      printf(strbuf);
    } else if (child) {
      sprintf(strbuf, "Child %d has been started with pid %d\n", i, child);
      printf(strbuf);
    } else {
      int pid = getpid();
      sprintf(strbuf, "I am child %d with pid %d.  The magic value is %03X\n", 
                       i, pid, magicvalue);
      printf(strbuf);
      return pid;
    }
  }

  return 0;
}
