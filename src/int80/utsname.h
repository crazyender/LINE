#ifndef _LINUX_UTSNAME_H
#define _LINUX_UTSNAME_H

#define UMANE_LINE_SYSNAME "Linux"
#define UNAME_LINUX_RELEASE "2.6.32-31-generic"
#define UNAME_LINE_VERSION "2011-09-07 10:10"
#define UNAME_LINE_MACHINE "i686"
#define UNAME_LINE_NODE "ENDER_DELL"

#define __OLD_UTS_LEN 8

struct oldold_utsname {
	char sysname[9];
	char nodename[9];
	char release[9];
	char version[9];
	char machine[9];
};

#define __NEW_UTS_LEN 64

struct old_utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
};

struct new_utsname {
	char sysname[__NEW_UTS_LEN + 1];
	char nodename[__NEW_UTS_LEN + 1];
	char release[__NEW_UTS_LEN + 1];
	char version[__NEW_UTS_LEN + 1];
	char machine[__NEW_UTS_LEN + 1];
	char domainname[__NEW_UTS_LEN + 1];
};


#endif /* _LINUX_UTSNAME_H */
