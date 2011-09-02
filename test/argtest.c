/**
 * Output the environment and command line arguments
 */

#include <stdio.h>

int main(int argc, char *argv[], char *envp[])
{
  int i;

  printf("----args-----\n");
  for(i = 0; i < argc; i++) {
    printf("%d - %s\n", i, argv[i]);
  }

  printf("----env-----\n");
  for(i = 0; NULL != envp[i]; i++) {
    printf("%d - %s\n", i, envp[i]);
  }

  return 0;
}
