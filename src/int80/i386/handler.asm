;
; $Id: handler.asm,v 1.2 2001/05/01 17:39:00 mvines Exp $
;
; Copyright (C) 2001  Michael Vines
;
.386
.model small

.data
    _syscallHandlerPtr dd 0

.code

public _InterruptHandler

_InterruptHandler proc

    ; Check for SYSCALL_LINEXEC_HANDLER
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

_InterruptHandler endp

End

