/**
 * LINELog handler
 * $Id: logcon.c,v 1.5 2001/04/23 21:32:12 mvines Exp $
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

#include "log.h"
#include "logapi.h"


static HANDLE hStdout;
static HANDLE hFile;
static int minLevel;
static char logFile[MAX_PATH] = {0,};


static void save_config(HKEY hKey)
{
  RegSetValueEx(hKey, "loglevel", 0, REG_DWORD, (BYTE*)&minLevel, sizeof(minLevel));
  RegSetValueEx(hKey, "logfile", 0, REG_SZ, logFile, strlen(logFile));
}      


static void load_config(HKEY hKey)
{
  DWORD type;
  DWORD size;
  DWORD ret;
    
  size = sizeof(minLevel);
  ret = RegQueryValueEx(hKey, "loglevel", 0, &type, (BYTE*)&minLevel, &size);
  if ((type != REG_DWORD) || ret != (ERROR_SUCCESS)) {
    minLevel = LOG_VERBOSE;
  }      
  
  size = sizeof(logFile);
  ret = RegQueryValueEx(hKey, "logfile", 0, &type, logFile, &size);
  if ((type != REG_SZ) || (ret != ERROR_SUCCESS)) {
    logFile[0] = '\0';
  }      
}      
      



static void write(char const *fmt, ...) 
{  
  va_list args;
  char buf[1024];
  DWORD written;
  int len;

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  buf[sizeof(buf)-1] = '\0';
  len = strlen(buf);
  
  WriteFile(hStdout, buf, len, &written, NULL);
  
  if (hFile != INVALID_HANDLE_VALUE) {
    WriteFile(hFile, buf, len, &written, NULL);
  }
}


__declspec(dllexport) int handlerLoad(HKEY hKey)
{
  load_config(hKey);
  
  
  if (!AllocConsole()) {
    MessageBox(0, "AllocConsole failed!\n", "logcon", 0);
    return 1;
  }
  
  SetConsoleTitle("LINE Log Window");
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
  
  hFile = INVALID_HANDLE_VALUE;
  if (logFile[0] != '\0') {
    hFile = CreateFile(logFile, GENERIC_WRITE, 
                       FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, 
                       OPEN_ALWAYS, 0, 0);
  }
  
  return 0;
}


__declspec(dllexport) void handlerMsg(struct cooked_log_msg *m)
{
  if (m->level < minLevel) return;
  
  switch (m->source) {
  case LOG_LINE:
    write("   Line:%2d", m->id);
    break;
  case LOG_LINEXEC:
    write("Linexec:%2d", m->id);
    break;
  default:
    write("?:%3d", m->id);
    break; 
  }

  write("|%x| ", m->raw->pid);

  switch (m->level) {
  case LOG_ERROR:
    write("<ERROR> ");
    break;
  case LOG_WARN:
    write("<WARNING> ");
    break;
  case LOG_NORMAL:
    break;
  case LOG_VERBOSE:
    write("<verb> ");
    break;
  case LOG_DEBUG:
  default:
    write("<dbug> ");
    break;
  }

  write("%s\n", m->raw->msg);
}


static void show_help(void)
{
  printf("LINELog logcon.dll\n"
         "Copyright (C) 2001  Michael Vines\n"
         "http://line.sourceforge.net/\n\n"
         "This program is distributed in the hope that it will be useful,\n"
         "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
         "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
         "GNU General Public License for more details\n\n");

  printf("Configuration Options:\n"
         " -l level     Set the minimum log level to 'level'\n"
         "              (see src/common/log.h)\n"
         " -f filename  Also output log messages to this file (if filename is\n"
         "              not given then the current log file will be removed)\n"
         " -s           Show current configuration\n"
         " -h           Display this help and exit\n");
}



__declspec(dllexport) void handlerConfig(HKEY hKey, int argc, char *argv[])
{
  load_config(hKey);
  
  if (argc == 2) {
    if (0 == strcmp(argv[0], "-l")) {
      
      minLevel = atoi(argv[1]);
      save_config(hKey);
      
      return;
    } else if (0 == strcmp(argv[0], "-f")) {

      strncpy(logFile, argv[1], sizeof(logFile)-1);
      save_config(hKey);
      return;
    }
  } else if (argc == 1) {
    
    if (0 == strcmp(argv[0], "-s")) {
      printf("Log Level: %d\n", minLevel);
      printf("Log File: %s\n", logFile);
      return;
      
    } else if (0 == strcmp(argv[0], "-f")) {
      logFile[0] = '\0';
      save_config(hKey);
      return;
    }
  }
  
  show_help();
}


BOOL APIENTRY DllMain(HINSTANCE hModule, 
                      DWORD  ul_reason_for_call, 
                      LPVOID lpReserved)
{
  switch (ul_reason_for_call) {
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_ATTACH:
  case DLL_PROCESS_DETACH:
    break;
  }

  return TRUE;
}

