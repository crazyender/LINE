/**
 * $Id: linelog.c,v 1.6 2001/03/22 21:56:51 mvines Exp $
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
#include <stdio.h> 
#include <string.h> 
#include <windows.h> 

#include "reg.h"
#include "log.h"
#include "logapi.h"

HWND hWnd;
HANDLE hSem = NULL;


#define MAX_HANDLERS 10

struct handler {
  HINSTANCE hDll;
  
  char *name;

  type_handlerLoad handlerLoad;
  type_handlerMsg handlerMsg;
  type_handlerConfig handlerConfig;
};

int hcount = 0;
struct handler hlist[MAX_HANDLERS];

HKEY hLogKey;


static void msgerr(char const *fmt,...) __attribute ((format (printf, 1, 2)));

static void msgerr(char const *fmt,...) 
{
  va_list args;
  char buf[1024];

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  buf[sizeof(buf)-1] = '\0';
  MessageBox(0, buf, "LINELog", MB_ICONERROR);

}
  

static int loadHandler(char *name)
{
  HKEY hKey;
  char file[MAX_PATH];
  DWORD type, size, result;
  int retval = 0;
  struct handler newh;

  result = RegOpenKeyEx(hLogKey, name, 0, KEY_READ, &hKey);
  if (ERROR_SUCCESS != result) {
    msgerr("Unable to open handler registry key for '%s' (error %ld)\n", 
           name, result);
    return 1;
  }

  do {
    size = sizeof(file);

    if (ERROR_SUCCESS != RegQueryValueEx(hKey, REGVAL_HANDLER_DLL, NULL, &type, 
                                         (LPBYTE)file, &size)) {
      msgerr("Error reading handler reginfo for '%s' (error %ld)\n", 
             name, GetLastError());
      retval = 1;
      break;
    }

    if (REG_SZ != type) {
      msgerr("Got wrong value type when reading handler reginfo for '%s' (error %ld)\n", 
             name, GetLastError());
      retval = 1;
      break;
    }

    /* pass in the hKey so the DLL can read it's configuration if necessary */
    newh.hDll = LoadLibrary(file);
    do {
      if (NULL == newh.hDll) {
        //msgerr("Unable to load '%s' (error %ld)\n", file, GetLastError());
        break;
      }

      newh.name = strdup(name);
      newh.handlerConfig = (type_handlerConfig)GetProcAddress(newh.hDll, "handlerConfig");
      newh.handlerLoad = (type_handlerLoad)GetProcAddress(newh.hDll, "handlerLoad");
      newh.handlerMsg = (type_handlerMsg)GetProcAddress(newh.hDll, "handlerMsg");

      if (NULL == newh.handlerConfig ||
          NULL == newh.handlerLoad ||
          NULL == newh.handlerMsg) {
        msgerr("Error loading handler symbols from '%s'\n", file);
        break;
      }

      if (hcount >= MAX_HANDLERS) {
        msgerr("Too many handlers\n");
        break;
      }
      
      if (newh.handlerLoad(hKey) != 0) {
        msgerr("'%s'.handlerLoad() failed\n", newh.name);
        ExitProcess(1);
      }
  
      hlist[hcount++] = newh;
    } while (0);

  } while (0);

  RegCloseKey(hKey);  
  return retval;
}


