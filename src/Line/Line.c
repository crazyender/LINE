/**
 * LINE process monitor.  This program acts as a debugger for the Linux
 * processes, intercepting syscalls and redirecting them back to the
 * originating process for handling 
 * $Id: Line.c,v 1.19 2001/05/01 15:54:30 mvines Exp $
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
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include "syscall_names.h"
#include "process_table.h"
#include "winver.h"
#include "../linexec/process.h"
#include "log.h"
#include "internal_syscalls.h"
#include "version.h"
#include "memflags.h"

#include <winioctl.h>
#include "../int80/int80.h"


/* The Cygwin Windows header files doesn't seem to have this */
PVOID WINAPI VirtualAllocEx(HANDLE,PVOID,DWORD,DWORD,DWORD);

  
/* this is assumed to be at the same address for all linux processes */
static DWORD dispatchHandler = 0;

/* this is also assumed to be at the same address for every linux process */
static ProcessInfo *pInfo = 0;



static void dumpregs(CONTEXT *c)
{
  printf("Registers:\n");
  printf("eax=%08lX, ebx=%08lX, ecx=%08lX, edx=%08lX\n",
      c->Eax, c->Ebx, c->Ecx, c->Edx);
      
  printf("esi=%08lX, edi=%08lX, ebp=%08lX, esp=%08lX\n",
         c->Esi, c->Edi, c->Ebp, c->Esp);
  printf("eip=%08lX\n", c->Eip);
}


static void dumpcodebytes(HANDLE hProcess, DWORD eip)
{
  int i;
  unsigned char b;
  DWORD foo;

  printf("Code bytes:\n");

  for (i = 0; i < 16; i++, eip++) {
    if(0 == ReadProcessMemory(hProcess, (LPCVOID)(eip), &b, 1, &foo)) {   
      printf("unable to read memory at %08lX", eip); 
      break;
    }
    printf("%02X ", b);
  }

  printf("\n");
}


