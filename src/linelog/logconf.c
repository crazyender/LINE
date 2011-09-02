/**
 * LINELog config tool
 * $Id: logconf.c,v 1.1 2001/03/16 20:38:26 mvines Exp $
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
#include <stdlib.h> 
#include <string.h> 
#include <windows.h> 
#include <getopt.h>

#include "reg.h"
#include "log.h"
#include "logapi.h"


HKEY hLogKey;
  

static int configHandler(char *name, int argc, char *argv[])
{
  HKEY hKey;
  char file[MAX_PATH];
  DWORD type, size, result;
  int retval = 0;
  HINSTANCE hDll;
  type_handlerConfig handlerConfig;

  result = RegOpenKeyEx(hLogKey, name, 0, KEY_READ | KEY_WRITE, &hKey);
  if (ERROR_SUCCESS != result) {
    printf("Unable to open handler registry key for '%s' (error %ld)\n", 
           name, result);
    return 1;
  }

  do {
    size = sizeof(file);

    if (ERROR_SUCCESS != RegQueryValueEx(hKey, REGVAL_HANDLER_DLL, NULL, &type, 
                                         (LPBYTE)file, &size)) {
      printf("Error reading handler reginfo for '%s' (error %ld)\n", 
             name, GetLastError());
      retval = 1;
      break;
    }

    if (REG_SZ != type) {
      printf("Got wrong value type when reading handler reginfo for '%s' (error %ld)\n", 
             name, GetLastError());
      retval = 1;
      break;
    }

    /* pass in the hKey so the DLL can read it's configuration if necessary */
    hDll = LoadLibrary(file);
    do {
      if (NULL == hDll) {
        printf("Unable to load '%s' (error %ld)\n", file, GetLastError());
        break;
      }

      handlerConfig = (type_handlerConfig)GetProcAddress(hDll, "handlerConfig");

      if (NULL == handlerConfig) {
        printf("Error loading symbol 'handlerConfig' from '%s'\n", file);
        break;
      }

      handlerConfig(hKey, argc, argv); 
    } while (0);

  } while (0);

  RegCloseKey(hKey);  
  return retval;
}


static int regHandler(char *h)
{
  char fullpath[MAX_PATH];
  char *c;
  HKEY hKey;
  DWORD err;
  
  cygwin_conv_to_full_win32_path(h, fullpath);
  
  c = fullpath + strlen(fullpath);
  
  while (1) {
    if (c < fullpath) {
      c = fullpath;
      break;
    }
    
    if ('\\' == *c) {
      c++;
      break;
    }
    c--;
  }

    
  printf("Registering '%s' (%s)\n", c, fullpath);

  err = RegCreateKeyEx(hLogKey, c, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
  if (ERROR_SUCCESS != err) {
    printf("Unable to create registry key (error %ld)\n", err);
    return -1;
  }
  
  err = RegSetValueEx(hKey, REGVAL_HANDLER_DLL, 0, REG_SZ, 
                      fullpath, strlen(fullpath));
  if (ERROR_SUCCESS != err) {
    printf("Error writing registry value (error %ld)\n", err);                                   
  }                                     
  
  
  CloseHandle(hKey);
  return 0;
}


static int unregHandler(char *h)
{
  LONG result;
  
  result = RegDeleteKey(hLogKey, h);
  if (ERROR_SUCCESS == result) {
    printf("Handler '%s' unregistered.\n", h);
    return 0;
  }
  
  if (ERROR_FILE_NOT_FOUND == result) {
    printf("Handler not found: %s\n", h);
  } else {
    printf("Error occured while unregistering '%s': Win32 error %ld\n", 
           h, result);
  }
  return 1;
}


static void listHandlers(void)
{
  char name[MAX_PATH];
  int i;
  int result;
  DWORD size;
  
  printf("Current handlers\n");
  printf("----------------\n");
 
  i = 0;
  while (1) {
    size = sizeof(name);
    result = RegEnumKeyEx(hLogKey, i, name, &size, NULL, NULL, NULL, NULL);

    if (ERROR_NO_MORE_ITEMS == result) break;
    if (result != ERROR_SUCCESS) {
      printf("Win32 error %d\n", result);
      break;
    }
    
    printf("%s\n", name);
    i++;
  }
  
  if (0 == i) {
    printf("(none)\n");
  }
}


/**
 * Returns zero if a linelog is not running
 */
static int queryLogger(void)
{
  return (FindWindow("LINELog", "LINELog") != NULL);
}


static void killLogger(void)
{
  SendMessage(FindWindow("LINELog", "LINELog"), WM_QUIT, 0, 0);
}


static void show_help(void)
{
  printf("LINELog Config Tool\n"
         "Copyright (C) 2001  Michael Vines\n"
         "http://line.sourceforge.net/\n\n"
         "This program is distributed in the hope that it will be useful,\n"
         "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
         "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
         "GNU General Public License for more details\n\n");

  printf("Usage: Linelog.exe [options]\n"
         "Options:\n"
         " -r,--reg dllname     Register this log handler DLL\n"
         " -u,--unreg handler   Unregister this log handler (handler must be a name\n"
         "                      shown in the output of the --list option)\n"
         " -l,--list            List all the currently registered handlers\n"
         " -c,--config handler [handler options]\n"
         "                      Run the specified handler configuration utility.\n"
         "                      Any other command line options will be passed to\n"
         "                      the handler.\n"
         " -q, --query          Check if LINELog is currently running.\n"
         " -k, --kill           Kill LINELog if it is currently running\n"
         " -h, --help           Display this help and exit\n");
}


int main(int argc, char *argv[]) 
{ 
  
  if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, REGKEY_LOGGER, 0, NULL,
                                      0, KEY_READ, NULL, &hLogKey, NULL)) {
    printf("Unable to create/open log registry key: %s\n", REGKEY_LOGGER);
    return -1;
  }
  

  while (1) {
    int option_index = 0;
    int c;
    
    static struct option long_options[] = {
      {"reg", 1, 0, 'r'},
      {"unreg", 1, 0, 'u'},
      {"list", 0, 0, 'l'},
      {"config", 1, 0, 'c'},
      {"query", 0, 0, 'q'},
      {"kill", 0, 0, 'k'},
      {"help", 0, 0, 'h'},
      {0,0,0,0}
    };
    
    c = getopt_long(argc, argv, "r:u:lc:qkh", long_options, &option_index);
    
    if (-1 == c) break;
    
    switch (c) {
    case 'r':
      killLogger();
      return regHandler(optarg);

    case 'u':
      killLogger();
      return unregHandler(optarg);
    
    case 'l':
      listHandlers();
      return 0;

    case 'c':
      killLogger();
      return configHandler(optarg, argc - optind, &argv[optind]);
      
    case 'h':
      show_help();
      return 0;
      
    case 'q':
      {
        int ret = queryLogger();

        if (ret) {
          printf("Running.\n");
        } else {
          printf("Not running.\n");
        }

        return ret;
      }

    case 'k':
      killLogger();
      return 0;
      
    default:
      printf("Unknown option '%c'\n", c);
      return -1;
    }
  }     

  show_help();
  return 1;
} 
