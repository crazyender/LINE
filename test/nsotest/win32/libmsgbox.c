/**
 * The Windows version of libmsgbox.so
 * $Id: libmsgbox.c,v 1.1.1.1 2001/03/07 18:34:18 mvines Exp $
 */ 

#include <windows.h>
#include <cygwin/cygwin_dll.h>

DECLARE_CYGWIN_DLL(DllMain);


__declspec(dllexport) void msgbox(char *msg)
{
  MessageBox(NULL, msg, "libmsgbox", MB_OK);
}


__declspec(dllexport) void msgbox2(char *title, char *msg)
{
  MessageBox(NULL, msg, title, MB_OK);
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