/* active/deactive the NT/2000 int80.sys device */
BOOL EnableInt80Device(BOOL enable) 
{
  DWORD   BytesReturned;
  BOOL    rc;
  DWORD   ioctl;
  char    ServiceCounters[SERVICECOUNTERS_BUFSIZE];
  HANDLE  hDevice;

  if (_WINNT != winVersion) {
    return FALSE;
  }
  
  hDevice = CreateFile("\\\\.\\LinuxSyscallRedirector", 
                       GENERIC_READ | GENERIC_WRITE, 0, NULL, 
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  

  if(INVALID_HANDLE_VALUE == hDevice) {
    log_verb(LOG_LINE_INT80DRIVER, 
             "Unable to open LinuxSyscallRedirector (int80.sys): error %ld", 
             GetLastError());
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
    log_warning(LOG_LINE_INT80DRIVER, 
                "DeviceIoControl failed (error=%ld)", GetLastError());
    return FALSE;
  }
  
  CloseHandle(hDevice);

  return TRUE;
}


/* 64K of empty memory, used in doCommitMemory() */ 
static char emptyMemory[0x10000] = {0,};

int doCommitMemory(EXCEPTION_RECORD *e, HANDLE hProcess)
{              	
  DWORD allocMem;
  MEMORY_BASIC_INFORMATION allocInfo;
  DWORD allocRet;
  char c;
  
  if (winVersion != _WINNT) {
    return 0;
  }

  allocMem =  e->ExceptionInformation[1];

  allocMem &= 0xFFFF0000; /* 64k align the address */
  log_verbose(LOG_LINE_COMMITMEM, "doCommitMemory(%08lX)", allocMem);
   
  allocRet = VirtualQueryEx(hProcess, (void*)allocMem, 
                            &allocInfo, sizeof(allocInfo));
     
  if (allocRet <= 0) { 
    log_error(LOG_LINE_COMMITMEM, "doCommitMemory: VirtualQueryEx() failed");
    return 0;
  }


  log_debug(LOG_LINE_COMMITMEM, "doCommitMemory: "
    "%08lX-%08lX (protect %ld, initprotect %ld, state %ld)",
    (DWORD)allocInfo.BaseAddress, // base address of region 
    (DWORD)allocInfo.BaseAddress + allocInfo.RegionSize, // size, in bytes, of region 
    allocInfo.Protect,
    allocInfo.AllocationProtect,
    allocInfo.State
  );
  

  if (allocInfo.AllocationProtect == PAGE_EXECUTE_READWRITE && 
      allocInfo.State == MEM_RESERVE) {
    
    allocRet = (DWORD)VirtualAllocEx(hProcess, (LPVOID)allocMem, 0xFFFF, 
                                     MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (allocRet != allocMem) {
      log_error(LOG_LINE_COMMITMEM, "doCommitMemory: VirtualAllocEx() failed");
      return 0;
    }                                 
    
    
    if (!WriteProcessMemory(hProcess, (LPVOID)allocMem, emptyMemory, 
                            0xFFFF, &allocRet) || (allocRet != 0xFFFF)) {
      log_error(LOG_LINE_COMMITMEM, "doCommitMemory: error initalizing commited memory to zero");
      return 0;
    }
    
    
    /*
     * Mark the 64K region as allocated 
     */
    if (!ReadProcessMemory(hProcess, &pInfo->fixedMem[allocMem >> 16], 
                           &c, 1, NULL)) {
      log_error(LOG_LINE_COMMITMEM, "doCommitMemory: error reading process info");
      return 0;                            
    }
    
    if (c != MEMFLAG_RESERVED) {
      log_error(LOG_LINE_COMMITMEM, "doCommitMemory: process info is wrong (should be 2, but got %d instead", c);
      return 0;                            
      
    }
    
    c = 1;
    if (!WriteProcessMemory(hProcess, &pInfo->fixedMem[allocMem >> 16], 
                           &c, 1, NULL)) {
      log_error(LOG_LINE_COMMITMEM, "doCommitMemory: error writing process info");
      return 0;                            
    }
    
    log_verbose(LOG_LINE_COMMITMEM, "doCommitMemory: ok");
    return 1;
  }
   
  return 0;
}
  

static void start_linelog(char const *linedir)
{
  STARTUPINFO si;
  PROCESS_INFORMATION ignored_pi;
  char cmdline[MAX_PATH];
  char sName[256];
  HANDLE hSem;
  
  snprintf(sName, sizeof(sName), "LineLogSem%d", getpid());
  hSem = CreateSemaphore(NULL, 0, 1, sName);
  
  strncpy(cmdline, "\"", sizeof(cmdline));
  strncat(cmdline, linedir, sizeof(cmdline));
  strncat(cmdline, "\\linelog.exe\" ", sizeof(cmdline));
  strncat(cmdline, sName, sizeof(cmdline));
  
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  if (!CreateProcess(NULL, cmdline,  
        NULL, NULL, FALSE, CREATE_NEW_CONSOLE,
        NULL, NULL, &si, &ignored_pi)) {
    DWORD d = GetLastError();

    printf("Line: Unable to run Linelog (%s): error %ld)\n", cmdline, d);

  } else {
    WaitForSingleObject(hSem, INFINITE);  // TODO! probably want a timeout here!
  }
  CloseHandle(hSem);
} 
  

static void show_help(void)
{
  printf("Usage: Line.exe [options] linux_executable [arg1 arg2 ...]\n"
         "Options:\n"
         " -l, --linedir=DIR    Specify the LINE 'install' directory\n"
         " -c, --chroot=PATH    Change the root directory to the specified path\n"
         " -m, --map            Dump the current memory map on an unexpected\n"
         "                      exception\n"
         " -s, --sameconsole    Start the Linux application in a same console\n"
         "                      window.  This option is only used when the\n"
         "                      Linux NT driver is installed\n"
         " -h, --help           Display this help and exit\n");
}


int main(int argc, char *argv[])
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  DEBUG_EVENT event;
  WORD instr;
  CONTEXT c; 
  DWORD i;
  DWORD createFlags;
  char cmdline[1024];
  char *linedir = NULL;
  char *linexec = NULL;
  char *root = NULL;
  WORD swiOpcode;
  int pid, tid;
  char *s;
  int dumpMemoryMap = 0;
  int sameConsole = 1;
  BOOL driverActive;

   
  /*printf("LINE %s\n", line_version_string);
  printf("Copyright (C) 2001  Michael Vines\n"
         "http://line.sourceforge.net\n\n"
         "This program is distributed in the hope that it will be useful,\n"
         "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
         "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
         "GNU General Public License for more details\n\n");*/

  get_winver(1);
  printf("\n");
#if 0
  while (1) {
    int option_index = 0;
    int c;
    
    static struct option long_options[] = {
      {"linedir", 1, 0, 'l'},
      {"help", 0, 0, 'h'},
      {"map", 0, 0, 'm'},
      {"sameconsole", 0, 0, 'n'},
      {"chroot", 1, 0, 'c'},
      {0,0,0,0}
    };
    
    c = getopt_long(argc, argv, "l:hsc:m", long_options, &option_index);
    
    if (-1 == c) break;
    
    switch (c) {
    case 'l':
      cygwin_conv_to_full_win32_path(optarg, cmdline);
      linedir = strdup(cmdline);
      cmdline[0] = '\0';
      break;

    case 'c':
      root = strdup(optarg);
      break;
      
    case 'h':
      show_help();
      exit(0);
      
    case 's':
      sameConsole = 1;
      break;
      
    case 'm': 
      dumpMemoryMap = 1;
      break;
      
    default:
      printf("Unknown option '%c'\n", c);
      exit(-1);
    }
      
  }
#endif
  
  //if (optind > argc-1) {
  //  show_help();
  //  exit(1);
  //}
 
  /**
   * --linedir not specified, use the path to line.exe 
   */
  if (NULL == linedir) {
    int   i;
    
    cygwin_conv_to_full_win32_path(argv[0], cmdline);
    
    i = strlen(cmdline);
    while ((i >= 0) && (cmdline[i] != '\\')) {
      i--;
    }
    
    if (i < 0) i = 0;
    cmdline[i] = '\0';
    
    linedir = strdup(cmdline);
    cmdline[0] = '\0';
  }
  

  SetConsoleTitle("LINE");

  start_linelog(linedir);
  
  /* The result of when Win9x encounters a stray INT instruction can
      be anything from a GPF to a complete system crash.  This is not 
      good...on those systems the loader will alter all the INT 80h's  
      to INT 03h's which the debug interface can detect and handle */
  if (_WINNT == winVersion) {
    swiOpcode = 0x80CD;
  } else {
    swiOpcode = 0x03CD;
  }

  init_process_table();
  
  
  driverActive = EnableInt80Device(TRUE);
  

  /*
   * build linexec command line 
   */
  strncpy(cmdline, "\"", sizeof(cmdline));
  strncat(cmdline, linedir, sizeof(cmdline));
  strncat(cmdline, "\\linexec.exe\"", sizeof(cmdline));
  linexec = strdup(cmdline);
  
  if (root != NULL) {
    strncat(cmdline, " -c ", sizeof(cmdline));
    strncat(cmdline, root, sizeof(cmdline));
  }
  
  //strncat(cmdline, " -f -p ", sizeof(cmdline));
  
  //if (driverActive) {
  //  strncat(cmdline, " -n ", sizeof(cmdline));
  //}
  
  /* first argument is the execuable name */
  strncat(cmdline, " ", sizeof(cmdline));
  strncpy(cmdline, "./linexec", sizeof(cmdline));
  strncat(cmdline, " bash", sizeof(cmdline));
  //for (i = optind; i < argc; i++) {
	 // strncat(cmdline, "\"", sizeof(cmdline));
	 // strncat(cmdline, argv[i], sizeof(cmdline));
	 // strncat(cmdline, "\" ", sizeof(cmdline));
  //}
  cmdline[sizeof(cmdline)-1] = '\0';

  /* fire off linexec */
  log(LOG_LINE_MISC, "Running: %s", cmdline);
  execl("./linexec.exe", "-linexec.exe", "bash", 0);
  return 0;
  
  createFlags = 0;
  
  if (!sameConsole) {
    createFlags |= CREATE_NEW_CONSOLE;
  }
      
  if (!driverActive) {
    createFlags |= DEBUG_PROCESS | PROCESS_VM_READ | PROCESS_VM_WRITE;
  }
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  printf("[LINE] cmdline = %s\n", cmdline);
  if (!CreateProcess(NULL, cmdline, 
        NULL, NULL, FALSE, 
        createFlags,
        NULL, NULL, &si, &pi)) {
    DWORD d = GetLastError();

    printf("Line: Unable to run Linexec (%s): error %ld)\n", linexec, d);
    return 1;
  }
  
  free(linexec);
  linexec = NULL;
  
  
  if (driverActive) {
    /* wait for Linexec to exit */
    WaitForSingleObject(pi.hProcess, INFINITE);

    /* deactive int80 redirection */    
    EnableInt80Device(FALSE);
    return 0;
  }


  /* the registers from the linexec process that we are interestd in... */
  c.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;

  while(WaitForDebugEvent(&event, INFINITE)) {
    log_debug(LOG_LINE_DBGEVENT, "Debug Event: pid %08lx, tid %08lx", 
              event.dwProcessId, event.dwThreadId);
    
    switch(event.dwDebugEventCode) {
      case EXCEPTION_DEBUG_EVENT: 
        log_debug(LOG_LINE_EVENTNAME, "EXCEPTION_DEBUG_EVENT"); 

        find_process_info(event.dwThreadId, &pid, &tid);

#if 0 
        printf( "  [code:%08lX    address:%08X]\n",  
                  event.u.Exception.ExceptionRecord.ExceptionCode,
                  (int)event.u.Exception.ExceptionRecord.ExceptionAddress);
#endif

        if (_WINNT == winVersion) {
          /* A breakpoint occurs on my system when the linexec process starts 
             up, it seems to be harmless so just ignore it for now */
          if ((event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) &&
              ((DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress > 0x77000000)) {
          
            log_verbose(LOG_LINE_BREAKPOINT, "Ignoring weird NT breakpoint");
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, 
                           DBG_CONTINUE);
            break;
          }
        }

        if(0 == GetThreadContext(get_thread_handle(pid, tid), &c)) {   
          DWORD d = GetLastError();
          printf( "Can't get thread context (error %ld)\n", d); 
          return 0;
        }

#if 0
        printf( "  esp: %08lX    eip: %08lX    ", c.Esp, c.Eip);
#endif
        
        if (_WINNT != winVersion) {
          c.Eip -= 2;
        }

        if(0 == ReadProcessMemory(get_process_handle(pid), 
                                  (LPCVOID)(c.Eip), &instr, 2, &i)) {   
          printf( "\nUnable to read process memory at EIP (err=%ld).  "
                  "Killing process\n", GetLastError());
          dumpregs(&c);
          MessageBox(0, "Unable to read process memory", 
                     "Fatal Error", MB_ICONERROR);
          TerminateProcess(get_process_handle(pid), -1);
          break;
        }
        
#if 0        
        printf( "  instruction: %04X\n", instr);
#endif

        /* Not a system call? */
        if (instr != swiOpcode) {
        
          if (_WINNT != winVersion) {
            if(event.u.Exception.ExceptionRecord.ExceptionCode ==
                EXCEPTION_BREAKPOINT) {
              /* this happens on Windows 98, doesn't see to be harmful
                 though */
              log_verbose(LOG_LINE_BREAKPOINT, 
                          "Ignored unknown breakpoint: %04X", instr);

              break;
            }
          }
          
          if (EXCEPTION_ACCESS_VIOLATION == 
              event.u.Exception.ExceptionRecord.ExceptionCode) {
                
            if (doCommitMemory(&event.u.Exception.ExceptionRecord, 
                               get_process_handle(pid))) {
              break;
            }
          }
          
          printf("Unexpected exception in Linux executable.\n\n"
                 "Win32 Exception Code: %08lX\n"
		             "Exception Address: %08lX\n",
		             event.u.Exception.ExceptionRecord.ExceptionCode, 
                 (DWORD)event.u.Exception.ExceptionRecord.ExceptionAddress);
          dumpregs(&c);
          dumpcodebytes(get_process_handle(pid), c.Eip);

          MessageBox(0, "Unexpected exception in Linux executable.  Killing process", 
                     "Fatal Error", MB_ICONERROR);
  	      
          ExitProcess(1);
          if (dumpMemoryMap) {
            c.Eax = SYSCALL_DUMPMEMORYMAP;
          } else {
            TerminateProcess(get_process_handle(pid), -1);
          }
          
        } 
  
        c.Eip += 2;

        /* linexec telling us the location of its' syscall handler */
        if (c.Eax == SYSCALL_LINEXEC_HANDLER) {

          /* first time  */
          if (0 == dispatchHandler || 0 == pInfo) {
            dispatchHandler = c.Ebx;
            pInfo = (ProcessInfo *)c.Ecx;
            log_verb(LOG_LINE_LINEXECINFO, "Linexec syscall handler=%08lX, "
                     "processInfo=%08lX", dispatchHandler, (DWORD)pInfo);
          } else {
            /* All instances of linexec should have the same 
               dispatch handler entry point so don't bother storing a 
               separate address for one */
            if (dispatchHandler != c.Ebx || pInfo != (ProcessInfo*)c.Ecx) {
               MessageBox(0, "Dispatch handler location conflict", 
                          "Fatal Error", MB_ICONERROR); 
               exit(1);
            }
          }
        } else {
          if (c.Eax < NUM_SYSCALLS) {
            log_verb(LOG_LINE_SYSCALL, "[%08lX] SYSCALL %s (%ld)",  c.Eip, 
                                       syscall_names[c.Eax],c.Eax);
          } else {
            log_verb(LOG_LINE_SYSCALL, "[%08lX] SYSCALL %ld", c.Eip, c.Eax);
          }

          /* push EIP onto the stack for the RET */
          c.Esp -= 4;
          if(0 == WriteProcessMemory(get_process_handle(pid), 
                                  (LPVOID)(c.Esp), &c.Eip, 4, &i)) {   
            printf( "Unable to push EIP on stack!");
            return 0;
          }

          c.Eip = dispatchHandler;
        }
        
        if(0 == SetThreadContext(get_thread_handle(pid, tid), &c)) {
          printf( "Can't set thread context"); 
          return 0;
        }
        break;


      case CREATE_THREAD_DEBUG_EVENT: 
        log_debug(LOG_LINE_EVENTNAME, "CREATE_THREAD_DEBUG_EVENT");  

        find_process_info_with_pid(event.dwProcessId, &pid);

        tid = add_thread(pid, event.u.CreateThread.hThread, event.dwThreadId);
        if (-1 == tid) {
          MessageBox(0, "Thread Table Full.  Killing process", "LINE", MB_ICONERROR);
          TerminateProcess(get_process_handle(pid), -1);
        }
        break;


      case CREATE_PROCESS_DEBUG_EVENT: 
        log_debug(LOG_LINE_EVENTNAME, "CREATE_PROCESS_DEBUG_EVENT");  

        pid = add_process(event.u.CreateProcessInfo.hProcess, event.dwProcessId, 
                          event.u.CreateProcessInfo.hThread, event.dwThreadId);
        if (-1 == pid) {
          MessageBox(0, "Process Table Full", "LINE", MB_ICONERROR);
          return 0;
        }
        break;


      case EXIT_THREAD_DEBUG_EVENT: 
        log_debug(LOG_LINE_EVENTNAME, "EXIT_THREAD_DEBUG_EVENT");  

        find_process_info(event.dwThreadId, &pid, &tid);
        remove_thread(pid, tid);
        break;


      case EXIT_PROCESS_DEBUG_EVENT: 
        log_debug(LOG_LINE_EVENTNAME, "EXIT_PROCESS_DEBUG_EVENT"); 
        
        log_debug(LOG_LINE_PROCESSEXIT, "exitcode: %ld", event.u.ExitProcess.dwExitCode);

        find_process_info_with_pid(event.dwProcessId, &pid);
        
        if (!remove_processinfo(pid)) {
          printf("\nAll child processes have exited.  "
                 "Terminating LINE process monitor.\n");
          return 0;
        }
        break;


      case LOAD_DLL_DEBUG_EVENT: 
        log_debug(LOG_LINE_EVENTNAME, "LOAD_DLL_DEBUG_EVENT");  
        
        log_debug(LOG_LINE_DLLINFO, "Loading DLL to base address %08lX", 
                                     (DWORD)event.u.LoadDll.lpBaseOfDll);
        if ((0 == event.u.LoadDll.fUnicode) && event.u.LoadDll.lpImageName) {
          char *dllptr;
          char dllname[MAX_PATH];
          
          if (!ReadProcessMemory(get_process_handle(pid), 
                            event.u.LoadDll.lpImageName, &dllptr, 
                            sizeof(dllptr), NULL)) break;

          if (NULL == dllptr) break;
          
          if (!ReadProcessMemory(get_process_handle(pid), 
                            dllptr, dllname, 
                            sizeof(dllname), NULL)) break;
          
          log_debug(LOG_LINE_DLLINFO, "DLL name is %s", dllname);
        }

        break;


      case UNLOAD_DLL_DEBUG_EVENT: 
        log_debug(LOG_LINE_EVENTNAME, "UNLOAD_DLL_DEBUG_EVENT");  
        log_debug(LOG_LINE_DLLINFO, "Unloading DLL at base address %08lX", 
                                    (DWORD)event.u.UnloadDll.lpBaseOfDll);
        break;


      case OUTPUT_DEBUG_STRING_EVENT: 
        log_debug(LOG_LINE_EVENTNAME, "OUTPUT_DEBUG_STRING_EVENT");  
        if (event.u.DebugString.fUnicode) {
           log_debug(LOG_LINE_DBGSTRING, "(unicode strings not supported)");
           break;
        }

        s = malloc(event.u.DebugString.nDebugStringLength);
        if (s != NULL) {
          find_process_info_with_pid(event.dwProcessId, &pid);

          ReadProcessMemory(get_process_handle(pid), 
                            event.u.DebugString.lpDebugStringData, s, 
                            event.u.DebugString.nDebugStringLength, NULL);
          log_debug(LOG_LINE_DBGSTRING, "%s", s);
          free(s);
          s = NULL;
        }
        break;


      case RIP_EVENT:
        log_debug(LOG_LINE_EVENTNAME, "RIP_EVENT");  
        break;


      default:
        log_debug(LOG_LINE_EVENTNAME, "*****UNKNOWN DEBUG EVENT*****");
        break;
    }

    ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE);
  }
  return 0;
}

