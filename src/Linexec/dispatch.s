/**
 * LINE Syscall Handler
 * $Id: dispatch.s,v 1.3 2001/03/24 22:50:49 mvines Exp $
 *
 * Copyright (C) 2001  Michael Vines
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */ 
 
.text
.globl _syscall_dispatch
	.def	_syscall_dispatch;	.scl	2;	.type	32;	.endef


_syscall_dispatch:	
	/* swap stacks */ 
	movl %esp, _pInfo+4  /* pInfo.userSP */
	movl _pInfo, %esp    /* pInfo.syscallSP */
	
	/* push regs  */
	push %ebp
	push %edi
	push %esi
	push %edx
	push %ecx
	push %ebx
	push %eax

	/*
	xorl %ebx, %ebx
	call *_syscall_tbl(,%ebx, 4)
	pop %eax
	pop %ebx
	push %ebx
	push %eax
	*/

	cmpl $256, %eax
	jb do_syscall

	/* invalid syscall, redirect to syscall 0 */
	xorl %eax, %eax
	
do_syscall:
	call *_syscall_tbl(,%eax, 4)

exit_dispatch:
	pop %ebx  /* don't want to restore %eax */
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp

	/* restore stack */
	push _pInfo+4 /* pInfo.userSP */
	pop %esp
	ret
