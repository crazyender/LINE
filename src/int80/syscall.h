#ifndef SYSCALL_H
#define SYSCALL_H
#include <ntddk.h>
#include "undocnt2k.h"
#include "intel.h"



/* Interrupt to be hooked */
#define HOOKINT 0x80  


void SetupSystemCall(struct _KDPC *DPC, ULONG cpuNum, PVOID arg1, PVOID arg2);
void RemoveInterrupt(void);
extern volatile LONG gFinishedDPC;
extern IdtEntry_t OldIdtEntry;
extern IdtEntry_t LinOldIdtEntry;
extern PIdtr_t Idtr;
extern int useCount;
extern char buffer[];
#endif