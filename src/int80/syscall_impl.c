#include "systable.h"
#include "errno.h"
#include "utsname.h"

SYSIMPL(sys_uname)
{
    struct old_utsname *lu;
    lu = (struct old_utsname*)ebx;
  
    strcpy(lu->sysname, UMANE_LINE_SYSNAME);
    strcpy(lu->nodename, UNAME_LINE_NODE);
    strcpy(lu->release, UNAME_LINUX_RELEASE); // u.release);
    strcpy(lu->version, UNAME_LINE_VERSION);
    strcpy(lu->machine, UNAME_LINE_MACHINE);
  return 0;
}

SYSIMPL(sys_oldolduname)
{
    return -ENOSYS;
}

SYSIMPL(sys_olduname)
{
    return -ENOSYS;
}