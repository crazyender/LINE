#include <stdio.h> /* sprintf */
#include <string.h>
#include <limits.h> /* INT_MAX */
#include "putty.h"
#define VERIFY(f)          ((void)(f))
#define CMD_TERM "cmd"
#define MAX_HISTORY 100
#define MAX_CMD_BUF 1024

static const char *     cmd_init(void *frontend_handle, void **backend_handle,
 Config *cfg,
 char *unused_host, int unused_port,
 char **realhost, int nodelay, int keepalive);
static void cmd_free(void *handle);
static void cmd_reconfig(void *handle, Config *cfg);
static int  cmd_send(void *handle, char *buf, int len);
static int  cmd_sendbuffer(void *handle);
static void cmd_size(void *handle, int width, int height);
static void cmd_special(void *handle, Telnet_Special code);
static const struct telnet_special *    
			cmd_get_specials(void *handle);
static int  cmd_connected(void *handle);
static int  cmd_exitcode(void *handle);
static int  cmd_sendok(void *handle);
static void cmd_unthrottle(void *handle, int backlog);
static int  cmd_ldisc(void *handle, int option);
static void cmd_provide_ldisc(void *handle, void *ldisc);
static void cmd_provide_logctx(void *handle, void *logctx);
static int  cmd_cfg_info(void *handle);
static BOOL StartChildProcess(void* local, 
				char* lpszCmdLine, BOOL bShowChildWindow);
static BOOL IsChildRunning(void* local);
static void TerminateChildProcess(void* local);
static HANDLE PrepAndLaunchRedirectedChild(char* lpszCmdLine,
				PROCESS_INFORMATION* ppi,
 				HANDLE hStdOut, HANDLE hStdIn, HANDLE hStdErr,
 				BOOL bShowChildWindow);
static  void WINAPI cygterm_readstdin_thread(void* param);
static  void WINAPI cygterm_readstderr_thread(void* param);
static  void WINAPI cygterm_watch_child_process(void* param);
static void cmd_convert_to_cmd_format(char* src, int len, char* des, int deslen);

typedef struct _histroy_entry
{
	LIST_ENTRY node;
	char cmd[MAX_CMD_BUF];
}histroy_entry;

typedef struct _cmd_backend_data {
	void *frontend;
	PROCESS_INFORMATION pi;
	HANDLE pin;
    HANDLE pout;
	Config cfg;
	int bufsize;
	int editing;
    int echoing;
	int exitcode;
	char semdcmd[MAX_CMD_BUF];
	LIST_ENTRY histroy;
	PLIST_ENTRY current_histroy;
	int historycount;
	HANDLE m_hStdIn;
	HANDLE m_hStdOut;
	HANDLE m_hStdErr;
	HANDLE m_hStdInWrite;
	HANDLE m_hStdOutRead;
	HANDLE m_hStdErrRead;
	HANDLE m_hChildProcess;
	HANDLE m_hStdOutThread;
	HANDLE m_hStdErrThread;
	HANDLE m_hProcessThread;
	HANDLE m_hExitEvent;
	HANDLE m_bRunThread;
	HANDLE m_hWriteEvent;
} cmd_backend_data, *pcmd_backend_data;



Backend cmd_backend = {
    cmd_init,
	cmd_free,
	cmd_reconfig,
	cmd_send,
	cmd_sendbuffer,
	cmd_size,
	cmd_special,
	cmd_get_specials,
	cmd_connected,
	cmd_exitcode,
	cmd_sendok,
	cmd_ldisc,
	cmd_provide_ldisc,
	cmd_provide_logctx,
	cmd_unthrottle,
	cmd_cfg_info,
	1
};

static void my_print(const char* fmt, ... )
{
	va_list ap;
    char buf[1025] = {0};
	int len;
	DWORD n;

	va_start(ap, fmt);
	len = vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	OutputDebugStringA(buf);
}