static int loadHandlers(void)
{
  int retval = 0;
  char name[MAX_PATH];
  int i;
  int result;
  DWORD size;

  do {
    i = 0;
    while (1) {
      size = sizeof(name);
      result = RegEnumKeyEx(hLogKey, i, name, &size, NULL, NULL, NULL, NULL);

      if (ERROR_NO_MORE_ITEMS == result) break;
      if (result != ERROR_SUCCESS) {
        retval = 1;
        break;
      }

      if (loadHandler(name) != 0) {
        retval = 1;
        break;
      }
      i++;
    }
  } while (0);

  return retval;
}

 
static void dispatchMsg(struct cooked_log_msg *msg)
{
  int i;
  
  for (i = 0; i < hcount; i++) {
    hlist[i].handlerMsg(msg);
  }
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PCOPYDATASTRUCT pcds;
  struct cooked_log_msg msg;
  
  switch (uMsg) {
  case WM_CREATE:
    ReleaseSemaphore(hSem, 1, NULL);
    CloseHandle(hSem);
    break;
    
  case WM_COPYDATA: 
    pcds = (PCOPYDATASTRUCT)lParam;
    
    if (pcds->cbData != sizeof(*msg.raw)) {
      msgerr("Invalid log message size: %ld bytes\n", pcds->cbData);
      break;
    }
    
    msg.raw = (struct log_msg*)pcds->lpData;
    msg.source = (msg.raw->id & LOG_SOURCE_MASK) >> LOG_SOURCE_SHIFT;
    msg.level  = (msg.raw->id & LOG_LEVEL_MASK) >> LOG_LEVEL_SHIFT;
    msg.id     = (msg.raw->id & LOG_ID_MASK) >> LOG_ID_SHIFT;
    
    dispatchMsg(&msg); 
    break;
    
  case WM_USER:
    if (lParam == WM_LBUTTONDOWN) {
      static char *msg = 
         "LINELog\n"
         "Copyright (C) 2001  Michael Vines\n"  
         "http://line.sourceforge.net\n\n"
         "This program is distributed in the hope that it will be useful,\n"
         "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
         "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
         "GNU General Public License for more details.\n\n"
         "Right click on the system tray icon to exit.";

      MessageBox(GetTopWindow(NULL), msg, "About LINELog", MB_ICONINFORMATION);
      break;
    } 
  
    if (lParam != WM_RBUTTONUP) {
      break;
    }
    /* fall-through */
    
  case WM_QUIT:
    PostQuitMessage(0);
    break;
    
  default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  } 

  return TRUE;
}



#define free_and_exit \
  { \
    ReleaseSemaphore(hSem, 1, NULL); \
    CloseHandle(hSem); \
    CloseHandle(hLogKey); \
    return 0; \
  }
  

int WINAPI WinMain(HINSTANCE hInstance,  HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{ 
  MSG msg; 
  WNDCLASS wc; 
	NOTIFYICONDATA ni;
	LPTSTR sem;
	
	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, REGKEY_LOGGER, 0, NULL,
                                      0, KEY_READ, NULL, &hLogKey, NULL)) {
    msgerr("Unable to create/open log registry key\n");
    free_and_exit;
  }
   
  
  sem = GetCommandLine();
  if ('\"' == *sem) {
    while (1) {
      sem++;
      if (!*sem) break;
      
      if (*sem == '\"') {
        sem++;
        break;
      }
    }
  } else {
    while (1) {
      sem++;
      if (!*sem) break;
      
      if (*sem == ' ') {
        sem++;
        break;
      }
    }    
  }
  
  while (*sem == ' ') {
    sem++;
  }

  if (*sem) {
    hSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, sem);
    if (NULL == hSem) {
      msgerr("Error opening semaphore '%s'\n", sem);
      free_and_exit;
    }
  }

  /* make sure we're not already running */
  if (FindWindow("LINELog", "LINELog") != NULL) {
    free_and_exit;
  }
  
  wc.style = CS_NOCLOSE;
  wc.lpfnWndProc = (WNDPROC)WndProc; 
  wc.cbClsExtra = 0; 
  wc.cbWndExtra = 0; 
  wc.hInstance = hInstance; 
  wc.hIcon = NULL; 
  wc.hCursor = NULL;
  wc.hbrBackground = NULL; 
  wc.lpszMenuName =  NULL; 
  wc.lpszClassName = "LINELog"; 

  if (!RegisterClass(&wc)) {
    msgerr("Unable to register window class\n");
    free_and_exit;
  }
 
  // Create the hidden window. 
  hWnd  = CreateWindow("LINELog", "LINELog", 0, CW_USEDEFAULT, CW_USEDEFAULT, 
                       400, 400, (HWND)NULL, (HMENU)NULL, hInstance, 
                       (LPVOID)NULL); 
 
  if (!hWnd) {
    msgerr("Unable to create main window\n");
    free_and_exit;
  }

  if (loadHandlers() != 0) {
    free_and_exit;
  }
  
  // Create the systray icon
	ni.cbSize = sizeof(ni);
	ni.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	ni.hWnd = hWnd;
	ni.uID = 0;
	ni.uCallbackMessage = WM_USER;
	strcpy(ni.szTip, "LINELog -- Right click to terminate");
	ni.hIcon = LoadIcon(NULL, IDI_WARNING);

	Shell_NotifyIcon(NIM_ADD, &ni);	

 
  while (GetMessage(&msg, (HWND) NULL, 0, 0)) { 
    TranslateMessage(&msg); 
    DispatchMessage(&msg); 
  } 

  Shell_NotifyIcon(NIM_DELETE, &ni);

  CloseHandle(hLogKey);
  return msg.wParam; 
} 
