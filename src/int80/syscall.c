#include <ntddk.h>
#include "syscall.h"
#include "systable.h"

static ULONG _syscallHandlerPtr;
volatile LONG gFinishedDPC = 0;
IdtEntry_t OldIdtEntry;
IdtEntry_t LinOldIdtEntry;
/* Buffer to store result of sidt instruction */
char buffer[6];
/* Pointer to structure to identify the limit and base of IDTR*/
PIdtr_t Idtr=(PIdtr_t)buffer;

/* XXX: useCount is suffering from a big race condition.  
        replace with an atomic counter soon! */

int useCount = 0;

__declspec(naked)  InterruptHandlerForKernel()
{
    __asm{
            PUSH    eax //78
            PUSH    0   //74
            PUSH    fs  //70
            PUSH    es  //6c
            PUSH    ds  //68
            PUSH    ebp //64
            PUSH    edi //60
            PUSH    esi //5c
            PUSH    edx //58
            PUSH    ecx //54
            PUSH    ebx //50



            // nt system call needs to setup fs for things such as tram frame, SEL, etc
            MOV     ebx,30h // segment selector 30h
            MOV     fs,bx
            MOV     ebx,23h
            MOV     esi,dword ptr fs:[124h]
            PUSH    dword ptr fs:[0]            // -4ch
            MOV dword ptr fs:[0],0FFFFFFFFh
            PUSH    dword ptr [esi+13Ah]        // -48h
            SUB     esp,48h                     
            MOV     ebx,dword ptr [esp+6Ch]
            AND     ebx,1
            MOV     byte ptr [esi+13Ah],bl
            MOV     ebp,esp
            MOV     ebx,dword ptr [esi+128h]
            MOV     dword ptr [ebp+3Ch],ebx
            AND     dword ptr [ebp+2Ch],0
            TEST    byte ptr [esi+3],0DFh
            MOV     dword ptr [esi+128h],ebp
            cld
            JNE     Dr_kss_ab
Dr_kss_aa:
            MOV     ebx,dword ptr [ebp+60h]
            MOV     edi,dword ptr [ebp+68h]
            MOV     dword ptr [ebp+0Ch],edx
            MOV     dword ptr [ebp+8],0BADB0D00h
            MOV     dword ptr [ebp],ebx
            MOV     dword ptr [ebp+4],edi
            JMP     Dr_kss_done
Dr_kss_ab:
            TEST    dword ptr [ebp+70h],20000h
            JNE     Dr_kss_ac
            TEST    byte ptr [ebp+6Ch],1
            JE      Dr_kss_aa

Dr_kss_ac:
            MOV     ebx,dr0
            MOV     ecx,dr1
            MOV     edi,dr2
            MOV     dword ptr [ebp+18h],ebx
            MOV     dword ptr [ebp+1Ch],ecx
            MOV     dword ptr [ebp+20h],edi
            MOV     ebx,dr3
            MOV     ecx,dr6
            MOV     edi,dr7
            MOV     dword ptr [ebp+24h],ebx
            MOV     dword ptr [ebp+28h],ecx
            XOR     ebx,ebx
            MOV     dword ptr [ebp+2Ch],edi
            MOV     dr7,ebx
            MOV     edi,dword ptr fs:[20h]
            MOV     ebx,dword ptr [edi+2F4h]
            MOV     ecx,dword ptr [edi+2F8h]
            MOV     dr0,ebx
            MOV     dr1,ecx
            MOV     ebx,dword ptr [edi+2FCh]
            MOV     ecx,dword ptr [edi+300h]
            MOV     dr2,ebx
            MOV     dr3,ecx
            MOV     ebx,dword ptr [edi+304h]
            MOV     ecx,dword ptr [edi+308h]
            MOV     dr6,ebx
            MOV     dr7,ecx
            JMP     Dr_kss_aa
Dr_kss_done:

			sti
            // setup linux system call parameters
            MOV  edx, esp
            PUSH dword ptr [edx+78h] //original eax
            PUSH dword ptr [edx+74h] //gs
            PUSH dword ptr [edx+70h] //fs
            PUSH dword ptr [edx+6ch] //es
            PUSH dword ptr [edx+68h] //ds
            PUSH eax                 //eax
            PUSH dword ptr [edx+64h] //ebp
            PUSH dword ptr [edx+60h] //edi
            PUSH dword ptr [edx+5ch] //esi
            PUSH dword ptr [edx+58h] //edx
            PUSH dword ptr [edx+54h] //ecx
            PUSH dword ptr [edx+50h] //ebx
			call [LinuxServiceTable+eax*4]
            ADD     esp, 30h
			//MOV [esp+24], eax
            MOV     edx,dword ptr [esp+4Ch] // saved fs[0]
            MOV     dword ptr fs:[0],edx
            MOV     ecx,dword ptr [esp+48h]
            MOV     esi,dword ptr fs:[124h]
            MOV     byte ptr [esi+13Ah],cl
            ADD esp, 50h
            

            POP     ebx
            POP     ecx
            POP     edx
            POP     esi
            POP     edi
            POP     ebp
            POP     ds
            POP     es
            POP     fs
			ADD     esp,8
		
		iretd
     }
}

