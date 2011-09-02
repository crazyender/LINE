/**
 * Simple fork test 
 */ 
#include <stdio.h>
#include <unistd.h>

int main()
{
  int child;
  
  printf("Parent is starting (pid %d)\n", getpid());

  child = fork();

  if (0 == child) {
    printf("I am the child (pid %d)\n", getpid());
  } else if (child < 0) {
    printf("Fork failed!\n");
  } else {
    printf("I am the parent (pid %d).  My child is pid %d\n", getpid(), child);
  }

  return 0;
}