static void cmd_add_histroy(pcmd_backend_data local, char* cmd)
{
	histroy_entry * entry = smalloc(sizeof(histroy_entry));
	memset(entry,0, sizeof(*entry));
	strcpy(entry->cmd, cmd);
	local->historycount++;
	if( local->histroy.Flink == NULL )
	{
		local->histroy.Flink = &entry->node;
		local->histroy.Blink = &entry->node;
		entry->node.Flink = &local->histroy;
		entry->node.Blink = &local->histroy;
	}
	else
	{
		entry->node.Blink = local->histroy.Blink;
		entry->node.Flink = &local->histroy;
		local->histroy.Blink->Flink = &entry->node;
		local->histroy.Blink = &entry->node;
	}


}

// get the older one, if no current have been selected, choose newest one
static char* cmd_get_previeous_histroy(pcmd_backend_data local)
{
	histroy_entry* entry = NULL;
	if( local->historycount == 0 )
		return "";
	if( local->current_histroy == NULL )
	{
		local->current_histroy = local->histroy.Blink;
	}
	else
	{
		local->current_histroy = local->current_histroy->Blink;
		if( local->current_histroy == &local->histroy )
			local->current_histroy = local->current_histroy->Blink;
	}
	entry = (histroy_entry*)CONTAINING_RECORD(local->current_histroy, histroy_entry, node);
	return entry->cmd;

}
// get the newer one, if no current have been selected, choose oldest one
static char* cmd_get_next_histroy(pcmd_backend_data local)
{
	histroy_entry* entry = NULL;
	if( local->historycount == 0 )
		return "";
	if( local->current_histroy == NULL )
	{
		local->current_histroy = local->histroy.Flink;
	}
	else
	{
		local->current_histroy = local->current_histroy->Flink;
		if( local->current_histroy == &local->histroy )
			local->current_histroy = local->current_histroy->Flink;
	}
	entry = (histroy_entry*)CONTAINING_RECORD(local->current_histroy, histroy_entry, node);
	return entry->cmd;
}

static char* cmd_get_newest_histroy(pcmd_backend_data local)
{
	histroy_entry* entry = NULL;
	if( local->historycount == 0 )
		return "";
	entry = (histroy_entry*)CONTAINING_RECORD(local->histroy.Blink, histroy_entry, node);
	return entry->cmd;
}

static int cmd_histroy_count(pcmd_backend_data local)
{
	return local->historycount;
}

static void cmd_rollback_until_none(pcmd_backend_data local)
{
	int len = strlen(local->semdcmd);
	for( len; len >0; len --)
	{
		char buf[1];
		buf[0] = 127;
		from_backend(local->frontend, 0, buf, 1);
	}
	local->semdcmd[0] = '\0';
}


static const char *     cmd_init(void *frontend_handle, void **backend_handle,
 Config *cfg,
 char *unused_host, int unused_port,
 char **realhost, int nodelay, int keepalive)
{
    pcmd_backend_data local = NULL;
    if( !backend_handle || !cfg || !realhost )
        return "invalid parameter";
    local = snew(cmd_backend_data);
    strcpy(*realhost = smalloc(sizeof CMD_TERM), CMD_TERM);
    local->frontend = frontend_handle;
    local->cfg = *cfg;
	local->echoing = 0;
	local->editing = 0;
	memset(local->semdcmd, 0, sizeof(local->semdcmd));
	local->historycount = 0;
	local->histroy.Blink = local->histroy.Flink = NULL;
	local->current_histroy = NULL;
    *backend_handle = local;
	cmd_add_histroy(local, "");
	StartChildProcess(local, cfg->host, FALSE, &local->pi);
    return 0;
}

static void cmd_free(void *handle)
{
    sfree(handle);
    return 0;
}

static void cmd_reconfig(void *handle, Config *cfg)
{
    pcmd_backend_data local = NULL;
    if( !handle )
        return;
    local = (pcmd_backend_data)handle;
    local->cfg = *cfg;

}

