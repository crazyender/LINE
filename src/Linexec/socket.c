/**
 * Socket syscalls
 * $Id: socket.c,v 1.2 2001/03/20 21:06:13 mvines Exp $
 *
 * Copyright (C) 2001  Michael Vines
 *
 * Large portions of this file have been taken from the Linux 2.2.5 kernel
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
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include "net.h"
#include "syscall.h"
#include "cygwin_errno.h"
#include "errno.h"

#include "log.h"


/* this was taken from linux/net/socket.c 
     asmlinkage int sys_socketcall(int call, unsigned long *args)  */
SYSCALL(l_socketcall)
{
	unsigned long *a = (unsigned long *)ecx;
  unsigned long a0,a1;
  int call = ebx;
  int err;
  
  log_debug(LOG_LINEXEC_SOCKET, "socketcall(%d)", call);
	
	if(call<1||call>SYS_RECVMSG)
		return -EINVAL;

	a0=a[0];
	a1=a[1];
	
	switch(call) 
	{
		case SYS_SOCKET:
			err = socket(a0,a1,a[2]);
//			printf ("%d = socket()\n", err);
			break;
		case SYS_BIND:
			err = bind(a0,(struct sockaddr *)a1, a[2]);
			break;
		case SYS_CONNECT:
		  /*
		  {
		   struct sockaddr_in *s;
		   
		   s = (struct sockaddr_r_in*)a1;
		   printf("len: %d (=%d)\n", a[2], sizeof(struct sockaddr));
		   printf("fam: %d\n", s->sin_family);
		   printf("port: %d\n", ntohs(s->sin_port));
		   printf("addr: %08X\n", ntohl(s->sin_addr.s_addr));
		   printf("real addr: -%s-\n", inet_ntoa(s->sin_addr.s_addr));
		  }
		  */
  		err = connect(a0, (struct sockaddr *)a1, a[2]);
			break;
		case SYS_LISTEN:
			err = listen(a0,a1);
			break;
		case SYS_ACCEPT:
			err = accept(a0,(struct sockaddr *)a1, (int *)a[2]);
			break;
		case SYS_GETSOCKNAME:
			err = getsockname(a0,(struct sockaddr *)a1, (int *)a[2]);
			break;
		case SYS_GETPEERNAME:
			err = getpeername(a0, (struct sockaddr *)a1, (int *)a[2]);
			break;
		case SYS_SOCKETPAIR:
			err = socketpair(a0,a1, a[2], (int *)a[3]);
			break;
		case SYS_SEND:
			err = send(a0, (void *)a1, a[2], a[3]);
			break;
		case SYS_SENDTO:
			err = sendto(a0,(void *)a1, a[2], a[3],
					 (struct sockaddr *)a[4], a[5]);
			break;
		case SYS_RECV:
			err = recv(a0, (void *)a1, a[2], a[3]);
			break;
		case SYS_RECVFROM:
			err = recvfrom(a0, (void *)a1, a[2], a[3],
					   (struct sockaddr *)a[4], (int *)a[5]);
			break;
		case SYS_SHUTDOWN:
			err = shutdown(a0,a1);
			break;
		case SYS_SETSOCKOPT:
			err = setsockopt(a0, a1, a[2], (char *)a[3], a[4]);
			break;
		case SYS_GETSOCKOPT:
			err = getsockopt(a0, a1, a[2], (char *)a[3], (int *)a[4]);
			break;
			
		case SYS_SENDMSG:
		  printf("TODO: implement socketcall(SYS_SENDMSG)\n");
			//err = sendmsg(a0, (struct msghdr *) a1, a[2]);
		  return -EINVAL;
			break;
			
		case SYS_RECVMSG:
		  printf("TODO: implement socketcall(SYS_RECVMSG)\n");
			//err = recvmsg(a0, (struct msghdr *) a1, a[2]);
		  return -EINVAL;
			break;
			
		default:
		  return -EINVAL;
	}

  if (err < 0) err = -errno;

//	printf("socketcall ret: %d (errno %d)\n", err, errno);
	return err;
}