__declspec(naked)  InterruptHandlerForUser()
{
    __asm{
    CMP     EAX, 0DEADBEEFh
    JNE     reflect_syscall

    MOV     DS:_syscallHandlerPtr, EBX
    IRETD


reflect_syscall:
    PUSH    EAX

    ; simple sanity check
    MOV     EAX, DS:_syscallHandlerPtr
    CMP     EAX, 0
    JE      no_handler

    PUSH    EBX

    MOV     EBX, DWORD PTR [ESP+8]  ; read userland EIP from stack
    MOV     DWORD PTR [ESP+8], EAX  ; set EIP to syscall handler 

    MOV     EAX, DWORD PTR [ESP+8+12]  ; get ESP from stack
    SUB     EAX, 4
    MOV     DWORD PTR [ESP+8+12], EAX  ; write new ESP 
    MOV     DWORD PTR [EAX], EBX         ; place EIP on top of userland stack

    POP     EBX
    
    JMP     exit_handler

no_handler:
    POP     EAX
    PUSH    -38                         ; -38 == ENOSYS

exit_handler:
    POP     EAX
    IRETD

    }
}

__declspec(naked)  InterruptHandler()
{
    __asm{
        CMP     EAX, 0DEADBEEFh
        JE      InterruptHandlerForUser
        PUSH    EBX
        MOV     EBX, [LinuxServiceTable+eax*4]
        CMP     EBX, 0
        JE      user_handler
        // following is kernel handler
        POP     EBX
        JMP     InterruptHandlerForKernel
user_handler:
        POP     EBX
        JMP     InterruptHandlerForUser
    }
}

void SetupSystemCall(struct _KDPC *DPC, ULONG cpuNum, PVOID arg1, PVOID arg2)
{
	void* fp = NULL;
	Idtr_t        idtr;
	PIdtEntry_t    IdtEntry;
	__asm sidt    idtr;

	IdtEntry = (PIdtEntry_t)idtr.Base;

	__asm cli

    // save old idt entry, restore it when unload driver
	RtlCopyMemory(&OldIdtEntry,&IdtEntry[0x80],sizeof(OldIdtEntry));

	IdtEntry[0x80].OffsetLow    = (unsigned short)InterruptHandler;
	IdtEntry[0x80].Selector        = 8;
	IdtEntry[0x80].Reserved        = 0;
	IdtEntry[0x80].Type        = 0xe;
	IdtEntry[0x80].Always0    = 0;
    // 3 means can be called from user mode
	IdtEntry[0x80].Dpl        = 3;
	IdtEntry[0x80].Present        = 1;
	IdtEntry[0x80].OffsetHigh    = (unsigned short)((unsigned int)InterruptHandler >> 16);

	__asm sti

	InterlockedIncrement(&gFinishedDPC);
	DbgPrint("[ring0]gFinishedDPC = 0x%x", gFinishedDPC);
}

void RemoveInterrupt(void)
{
  PIdtEntry_t IdtEntry;

  /* Reach to IDT */
  IdtEntry=(PIdtEntry_t)Idtr->Base;

  _asm cli

  /* Restore the old IdtEntry */
  memcpy(&IdtEntry[HOOKINT], &OldIdtEntry, sizeof(OldIdtEntry));

  _asm sti
  
  useCount = 0;
}