static char cmdbuf[1024] = {0};
static int  cmd_send(void *handle, char *buf, int len)
{
	pcmd_backend_data local = handle;
	DWORD writed;
	char *tmp = smalloc(len+1);
	memset(tmp,0,len+1);
	if( !handle || !buf || !len )
		return 0;
	if( strstr(buf, "\r\n") == NULL )
	{
		if( len == 1 && buf[0] == 127 ) //backspace
		{
			int l = strlen(local->semdcmd);
			if( l != 0 )
			{
				from_backend(local->frontend, 0, buf, len);
				l = l-1;
			}
			local->semdcmd[l] = '\0';
			local->bufsize = 1;
		}
		else if( buf[0] == 27 )
		{
			// left || right || up || down
			char c = buf[2];
			if( c == 'A' || c == 'B' ) // up and down
			{
				cmd_rollback_until_none(local);
				if( local->historycount != 0 )
				{
					char* his_buf = (c == 'A') ? cmd_get_previeous_histroy(local) : cmd_get_next_histroy(local);
					strcpy(local->semdcmd, his_buf);
					from_backend(local->frontend, 0, local->semdcmd, strlen(local->semdcmd));
				}
			}
			else
			{
				from_backend(local->frontend, 0, buf+1, 2);
				memcpy(tmp, buf+1, 2);
				strcat(local->semdcmd, tmp);
				local->bufsize = 2;
			}
		}
		else
		{
			from_backend(local->frontend, 0, buf, len);
			memcpy(tmp, buf, len);
			strcat(local->semdcmd, tmp);
			local->bufsize = len;
		}
		
	}
	else
	{

		char sendcmd[MAX_CMD_BUF];
		strcpy(sendcmd, local->semdcmd);
		cmd_rollback_until_none(local);

		WriteFile(local->m_hStdInWrite, sendcmd, strlen(sendcmd), &writed, NULL);
		local->bufsize = writed;
		WriteFile(local->m_hStdInWrite, "\n", 1, &writed, NULL);
		local->bufsize += writed;
		cmd_add_histroy(local, sendcmd);
		local->current_histroy = NULL;
	}

	sfree(tmp);
	return local->bufsize;
}

static int  cmd_sendbuffer(void *handle)
{
    return 0;
}

static void cmd_size(void *handle, int width, int height)
{

}
static void cmd_special(void *handle, Telnet_Special code)
{
}

static const struct telnet_special *    cmd_get_specials(void *handle)
{
    return NULL;
}

static int  cmd_connected(void *handle)
{
    return 1;
}
static int  cmd_exitcode(void *handle)
{
    return 0;
}

static int  cmd_sendok(void *handle)
{
    return 1;
}

static void cmd_unthrottle(void *handle, int backlog)
{
}

static int  cmd_ldisc(void *handle, int option)
{
	return 0;
}

static void cmd_provide_ldisc(void *handle, void *ldisc)
{
}

static void cmd_provide_logctx(void *handle, void *logctx)
{
}

static int  cmd_cfg_info(void *handle)
{
    return 0;
}




