/* need to have access to the cygwin errno but don't want to get all of 
   Cygwin's E... defines */

int *__errno(void);
#define errno (*__errno())

