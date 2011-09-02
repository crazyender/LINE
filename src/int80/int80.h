/*
 * $Id: int80.h,v 1.1 2001/04/25 18:33:24 mvines Exp $
 */

#define FILE_DEVICE_HOOKINT  0x00008300
#define DRIVER_DEVICE_NAME   L"LinuxSyscallRedirector"
#define HOOKINT_IOCTL_INDEX  0x830
#define ADDINT_IOCTL_INDEX  0x831
#define REMOVEINT_IOCTL_INDEX  0x832
#define CALLPORT_IOCTL_INDEX  0x833
#define IOCTL_HOOKINT_SYSTEM_SERVICE_USAGE     CTL_CODE(FILE_DEVICE_HOOKINT,  \
                                               HOOKINT_IOCTL_INDEX,  \
                                               METHOD_BUFFERED,       \
                                               FILE_ANY_ACCESS)
#define IOCTL_ADDINT_SYSTEM_SERVICE_USAGE     CTL_CODE(FILE_DEVICE_HOOKINT,  \
                                               ADDINT_IOCTL_INDEX,  \
                                               METHOD_BUFFERED,       \
                                               FILE_ANY_ACCESS)
#define IOCTL_REMOVEINT_SYSTEM_SERVICE_USAGE     CTL_CODE(FILE_DEVICE_HOOKINT,  \
                                               REMOVEINT_IOCTL_INDEX,  \
                                               METHOD_BUFFERED,       \
                                               FILE_ANY_ACCESS)
#define IOCTL_CALLPORT_SYSTEM_SERVICE_USAGE     CTL_CODE(FILE_DEVICE_HOOKINT,  \
                                               CALLPORT_IOCTL_INDEX,  \
                                               METHOD_BUFFERED,       \
                                               FILE_ANY_ACCESS)

#define SERVICECOUNTERS_BUFSIZE 0x1000