BOOL StartChildProcess(void* handle, char* lpszCmdLine, BOOL bShowChildWindow, PROCESS_INFORMATION* ppi)
{
	pcmd_backend_data local = handle;
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hStdInWriteTmp, hStdOutReadTmp, hStdErrReadTmp;
	DWORD dwThreadID;
	// Set up the security attributes struct.
	SECURITY_ATTRIBUTES sa;
	ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength= sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;


	// Create the child stdin pipe.
	VERIFY(CreatePipe(&local->m_hStdIn, &hStdInWriteTmp, &sa, 0));

	// Create the child stdout pipe.
	VERIFY(CreatePipe(&hStdOutReadTmp, &local->m_hStdOut, &sa, 0));

	// Create the child stderr pipe.
	VERIFY(CreatePipe(&hStdErrReadTmp, &local->m_hStdErr, &sa, 0));

	// Create new stdin write, stdout and stderr read handles.
	// Set the properties to FALSE. Otherwise, the child inherits the
	// properties and, as a result, non-closeable handles to the pipes
	// are created.

	VERIFY(DuplicateHandle(hProcess, hStdInWriteTmp,
		hProcess, &local->m_hStdInWrite, 0, FALSE, DUPLICATE_SAME_ACCESS));

	VERIFY(DuplicateHandle(hProcess, hStdOutReadTmp,
		hProcess, &local->m_hStdOutRead, 0, FALSE, DUPLICATE_SAME_ACCESS));

	VERIFY(DuplicateHandle(hProcess, hStdErrReadTmp,
		hProcess, &local->m_hStdErrRead, 0, FALSE, DUPLICATE_SAME_ACCESS));

	// Close inheritable copies of the handles you do not want to be
	// inherited.

	VERIFY(CloseHandle(hStdInWriteTmp));
	VERIFY(CloseHandle(hStdOutReadTmp));
	VERIFY(CloseHandle(hStdErrReadTmp));

	// Start child process with redirected stdout, stdin & stderr
	local->m_hChildProcess = PrepAndLaunchRedirectedChild(lpszCmdLine, ppi,
		local->m_hStdOut, local->m_hStdIn, local->m_hStdErr, bShowChildWindow);

	if (local->m_hChildProcess == NULL)
	{
		// close all handles and return FALSE
		VERIFY(CloseHandle(local->m_hStdIn));
		local->m_hStdIn = NULL;
		VERIFY(CloseHandle(local->m_hStdOut));
		local->m_hStdOut = NULL;
		VERIFY(CloseHandle(local->m_hStdErr));
		local->m_hStdErr = NULL;

		return FALSE;
	}


	local->m_bRunThread = TRUE;

	// Create Exit event
	local->m_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	VERIFY(local->m_hExitEvent != NULL);

	// Launch the thread that read the child stdout.
	local->m_hStdOutThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cygterm_readstdin_thread,
		(LPVOID)local, 0, &dwThreadID);
	VERIFY(local->m_hStdOutThread != NULL);

	SetStdHandle(STD_OUTPUT_HANDLE, local->m_hStdInWrite);
	SetStdHandle(STD_INPUT_HANDLE, local->m_hStdOutRead);
	//// Launch the thread that read the child stderr.
	local->m_hStdErrThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cygterm_readstderr_thread,
		(LPVOID)local, 0, &dwThreadID);
	VERIFY(local->m_hStdErrThread != NULL);

	// Launch the thread that monitoring the child process.
	local->m_hProcessThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)cygterm_watch_child_process,
		(LPVOID)local, 0, &dwThreadID);
	VERIFY(local->m_hProcessThread != NULL);

	local->m_hWriteEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

	return TRUE;

}

static BOOL IsChildRunning(pcmd_backend_data local)
{
	DWORD dwExitCode;
	if (local->m_hChildProcess == NULL) return FALSE;
	GetExitCodeProcess(local->m_hChildProcess, &dwExitCode);
	return (dwExitCode == STILL_ACTIVE) ? TRUE: FALSE;

}

