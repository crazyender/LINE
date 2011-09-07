#ifndef SYSTABLE_H
#define SYSTABLE_H
#include <ntddk.h>
#define NOIMLE 0
ULONG LinuxServiceTable[];

#define SYSIMPL(syscall_name) int __cdecl syscall_name(int ebx, int ecx, int edx, \
                             int esi, int edi, int ebp, int eax) 

SYSIMPL(sys_uname);
SYSIMPL(sys_oldolduname);
SYSIMPL(sys_olduname);
#endif