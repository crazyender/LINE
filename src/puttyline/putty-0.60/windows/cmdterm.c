#include <stdio.h> /* sprintf */
#include <string.h>
#include <limits.h> /* INT_MAX */
#include "putty.h"
#define CMD_TERM "cmd"

static const char *                     cmd_init(void *frontend_handle, void **backend_handle,
                                             Config *cfg,
                                             char *unused_host, int unused_port,
                                             char **realhost, int nodelay, int keepalive);
static void                             cmd_free(void *handle);
static void                             cmd_reconfig(void *handle, Config *cfg);
static int                              cmd_send(void *handle, char *buf, int len);
static int                              cmd_sendbuffer(void *handle);
static void                             cmd_size(void *handle, int width, int height);
static void                             cmd_special(void *handle, Telnet_Special code);
static const struct telnet_special *    cmd_get_specials(void *handle);
static int                              cmd_connected(void *handle);
static int                              cmd_exitcode(void *handle);
static int                              cmd_sendok(void *handle);
static void                             cmd_unthrottle(void *handle, int backlog);
static int                              cmd_ldisc(void *handle, int option);
static void                             cmd_provide_ldisc(void *handle, void *ldisc);
static void                             cmd_provide_logctx(void *handle, void *logctx);
static int                              cmd_cfg_info(void *handle);

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



static const char *                     cmd_init(void *frontend_handle, void **backend_handle,
                                             Config *cfg,
                                             char *unused_host, int unused_port,
                                             char **realhost, int nodelay, int keepalive)
{
    pcmd_backend_data obj = NULL;
    if( !frontend_handle || !backend_handle || !cfg || !realhost )
        return "invalid parameter";
    obj = snew(cmd_backend_data);
    strcpy(*realhost = smalloc(sizeof CMD_TERM), CMD_TERM);
    obj->frontend = frontend_handle;
    obj->cfg = *cfg;
    *backend_handle = obj;
    return 0;
}

static void                             cmd_free(void *handle)
{
    sfree(handle);
    return 0;
}

static void                             cmd_reconfig(void *handle, Config *cfg)
{
    pcmd_backend_data local = null;
    if( !handle )
        return;
    local = (pcmd_backend_data)handle;
    local->cfg = *cfg;

}

static int                              cmd_send(void *handle, char *buf, int len)
{
	if( !handle || !buf || !len )
		return 0;

	return len;
}

static int                              cmd_sendbuffer(void *handle)
{
    return 0;
}

static void                             cmd_size(void *handle, int width, int height)
{

}
static void                             cmd_special(void *handle, Telnet_Special code)
{
}

static const struct telnet_special *    cmd_get_specials(void *handle)
{
    return NULL;
}

static int                              cmd_connected(void *handle)
{
    return 1;
}
static int                              cmd_exitcode(void *handle)
{
    return 0;
}

static int                              cmd_sendok(void *handle)
{
    return 1;
}

static void                             cmd_unthrottle(void *handle, int backlog)
{
}

static int                              cmd_ldisc(void *handle, int option)
{
}

static void                             cmd_provide_ldisc(void *handle, void *ldisc)
{
}

static void                             cmd_provide_logctx(void *handle, void *logctx)
{
}

static int                              cmd_cfg_info(void *handle)
{
    return 0;
}