static void TerminateChildProcess(pcmd_backend_data local)
{
	local->m_bRunThread = FALSE;
	SetEvent(local->m_hExitEvent);
	SetEvent(local->m_hWriteEvent);
	CloseHandle(local->m_hWriteEvent);
	Sleep(500);

	// Check the process thread.
	if (local->m_hProcessThread != NULL)
	{
		VERIFY(WaitForSingleObject(local->m_hProcessThread, 1000) != WAIT_TIMEOUT);
		local->m_hProcessThread = NULL;
	}

	// Close all child handles first.
	if (local->m_hStdIn != NULL)
		VERIFY(CloseHandle(local->m_hStdIn));
	local->m_hStdIn = NULL;
	if (local->m_hStdOut != NULL)
		VERIFY(CloseHandle(local->m_hStdOut));
	local->m_hStdOut = NULL;
	if (local->m_hStdErr != NULL)
		VERIFY(CloseHandle(local->m_hStdErr));
	local->m_hStdErr = NULL;
	Sleep(100);

	// Close all parent handles.
	if (local->m_hStdInWrite != NULL)
		VERIFY(CloseHandle(local->m_hStdInWrite));
	local->m_hStdInWrite = NULL;
	if (local->m_hStdOutRead != NULL)
		VERIFY(CloseHandle(local->m_hStdOutRead));
	local->m_hStdOutRead = NULL;
	if (local->m_hStdErrRead != NULL)
		VERIFY(CloseHandle(local->m_hStdErrRead));
	local->m_hStdErrRead = NULL;
	Sleep(100);

	// Stop the stdout read thread.
	if (local->m_hStdOutThread != NULL)
	{
		TerminateThread(local->m_hStdOutThread, 1);
		VERIFY(WaitForSingleObject(local->m_hStdOutThread, 1000) != WAIT_TIMEOUT);
		local->m_hStdOutThread = NULL;
	}

	// Stop the stderr read thread.
	if (local->m_hStdErrThread != NULL)
	{
		TerminateThread(local->m_hStdErrThread, 1);
		VERIFY(WaitForSingleObject(local->m_hStdErrThread, 1000) != WAIT_TIMEOUT);
		local->m_hStdErrThread = NULL;
	}
	Sleep(100);

	// Stop the child process if not already stopped.
	// It's not the best solution, but it is a solution.
	// On Win98 it may crash the system if the child process is the COMMAND.COM.
	// The best way is to terminate the COMMAND.COM process with an "exit" command.

	if (IsChildRunning(local))
	{
		VERIFY(TerminateProcess(local->m_hChildProcess, 1));
		VERIFY(WaitForSingleObject(local->m_hChildProcess, 1000) != WAIT_TIMEOUT);
	}
	local->m_hChildProcess = NULL;

	// cleanup the exit event
	if (local->m_hExitEvent != NULL)
		VERIFY(CloseHandle(local->m_hExitEvent));
	local->m_hExitEvent = NULL;

}

