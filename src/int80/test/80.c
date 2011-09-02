/**
 * int80.sys test program
 * $Id: 80.c,v 1.2 2001/05/29 16:05:30 mvines Exp $
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
#include <windows.h>
#include <stdio.h>
#include <winioctl.h>
#include "../int80.h"


BOOL EnableInt80Device(BOOL enable);


int syscallHandler(void)
{
  printf(" ==> Hello from syscallHandler() <==\n");
  return 0;
}


int main(int argc,char *argv[])
{
  if (!EnableInt80Device(TRUE)) {
    return 1;
  }
  
  printf("Sending syscall handler address to driver.\n");

  asm("movl  $0xDEADBEEF, %eax 
       movl  $_syscallHandler, %ebx
       int   $0x80");
  
  printf("Trying a syscall.\n");

  asm("movl  $1, %eax
       int   $0x80");

  printf("Return from syscall.  Everything looks good!\n");
  
  EnableInt80Device(FALSE);
  return 0;
}


BOOL EnableInt80Device(BOOL enable) 
{
  DWORD   BytesReturned;
  BOOL    rc;
  DWORD   ioctl;
  char    ServiceCounters[SERVICECOUNTERS_BUFSIZE];
  HANDLE  hDevice;

  hDevice = CreateFile("\\\\.\\LinuxSyscallRedirector", 
                       GENERIC_READ | GENERIC_WRITE, 0, NULL, 
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  

  if(INVALID_HANDLE_VALUE == hDevice) {
    printf("Unable to open LinuxSyscallRedirector (error %ld)\n", GetLastError());
    return FALSE;
  }


  if (enable) {
    ioctl = (DWORD)IOCTL_ADDINT_SYSTEM_SERVICE_USAGE;
  } else {
    ioctl = (DWORD)IOCTL_REMOVEINT_SYSTEM_SERVICE_USAGE;
  }


  rc = DeviceIoControl(hDevice, ioctl, NULL, 0, 
                       ServiceCounters, sizeof(ServiceCounters),
                       &BytesReturned, NULL);
  
  if (!rc) {
    printf("DeviceIoControl failed (error=%ld)\n", GetLastError());
    return FALSE;
  }
  
  CloseHandle(hDevice);

  return TRUE;
}