static HANDLE PrepAndLaunchRedirectedChild(char* lpszCmdLine,PROCESS_INFORMATION* ppi,
	HANDLE hStdOut, HANDLE hStdIn, HANDLE hStdErr,
	BOOL bShowChildWindow)
{
	HANDLE hProcess =  GetCurrentProcess();
	LPVOID lpSD = NULL;

	PROCESS_INFORMATION pi = *ppi;
	// Create the NULL security token for the process
	LPSECURITY_ATTRIBUTES lpSA = NULL;
	BOOL bResult;
	// Set up the start up info struct.
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdOutput = hStdOut;
	si.hStdInput  = hStdIn;
	si.hStdError  = hStdErr;

	// Use this if you want to show the child.
	si.wShowWindow = bShowChildWindow ? SW_SHOW: SW_HIDE;
	// Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
	// use the wShowWindow flags.


	// On NT/2000 the handle must have PROCESS_QUERY_INFORMATION access.
	// This is made using an empty security descriptor. It is not the same
	// as using a NULL pointer for the security attribute!

	{
		lpSD = GlobalAlloc(GPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
		VERIFY(InitializeSecurityDescriptor(lpSD, SECURITY_DESCRIPTOR_REVISION));
		VERIFY(SetSecurityDescriptorDacl(lpSD, -1, 0, 0));

		lpSA = (LPSECURITY_ATTRIBUTES) GlobalAlloc(GPTR, sizeof(SECURITY_ATTRIBUTES));
		lpSA->nLength = sizeof(SECURITY_ATTRIBUTES);
		lpSA->lpSecurityDescriptor = lpSD;
		lpSA->bInheritHandle = TRUE;
	}

	// Try to spawn the process.
	bResult = CreateProcess(NULL, (char*)lpszCmdLine, lpSA, NULL, TRUE,
		CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

	// Cleanup memory allocation
	if (lpSA != NULL)
		GlobalFree(lpSA);
	if (lpSD != NULL)
		GlobalFree(lpSD);

	// Return if an error occurs.
	if (!bResult) return FALSE;

	// Close any unnecessary handles.
	VERIFY(CloseHandle(pi.hThread));

	// Save global child process handle to cause threads to exit.
	return pi.hProcess;

}

static DWORD cmd_on_receive_data( pcmd_backend_data local, HANDLE h) 
{
	DWORD nBytesRead;
	char lpszBuffer[256+1];
	char *readBuf = NULL;
	char* sendcmd = NULL;
	int readlen = 0;
	char * descmd = NULL;
	int deslen = 0;
	DWORD total = 0;
	DWORD byteleft = 0;


	if (!ReadFile(h, lpszBuffer, 256,
		&nBytesRead, NULL) || !nBytesRead)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
			return ERROR_BROKEN_PIPE;			// pipe done - normal exit path.
	}
	// data on!
	WaitForSingleObject(local->m_hWriteEvent, INFINITE);
	do {
		if (nBytesRead)
		{
			lpszBuffer[nBytesRead] = '\0';
			my_print("[ondata]%x, %d", h, nBytesRead);
			readBuf = lpszBuffer;
			readlen = nBytesRead;
			deslen = readlen*2;
			descmd = smalloc(deslen);
			memset(descmd, 0, deslen);
			cmd_convert_to_cmd_format(readBuf, readlen, descmd, deslen);
			from_backend(local->frontend, 0, descmd, deslen);
			sfree(descmd);
		}

		if( !PeekNamedPipe(h, lpszBuffer, 256, &nBytesRead, &total, &byteleft) )
		{
			if (GetLastError() == ERROR_BROKEN_PIPE)
				return ERROR_BROKEN_PIPE;			// pipe done - normal exit path.
		}
		if( byteleft == 0 )
		{
			SetEvent(local->m_hWriteEvent);
			break;
		}

		if (!ReadFile(h, lpszBuffer, 256,
			&nBytesRead, NULL) || !nBytesRead)
		{
			if (GetLastError() == ERROR_BROKEN_PIPE)
				return ERROR_BROKEN_PIPE;			// pipe done - normal exit path.
		}
	}while(1);

	return ERROR_SUCCESS;
}

static void WINAPI cmd_data_receive_thread( void* param, HANDLE h ) 
{
	pcmd_backend_data local = param;

	while (local->m_bRunThread)
	{
		DWORD error;
		error = cmd_on_receive_data(local, h);
		if (error == ERROR_BROKEN_PIPE)
		{
			SetEvent(local->m_hWriteEvent);
			break;			// pipe done - normal exit path.
		}
	}
	return 0;
}

static  void WINAPI cygterm_readstdin_thread(void* param)
{
	pcmd_backend_data local = param;
	return cmd_data_receive_thread(param, local->m_hStdOutRead);

}

static  void WINAPI cygterm_readstderr_thread(void* param)
{
	pcmd_backend_data local = param;
	return cmd_data_receive_thread(param, local->m_hStdErrRead);
}

static void cmd_convert_to_cmd_format(char* src, int len, char* des, int deslen)
{
	int index = 0;
	if( !src || !len || !des || !deslen )
		return;
	for( index = 0; index < len; index++ )
	{
		if( src[index] == '\n')
		{
			if( (index == 0) || (src[index-1] != '\r'))
			{
				*des++ = '\r';
				*des++ = '\n';
			}
			else
				*des++ = src[index];
		}
		else
		{
			*des++ = src[index];
		}
	}
}

static  void WINAPI cygterm_watch_child_process(void* param)
{
	pcmd_backend_data local = param;
	char* errormsg = "Child process exit!";
	DWORD ret = WaitForSingleObject(local->m_hChildProcess, INFINITE );
	from_backend(local->frontend, 0, errormsg, strlen(errormsg));
}