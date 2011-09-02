/*
User notes.
==========

This file contains the prototypes and related data structures for the system
service layer. NTDDK.H must be included before including this header file.

All the system services in system service layer are callable both from user
mode and kernel mode (only at IRQL=PASSIVE_LEVEL). When calling from UserMode, 
Ntxx variant of the system service should be used, while calling from KernelMode, 
Zwxx variant of the system service should be used. The reason is, NTDLL.DLL 
(linked to user mode applications) exports functions in Ntxx form, whereas 
NTOSKRNL.EXE (linked to kernel mode) exports functions in Zwxx form.
*/

/*
The header file is written assuming that all the Zwxx variants are exported by
by NTOSKRNL.EXE. However, not all the variants are exported by NTOSKRNL. And
the number of variants exported changes in different versions. Hence if you
get a linking error, while using some of these variants from kernel mode driver,
you need to write a INT 2E wrapper code for this.

Find out the system service id and number of parameter bytes for the given service
using a kernel mode debugger such as Softice (use NTCALLS command) and write a 
wrapper as follows

_declspec(naked) NTSTATUS NTAPI Zwxx(param list)
{
	_asm {
		mov eax, serviceid
		lea edx, [esp+4]
		int 2eh
		ret parameterbytestopopoff
	}
}
*/

/*
If you plan to use this file from user mode application, make sure that you
include "undocnt.h" as follows.

#define _X86_
#include <ntddk.h>
#include "undocnt.h"

You can not have both windows.h and ntddk.h included from same C file, since
it results in datatypes redefinations.

If you want to use, both system services and win32 API, then put the code
using Win32 APIs in some other C file.

If you are using this file from kernel mode driver, there is not need to
include "#define _X86_" statement.
*/

/*
This header file is written assuming NTDDK.H file from Windows NT 4.0 DDK
is used. If you have later version of DDK such as Windows 2000 DDK, it is
possible that some of the previously undocumented calls are now documented by
Microsoft or some new data structures are documented by Microsoft. In this 
case, you may get redefination errors. In this case, you may modify UNDOCNT.H
file to suit your setup.

This file is compiled using Visual C++ 4.2
*/


#ifndef _UNDOCNT_H
#define _UNDOCNT_H
#pragma pack(1)
typedef struct ServiceDescriptorEntry {
	unsigned int *ServiceTableBase;
	unsigned int *ServiceCounterTableBase; //Used only in checked build
	unsigned int NumberOfServices;
	unsigned char *ParamTableBase;
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;

__declspec(dllimport) _stdcall KeAddSystemServiceTable(PVOID, PVOID, PVOID, PVOID, PVOID);
__declspec(dllimport)  ServiceDescriptorTableEntry_t KeServiceDescriptorTable;
//__declspec(dllimport) void *PsInitialSystemProcess;
__declspec(dllimport) ULONG NtBuildNumber;
__declspec(dllimport) KeAttachProcess(void *);
__declspec(dllimport) KeDetachProcess();

#ifdef _DBG
#define trace(Message) DbgPrint Message
#else
#define trace(Message)
#endif

NTSTATUS
DriverDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

#define MYDRIVERENTRY(DriverName, DeviceId, DriverSpecificInit) \
PDEVICE_OBJECT         deviceObject        = NULL; \
NTSTATUS               ntStatus; \
WCHAR                  deviceNameBuffer[]  = L"\\Device\\"##DriverName; \
UNICODE_STRING         deviceNameUnicodeString; \
WCHAR                  deviceLinkBuffer[]  = L"\\DosDevices\\"##DriverName; \
UNICODE_STRING         deviceLinkUnicodeString; \
\
RtlInitUnicodeString (&deviceNameUnicodeString, deviceNameBuffer); \
ntStatus = IoCreateDevice (DriverObject, \
			   0, \
			   &deviceNameUnicodeString, \
			   ##DeviceId, \
			   0, \
			   FALSE, \
			   &deviceObject \
			   ); \
\
if (NT_SUCCESS(ntStatus)) { \
   RtlInitUnicodeString (&deviceLinkUnicodeString, deviceLinkBuffer);\
   ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString, \
				    &deviceNameUnicodeString);\
   if (!NT_SUCCESS(ntStatus)) {\
       IoDeleteDevice (deviceObject); \
       return ntStatus; \
   } \
\
   ntStatus=##DriverSpecificInit; \
\
   if (!NT_SUCCESS(ntStatus)) {\
       IoDeleteDevice (deviceObject); \
       IoDeleteSymbolicLink(&deviceLinkUnicodeString); \
       return ntStatus; \
   } \
\
\
   DriverObject->MajorFunction[IRP_MJ_CREATE]         = \
   DriverObject->MajorFunction[IRP_MJ_CLOSE]          = \
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDispatch; \
   DriverObject->DriverUnload                         = DriverUnload; \
   return STATUS_SUCCESS; \
} else { \
   return ntStatus; \
};

NTSYSAPI 
NTSTATUS 
NTAPI 
KeI386AllocateGdtSelectors(
	PUSHORT pSelectorArray, 
	ULONG NumberOfSelectors
);

NTSYSAPI 
NTSTATUS 
NTAPI 
KeI386ReleaseGdtSelectors(
	PUSHORT pSelectorArray, 
	ULONG NumberOfSelectors
);

NTSYSAPI 
NTSTATUS 
NTAPI 
KeI386SetGdtSelector(
	ULONG Selector, 
	PVOID pDescriptor
);

NTSYSAPI 
NTSTATUS 
NTAPI 
RtlLocalTimeToSystemTime(PLARGE_INTEGER LocalTime, 
						 PLARGE_INTEGER SystemTime
);

NTSYSAPI 
NTSTATUS 
NTAPI 
RtlSystemTimeToLocalTime(PLARGE_INTEGER SystemTime, 
						 PLARGE_INTEGER LocalTime
);

typedef struct vad {
	void *StartingAddress;
	void *EndingAddress;
	struct vad *ParentLink;
	struct vad *LeftLink;
	struct vad *RightLink;
	ULONG Flags;
}VAD, *PVAD;

/* Maximum size of the message */
#define MAX_MESSAGE_DATA                0x130

/* Types of LPC messges */
#define UNUSED_MSG_TYPE                 0x00
#define LPC_REQUEST                     0x01
#define LPC_REPLY                       0x02
#define LPC_DATAGRAM                    0x03
#define LPC_LOST_REPLY                  0x04
#define LPC_PORT_CLOSED                 0x05
#define LPC_CLIENT_DIED                 0x06
#define LPC_EXCEPTION                   0x07
#define LPC_DEBUG_EVENT                 0x08
#define LPC_ERROR_EVENT                 0x09
#define LPC_CONNECTION_REQUEST  0x0A

/* Structure for the LPC message */
typedef struct LpcMessage {
	/* LPC Message Header */
	USHORT  ActualMessageLength;
	USHORT  TotalMessageLength;
	ULONG MessageType;
	ULONG ClientProcessId;
	ULONG ClientThreadId;
	ULONG MessageId;
	ULONG SharedSectionSize;

	/* LPC Message Data, taken care of maximum message */
	CCHAR  MessageData[MAX_MESSAGE_DATA];
} LPCMESSAGE, *PLPCMESSAGE;

/* Structures required for big LPC through shared section */
typedef struct Unknown1 {
	ULONG Length;
	HANDLE SectionHandle;
	ULONG Param1;
	ULONG SectionSize;
	ULONG ClientBaseAddress;
	ULONG ServerBaseAddress;
} LPCSECTIONINFO, *PLPCSECTIONINFO;

typedef struct Unknown2 {
	ULONG Length;
	ULONG SectionSize;
	ULONG ServerBaseAddress;
} LPCSECTIONMAPINFO, *PLPCSECTIONMAPINFO;
#pragma pack()

/* Undocumented LPC API */
NTSYSAPI
NTSTATUS
NTAPI
NtCreatePort(
	PHANDLE PortHandle, 
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG MaxConnectInfoLength, 
	ULONG MaxDataLength, 
	ULONG Unknown
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreatePort(
	PHANDLE PortHandle, 
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG MaxConnectInfoLength, 
	ULONG MaxDataLength, 
	ULONG Unknown
);

/*
 * MaxConnectInfoLength 
 * MaxDataLength - only validations
 * Unknown - unused
 */
NTSYSAPI
NTSTATUS
NTAPI
NtConnectPort(
	PHANDLE PortHandle, 
	PUNICODE_STRING PortName, 
	PULONG Unknown, /* Can not be NULL */
	PLPCSECTIONINFO Unknown1, /* Used in Big LPC */
	PLPCSECTIONMAPINFO Unknown2, /* Used in Big LPC */
	PVOID Unknown3, /* Can be NULL */
	PVOID ConnectInfo,
	PULONG pConnectInfoLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwConnectPort(
	PHANDLE PortHandle, 
	PUNICODE_STRING PortName, 
	PULONG Unknown, /* Can not be NULL */
	PLPCSECTIONINFO Unknown1, /* Used in Big LPC */
	PLPCSECTIONMAPINFO Unknown2, /* Used in Big LPC */
	PVOID Unknown3, /* Can be NULL */
	PVOID ConnectInfo,
	PULONG pConnectInfoLength
);

NTSYSAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePort(
	PHANDLE PortHandle, 
	PULONG Unknown ,
	PLPCMESSAGE pLpcMessageOut, 
	PLPCMESSAGE pLpcMessageIn
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePort(
	PHANDLE PortHandle, 
	PULONG Unknown ,
	PLPCMESSAGE pLpcMessageOut, 
	PLPCMESSAGE pLpcMessageIn
);

NTSYSAPI
NTSTATUS
NTAPI
NtAcceptConnectPort(
	PHANDLE PortHandle, 
	ULONG Unknown, // Pass 0
	PLPCMESSAGE pLpcMessage, 
	ULONG Unknown1, // 1 
	ULONG Unknown3, // 0
	PLPCSECTIONMAPINFO pSectionMapInfo
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAcceptConnectPort(
	PHANDLE PortHandle, 
	ULONG Unknown, // Pass 0
	PLPCMESSAGE pLpcMessage, 
	ULONG Unknown1, // 1 
	ULONG Unknown3, // 0
	PLPCSECTIONMAPINFO pSectionMapInfo
);

NTSYSAPI
NTSTATUS
NTAPI
NtCompleteConnectPort(
	HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCompleteConnectPort(
	HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
NtRequestWaitReplyPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessageIn,
	PLPCMESSAGE pLpcMessageOut
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestWaitReplyPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessageIn,
	PLPCMESSAGE pLpcMessageOut
);

NTSYSAPI
NTSTATUS
NTAPI
NtListenPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwListenPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessage
);

NTSYSAPI
NTSTATUS
NTAPI
NtRequestPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessage
);

NTSYSAPI
NTSTATUS
NTAPI
NtReplyPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessage
);


NTSYSAPI
NTSTATUS
NTAPI
NtRegisterThreadTerminatePort(
	HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRegisterThreadTerminatePort(
	HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetDefaultHardErrorPort(
	HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultHardErrorPort(
	HANDLE PortHandle
);


/* This system service does not seem to return any information about the port,
it gets pointer to port object using ObReferenceObjectByHandle and closes the
pointer and returns STATUS_SUCCESS */
NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationPort(
	HANDLE PortHandle, 
	ULONG InfoClass,
	PVOID Buffer,
	ULONG BufferSize,
	PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationPort(
	HANDLE PortHandle, 
	ULONG InfoClass,
	PVOID Buffer,
	ULONG BufferSize,
	PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
NtReplyWaitReplyPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReplyPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessage
);

NTSYSAPI
NTSTATUS
NTAPI
NtImpersonateClientOfPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateClientOfPort(
	HANDLE PortHandle, 
	PLPCMESSAGE pLpcMessage
);

//Windows 2000 only
NTSYSAPI
NTSTATUS
NTAPI
NtCreateWaitablePort(
	PHANDLE PortHandle, 
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG MaxConnectInfoLength, 
	ULONG MaxDataLength, 
	ULONG Unknown
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateWaitablePort(
	PHANDLE PortHandle, 
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG MaxConnectInfoLength, 
	ULONG MaxDataLength, 
	ULONG Unknown
);


typedef USHORT ATOM;
typedef PUSHORT PATOM;

typedef enum _ATOM_INFO_CLASS {
	SingleAtom,
	AllAtoms,
	MaxAtomInfoClass,
} ATOM_INFO_CLASS;

typedef struct AtomInfoSingle {
	USHORT ReferenceCount;
	USHORT Unknown;
	USHORT AtomStringLength;
	WCHAR AtomString[1];
} ATOMINFOSINGLE, *PATOMINFOSINGLE;

typedef struct AtomInfoAll {
	ULONG TotalNumberOfEntriesInGlobalAtomTable;
	ATOM AtomValues[1];
} ATOMINFOALL, *PATOMINFOALL;


#ifdef NT50
NTSYSAPI
NTSTATUS
NTAPI
NtAddAtom(
	IN PWCHAR pString,
	IN ULONG StringLength,
	OUT PATOM pAtom
);


NTSTATUS
NTAPI
ZwAddAtom(
	IN PWCHAR pString,
	IN ULONG StringLength,
	OUT PATOM pAtom
);

#else
NTSYSAPI
NTSTATUS
NTAPI
NtAddAtom(
	IN PWCHAR pString,
	OUT PATOM pAtom
);

NTSTATUS
NTAPI
ZwAddAtom(
	IN PWCHAR pString,
	OUT PATOM pAtom
);

#endif

NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationAtom(
	IN ATOM Atom,
	IN ATOM_INFO_CLASS AtomInfoClass,
	OUT PVOID AtomInfoBuffer,
	IN ULONG AtomInfoBufferLength,
	OUT PULONG BytesCopied
);


NTSTATUS
NTAPI
ZwQueryInformationAtom(
	IN ATOM Atom,
	IN ATOM_INFO_CLASS AtomInfoClass,
	OUT PVOID AtomInfoBuffer,
	IN ULONG AtomInfoBufferLength,
	OUT PULONG BytesCopied
);


#ifdef NT50
NTSYSAPI
NTSTATUS
NTAPI
NtFindAtom(
	IN PWCHAR pString,
	IN ULONG StringLength,
	OUT PATOM pAtom
);


NTSTATUS
NTAPI
ZwFindAtom(
	IN PWCHAR pString,
	IN ULONG StringLength,
	OUT PATOM pAtom
);

#else
NTSYSAPI
NTSTATUS
NTAPI
NtFindAtom(
	IN PWCHAR pString,
	OUT PATOM pAtom
);


NTSTATUS
NTAPI
ZwFindAtom(
	IN PWCHAR pString,
	OUT PATOM pAtom
);

#endif

NTSYSAPI
NTSTATUS
NTAPI
NtDeleteAtom(
	IN ATOM Atom
);


NTSTATUS
NTAPI
ZwDeleteAtom(
	IN ATOM Atom
);

NTSYSAPI
NTSTATUS
NTAPI
NtLoadDriver(
	IN PUNICODE_STRING DriverRegistryEntry
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
	IN PUNICODE_STRING DriverRegistryEntry
);


NTSYSAPI
NTSTATUS
NTAPI
NtUnloadDriver(
	IN PUNICODE_STRING DriverRegistryEntry
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadDriver(
	IN PUNICODE_STRING DriverRegistryEntry
);

NTSYSAPI
NTSTATUS
NTAPI
NtClose(
    IN HANDLE Handle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwClose(
    IN HANDLE Handle
    );


#define DUPLICATE_SAME_ACCESS	0x00000002

NTSYSAPI
NTSTATUS
NTAPI
NtDuplicateObject(
	IN HANDLE hSourceProcessHandle,
	IN HANDLE hSourceHandle,
	IN HANDLE hTargetProcessHandle,
	IN OUT PHANDLE hTargetHandle,
	IN ACCESS_MASK AccessMask,
	IN BOOLEAN bInheritHandle,
	IN ULONG dwOptions
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateObject(
	IN HANDLE hSourceProcessHandle,
	IN HANDLE hSourceHandle,
	IN HANDLE hTargetProcessHandle,
	IN OUT PHANDLE hTargetHandle,
	IN ACCESS_MASK AccessMask,
	IN BOOLEAN bInheritHandle,
	IN ULONG dwOptions
);


NTSYSAPI
NTSTATUS
NTAPI
NtCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateSymbolicLinkObject(
	OUT PHANDLE hSymbolicLink,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PUNICODE_STRING SymbolicLinkValue
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject(
	OUT PHANDLE hSymbolicLink,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PUNICODE_STRING SymbolicLinkValue
);

NTSYSAPI
NTSTATUS
NTAPI
NtMakeTemporaryObject(
	IN HANDLE hObject
);

NTSYSAPI
NTSTATUS
NTAPI
ZwMakeTemporaryObject(
	IN HANDLE hObject
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenDirectoryObject(
	OUT PHANDLE hDirectory,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject(
	OUT PHANDLE hDirectory,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);


typedef struct QueryDirectoryObjectBuffer_t {
	UNICODE_STRING DirectoryEntry;
	UNICODE_STRING DirectoryEntryType;
	char Buffer[1000];
} QUERYDIRECTORYOBJECTBUFFER, *PQUERYDIRECTORYOBJECTBUFFER;

NTSYSAPI
NTSTATUS
NTAPI
NtQueryDirectoryObject(
	IN HANDLE hDirectory,
	OUT PQUERYDIRECTORYOBJECTBUFFER DirectoryEntryBuffer,
	IN ULONG DirectoryEntryBufferSize,
	IN BOOLEAN  bOnlyFirstEntry,
	IN BOOLEAN bFirstEntry,
	IN PULONG  BytesReturned,
	IN PULONG  EntryIndex
);

NTSTATUS
NTAPI
ZwQueryDirectoryObject(
	IN HANDLE hDirectory,
	OUT PQUERYDIRECTORYOBJECTBUFFER DirectoryEntryBuffer,
	IN ULONG DirectoryEntryBufferSize,
	IN BOOLEAN  bOnlyFirstEntry,
	IN BOOLEAN bFirstEntry,
	IN PULONG  BytesReturned,
	IN PULONG  EntryIndex
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenSymbolicLinkObject(
	OUT PHANDLE hSymbolicLink,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject(
	OUT PHANDLE hSymbolicLink,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtQuerySymbolicLinkObject(
	IN HANDLE hSymbolicLink,
	IN OUT PUNICODE_STRING ObjectName,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySymbolicLinkObject(
	IN HANDLE hSymbolicLink,
	IN OUT PUNICODE_STRING ObjectName,
	OUT PULONG BytesReturned
);

typedef struct ObjectBasicInfo_t {
	char Unknown1[8];
	ULONG HandleCount;
	ULONG ReferenceCount;
	ULONG PagedQuota;
	ULONG NonPagedQuota;
	char Unknown2[32];
} OBJECT_BASIC_INFO, *POBJECT_BASIC_INFO;

typedef struct ObjectNameInfo_t {
	UNICODE_STRING ObjectName;
	WCHAR ObjectNameBuffer[1];
} OBJECT_NAME_INFO, *POBJECT_NAME_INFO;

typedef struct ObjectTypeInfo_t {
	UNICODE_STRING ObjectTypeName;
	char Unknown[0x58];
	WCHAR ObjectTypeNameBuffer[1];
} OBJECT_TYPE_INFO, *POBJECT_TYPE_INFO;

typedef struct ObjectAllTypeInfo_t {
	ULONG NumberOfObjectTypes;
	OBJECT_TYPE_INFO ObjectsTypeInfo[1];
} OBJECT_ALL_TYPES_INFO, *POBJECT_ALL_TYPES_INFO;

typedef struct ObjectProtectionInfo_t {
	BOOLEAN bInherit;
	BOOLEAN bProtectHandle;
} OBJECT_PROTECTION_INFO, *POBJECT_PROTECTION_INFO;

typedef enum _OBJECT_INFO_CLASS {
	ObjectBasicInfo,
	ObjectNameInfo,
	ObjectTypeInfo,
	ObjectAllTypesInfo,
	ObjectProtectionInfo
} OBJECT_INFO_CLASS;


NTSYSAPI
NTSTATUS
NTAPI
NtQueryObject(
	IN HANDLE hObject,
	IN OBJECT_INFO_CLASS ObjectInfoClass,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryObject(
	IN HANDLE hObject,
	IN OBJECT_INFO_CLASS ObjectInfoClass,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetInformationObject(
	IN HANDLE hObject,
	IN OBJECT_INFO_CLASS ObjectInfoClass,
	IN PVOID Buffer,
	IN ULONG BufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationObject(
	IN HANDLE hObject,
	IN OBJECT_INFO_CLASS ObjectInfoClass,
	IN PVOID Buffer,
	IN ULONG BufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateEvent(
	OUT PHANDLE hEvent,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN EVENT_TYPE EventType,
	IN BOOLEAN bInitialState
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEvent(
	OUT PHANDLE hEvent,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN EVENT_TYPE EventType,
	IN BOOLEAN bInitialState
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenEvent(
	OUT PHANDLE hEvent,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEvent(
	OUT PHANDLE hEvent,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtClearEvent(
	IN HANDLE hEvent
);

NTSYSAPI
NTSTATUS
NTAPI
ZwClearEvent(
	IN HANDLE hEvent
);

NTSYSAPI
NTSTATUS
NTAPI
NtPulseEvent(
	IN HANDLE hEvent,
	OUT OPTIONAL PULONG PreviousState
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPulseEvent(
	IN HANDLE hEvent,
	OUT OPTIONAL PULONG PreviousState
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetEvent(
	IN HANDLE hEvent,
	OUT OPTIONAL PULONG PreviousState
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetEvent(
	IN HANDLE hEvent,
	OUT OPTIONAL PULONG PreviousState
);

NTSYSAPI
NTSTATUS
NTAPI
NtResetEvent(
	IN HANDLE hEvent,
	OUT OPTIONAL PULONG PreviousState
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResetEvent(
	IN HANDLE hEvent,
	OUT OPTIONAL PULONG PreviousState
);

typedef enum _EVENT_INFO_CLASS {
	EventBasicInfo
} EVENT_INFO_CLASS;

typedef struct EventInfo_t {
	EVENT_TYPE EventType;
	LONG EventState;
} EVENT_INFO, *PEVENT_INFO;

NTSYSAPI
NTSTATUS
NTAPI
NtQueryEvent(
	IN HANDLE hEvent,
	IN EVENT_INFO_CLASS InfoClass,
	OUT PVOID EventInfoBuffer,
	IN ULONG EventInfoBufferSize,
	OUT PULONG BytesCopied
);

NTSTATUS
NTAPI
ZwQueryEvent(
	IN HANDLE hEvent,
	IN EVENT_INFO_CLASS InfoClass,
	OUT PVOID EventInfoBuffer,
	IN ULONG EventInfoBufferSize,
	OUT PULONG BytesCopied
);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateEventPair(
	OUT PHANDLE hEventPair,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEventPair(
	OUT PHANDLE hEventPair,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenEventPair(
	OUT PHANDLE hEventPair,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEventPair(
	OUT PHANDLE hEventPair,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetLowWaitHighEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowWaitHighEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetHighWaitLowEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighWaitLowEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetHighEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetLowEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
NtWaitHighEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitHighEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
NtWaitLowEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitLowEventPair(
	IN HANDLE hEventPair
);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateMutant(
	OUT PHANDLE hMutex,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN bOwnMutant
);

NTSTATUS
NTAPI
ZwCreateMutant(
	OUT PHANDLE hMutex,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN bOwnMutant
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenMutant(
	OUT PHANDLE hMutex,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
ZwOpenMutant(
	OUT PHANDLE hMutex,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

typedef enum _MUTANT_INFO_CLASS {
	MutantBasicInfo
} MUTANT_INFO_CLASS;

typedef struct MutantInfo_t {
	LONG MutantState;
	BOOLEAN bOwnedByCallingThread;
	BOOLEAN bAbandoned;
	USHORT Unused;
} MUTANT_INFO, *PMUTANT_INFO;

NTSYSAPI
NTSTATUS
NTAPI
NtQueryMutant(
	IN HANDLE hMutant,
	IN MUTANT_INFO_CLASS InfoClass,
	OUT PVOID MutantInfoBuffer,
	IN ULONG MutantInfoBufferSize,
	OUT PULONG BytesCopied
);

NTSTATUS
NTAPI
ZwQueryMutant(
	IN HANDLE hMutant,
	IN MUTANT_INFO_CLASS InfoClass,
	OUT PVOID MutantInfoBuffer,
	IN ULONG MutantInfoBufferSize,
	OUT PULONG BytesCopied
);

NTSYSAPI
NTSTATUS
NTAPI
NtReleaseMutant(
	IN HANDLE hMutant,
	OUT OPTIONAL PULONG bWasSignalled
);

NTSTATUS
NTAPI
ZwReleaseMutant(
	IN HANDLE hMutant,
	OUT OPTIONAL PULONG bWasSignalled
);


NTSYSAPI
NTSTATUS
NTAPI
NtCreateSemaphore(
	OUT PHANDLE hSemaphore,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG InitialCount,
	IN ULONG MaximumCount
);


NTSTATUS
NTAPI
ZwCreateSemaphore(
	OUT PHANDLE hSemaphore,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG InitialCount,
	IN ULONG MaximumCount
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenSemaphore(
	OUT PHANDLE hSemaphore,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
ZwOpenSemaphore(
	OUT PHANDLE hSemaphore,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

typedef enum _SEMAPHORE_INFO_CLASS {
	SemaphoreBasicInfo
} SEMAPHORE_INFO_CLASS;

typedef struct SemaphoreInfo_t {
	ULONG CurrentCount;
	ULONG MaxCount;
} SEMAPHORE_INFO, *PSEMAPHORE_INFO;



NTSYSAPI
NTSTATUS
NTAPI
NtQuerySemaphore(
	IN HANDLE hSemaphore,
	IN SEMAPHORE_INFO_CLASS SemaphoreInfoClass,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	OUT PULONG BytesReturned
);

NTSTATUS
NTAPI
ZwQuerySemaphore(
	IN HANDLE hSemaphore,
	IN SEMAPHORE_INFO_CLASS SemaphoreInfoClass,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
NtReleaseSemaphore(
	IN HANDLE hSemaphore,
	IN ULONG ReleaseCount,
	OUT PULONG PreviousCount
);

NTSTATUS
NTAPI
ZwReleaseSemaphore(
	IN HANDLE hSemaphore,
	IN ULONG ReleaseCount,
	OUT PULONG PreviousCount
);


NTSYSAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
	IN HANDLE hObject,
	IN BOOLEAN bAlertable,
	IN PLARGE_INTEGER Timeout
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForSingleObject(
	IN HANDLE hObject,
	IN BOOLEAN bAlertable,
	IN PLARGE_INTEGER Timeout
);

NTSYSAPI
NTSTATUS
NTAPI
NtSignalAndWaitForSingleObject(
	IN HANDLE hSignalObject,
	IN HANDLE hWaitObject,
	IN BOOLEAN bAlertable,
	IN PLARGE_INTEGER Timeout
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSignalAndWaitForSingleObject(
	IN HANDLE hSignalObject,
	IN HANDLE hWaitObject,
	IN BOOLEAN bAlertable,
	IN PLARGE_INTEGER Timeout
);


NTSYSAPI
NTSTATUS
NTAPI
NtWaitForMultipleObjects(
	IN ULONG nWaitObjectHandles,
	IN PHANDLE WaitObjectHandlesArray,
	IN WAIT_TYPE WaitType,
	IN BOOLEAN bAlertable,
	IN PLARGE_INTEGER Timeout
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects(
	IN ULONG nWaitObjectHandles,
	IN PHANDLE WaitObjectHandlesArray,
	IN WAIT_TYPE WaitType,
	IN BOOLEAN bAlertable,
	IN PLARGE_INTEGER Timeout
);


NTSYSAPI
NTSTATUS
NTAPI
NtCreateTimer(
	OUT PHANDLE phTimer,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN TIMER_TYPE TimerType
);

NTSTATUS

ZwCreateTimer(
	OUT PHANDLE phTimer,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN TIMER_TYPE TimerType
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenTimer(
	OUT PHANDLE phTimer,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS

ZwOpenTimer(
	OUT PHANDLE phTimer,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

typedef enum _TIMER_INFO_CLASS {
	TimerBasicInfo
} TIMER_INFO_CLASS;

typedef struct TimerInfo_t {
	LARGE_INTEGER DueTime;
	CCHAR TimerState;
	CCHAR Unused[3];
	ULONG TimerType;
} TIMER_INFO, *PTIMER_INFO;

NTSYSAPI
NTSTATUS
NTAPI
NtQueryTimer(
	IN HANDLE hTimer,
	IN TIMER_INFO_CLASS InfoClass,
	OUT PVOID TimerInfoBuffer,
	IN ULONG TimerInfoBufferSize,
	OUT PULONG BytesCopied
);

NTSTATUS
NTAPI
ZwQueryTimer(
	IN HANDLE hTimer,
	IN TIMER_INFO_CLASS InfoClass,
	OUT PVOID TimerInfoBuffer,
	IN ULONG TimerInfoBufferSize,
	OUT PULONG BytesCopied
);


typedef VOID
(NTAPI *PTIMERAPCROUTINE)(
   PVOID lpArgToCompletionRoutine,
   ULONG dwTimerLowValue,
   ULONG dwTimerHighValue);


NTSYSAPI
NTSTATUS
NTAPI
NtSetTimer(
	IN HANDLE hTimer,
	IN PLARGE_INTEGER pDueTime,
	IN PTIMERAPCROUTINE pfnCompletionRoutine OPTIONAL,
	IN ULONG pfnCompletionRoutineArg,
	IN BOOLEAN bResume,
	IN LONG Period,
	OUT PBOOLEAN bTimerState
);



NTSYSAPI
NTSTATUS
NTAPI
NtCancelTimer(
	IN HANDLE hTimer,
	OUT PBOOLEAN pbState
);



NTSYSAPI
NTSTATUS
NTAPI
NtDelayExecution(
	IN ULONG bAlertable,
	IN PLARGE_INTEGER pDuration
);

NTSTATUS
NTAPI
ZwDelayExecution(
	IN ULONG bAlertable,
	IN PLARGE_INTEGER pDuration
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryTimerResolution(
	OUT PULONG MaxResolution,
	OUT PULONG MinResolution,
	OUT PULONG SystemResolution
);

NTSTATUS
NTAPI
ZwQueryTimerResolution(
	OUT PULONG MaxResolution,
	OUT PULONG MinResolution,
	OUT PULONG SystemResolution
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetTimerResolution(
	IN ULONG NewResolution,
	IN BOOLEAN bSet,
	OUT PULONG pResolutionSet
);

NTSTATUS
NTAPI
ZwSetTimerResolution(
	IN ULONG NewResolution,
	IN BOOLEAN bSet,
	OUT PULONG pResolutionSet
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryPerformanceCounter(
	OUT PLARGE_INTEGER pPerformanceCount,
	OUT PLARGE_INTEGER pFrequency
);

NTSTATUS
NTAPI
ZwQueryPerformanceCounter(
	OUT PLARGE_INTEGER pPerformanceCount,
	OUT PLARGE_INTEGER pFrequency
);

NTSYSAPI
NTSTATUS
NTAPI
NtQuerySystemTime(
	OUT PLARGE_INTEGER pSystemTime
);

NTSTATUS
NTAPI
ZwQuerySystemTime(
	OUT PLARGE_INTEGER pSystemTime
);


NTSYSAPI
NTSTATUS
NTAPI
NtSetSystemTime(
	IN PLARGE_INTEGER pSystemTime,
	OUT PLARGE_INTEGER pOldsystemTime OPTIONAL
);

NTSTATUS
NTAPI
ZwSetSystemTime(
	IN PLARGE_INTEGER pSystemTime,
	OUT PLARGE_INTEGER pOldsystemTime OPTIONAL
);

NTSYSAPI
ULONG
NTAPI
NtGetTickCount(
);

ULONG
NTAPI
ZwGetTickCount(
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenKey(
	OUT PHANDLE phKey,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKey(
	OUT PHANDLE phKey,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateKey(
	OUT PHANDLE phKey,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG TitleIndex,
	IN PUNICODE_STRING Class,
	IN ULONG CreateOptions,
	OUT PULONG pDisposition
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateKey(
	OUT PHANDLE phKey,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG TitleIndex,
	IN PUNICODE_STRING Class,
	IN ULONG CreateOptions,
	OUT PULONG pDisposition
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetValueKey(
	IN HANDLE hKey,
	IN PUNICODE_STRING uValueName,
	IN ULONG TitleIndex,
	IN ULONG ValueType,
	IN PVOID pValueData,
	IN ULONG pValueDataLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetValueKey(
	IN HANDLE hKey,
	IN PUNICODE_STRING uValueName,
	IN ULONG TitleIndex,
	IN ULONG ValueType,
	IN PVOID pValueData,
	IN ULONG pValueDataLength
);

NTSYSAPI
NTSTATUS
NTAPI
NtEnumerateKey(
	IN HANDLE hKey,
	IN ULONG Index,
	IN KEY_INFORMATION_CLASS  KeyInfoClass,
	OUT PVOID KeyInfoBuffer,
	IN ULONG KeyInfoBufferLength,
	OUT PULONG BytesCopied
);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateKey(
	IN HANDLE hKey,
	IN ULONG Index,
	IN KEY_INFORMATION_CLASS  KeyInfoClass,
	OUT PVOID KeyInfoBuffer,
	IN ULONG KeyInfoBufferLength,
	OUT PULONG BytesCopied
);

NTSYSAPI
NTSTATUS
NTAPI
NtEnumerateValueKey(
	IN HANDLE hKey,
	IN ULONG Index,
	IN KEY_VALUE_INFORMATION_CLASS  KeyValueInfoClass,
	OUT PVOID KeyValueInfoBuffer,
	IN ULONG KeyValueInfoBufferLength,
	OUT PULONG BytesCopied
);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateValueKey(
	IN HANDLE hKey,
	IN ULONG Index,
	IN KEY_VALUE_INFORMATION_CLASS  KeyValueInfoClass,
	OUT PVOID KeyValueInfoBuffer,
	IN ULONG KeyValueInfoBufferLength,
	OUT PULONG BytesCopied
);

NTSYSAPI
NTSTATUS
NTAPI
NtDeleteValueKey(
	IN HANDLE hKey,
	IN PUNICODE_STRING pValueName
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
	IN HANDLE hKey,
	IN PUNICODE_STRING pValueName
);

NTSYSAPI
NTSTATUS
NTAPI
NtDeleteKey(
	IN HANDLE hKey
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteKey(
	IN HANDLE hKey
);

NTSYSAPI
NTSTATUS
NTAPI
NtFlushKey(
	IN HANDLE hKey
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushKey(
	IN HANDLE hKey
);

NTSYSAPI
NTSTATUS
NTAPI
NtInitializeRegistry(
	IN ULONG UnknownParam
);

NTSTATUS
NTAPI
ZwInitializeRegistry(
	IN ULONG UnknownParam
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryKey(
	IN HANDLE hKey,
	IN KEY_INFORMATION_CLASS KeyInfoClass,
	OUT PVOID KeyInfoBuffer,
	IN ULONG KeyInfoBufferLength,
	OUT PULONG BytesCopied
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryKey(
	IN HANDLE hKey,
	IN KEY_INFORMATION_CLASS KeyInfoClass,
	OUT PVOID KeyInfoBuffer,
	IN ULONG KeyInfoBufferLength,
	OUT PULONG BytesCopied
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryValueKey(
	IN HANDLE hKey,
	IN PUNICODE_STRING uValueName,
	IN KEY_VALUE_INFORMATION_CLASS KeyValueInfoClass,
	OUT PVOID KeyValueInfoBuffer,
	IN ULONG KeyValueInfoBufferLength,
	OUT PULONG BytesCopied
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryValueKey(
	IN HANDLE hKey,
	IN PUNICODE_STRING uValueName,
	IN KEY_VALUE_INFORMATION_CLASS KeyValueInfoClass,
	OUT PVOID KeyValueInfoBuffer,
	IN ULONG KeyValueInfoBufferLength,
	OUT PULONG BytesCopied
);

NTSYSAPI
NTSTATUS
NTAPI
NtSaveKey(
	IN HANDLE hKey,
	IN HANDLE hFile
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKey(
	IN HANDLE hKey,
	IN HANDLE hFile
);

NTSYSAPI
NTSTATUS
NTAPI
NtLoadKey(
	IN POBJECT_ATTRIBUTES KeyNameAttributes,
	IN POBJECT_ATTRIBUTES HiveFileNameAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey(
	IN POBJECT_ATTRIBUTES KeyNameAttributes,
	IN POBJECT_ATTRIBUTES HiveFileNameAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtLoadKey2(
	IN POBJECT_ATTRIBUTES KeyNameAttributes,
	IN POBJECT_ATTRIBUTES HiveFileNameAttributes,
	IN ULONG ulFlags
);

NTSTATUS
NTAPI
ZwLoadKey2(
	IN POBJECT_ATTRIBUTES KeyNameAttributes,
	IN POBJECT_ATTRIBUTES HiveFileNameAttributes,
	IN ULONG ulFlags
);


NTSYSAPI
NTSTATUS
NTAPI
NtUnloadKey(
	IN POBJECT_ATTRIBUTES KeyNameAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadKey(
	IN POBJECT_ATTRIBUTES KeyNameAttributes
);

#define REG_NOTIFY_CHANGE_NAME          (0x00000001L) // Create or delete (child)
#define REG_NOTIFY_CHANGE_ATTRIBUTES    (0x00000002L)
#define REG_NOTIFY_CHANGE_LAST_SET      (0x00000004L) // time stamp
#define REG_NOTIFY_CHANGE_SECURITY      (0x00000008L)

NTSYSAPI
NTSTATUS
NTAPI
NtNotifyChangeKey(
	IN HANDLE hKey,
	IN HANDLE hEvent,
	IN PIO_APC_ROUTINE ApcRoutine,
	IN PVOID ApcRoutineContext,
	IN PIO_STATUS_BLOCK pIoStatusBlock,
	IN ULONG NotifyFilter,
	IN BOOLEAN bWatchSubtree,
	OUT PVOID RegChangesDataBuffer,
	IN ULONG RegChangesDataBufferLength,
	IN BOOLEAN bAynchronous
);

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeKey(
	IN HANDLE hKey,
	IN HANDLE hEvent,
	IN PIO_APC_ROUTINE ApcRoutine,
	IN PVOID ApcRoutineContext,
	IN PIO_STATUS_BLOCK pIoStatusBlock,
	IN ULONG NotifyFilter,
	IN BOOLEAN bWatchSubtree,
	OUT PVOID RegChangesDataBuffer,
	IN ULONG RegChangesDataBufferLength,
	IN BOOLEAN bAynchronous
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryMultipleValueKey(
	IN HANDLE hKey,
	IN OUT PKEY_VALUE_ENTRY ValueNameArray,
	IN ULONG nElementsValueNameArray,
	OUT PVOID ValueDataBuffer,
	IN OUT PULONG ValueDataBufferSize,
	OUT PULONG SizeRequired
);

NTSTATUS
NTAPI
ZwQueryMultipleValueKey(
	IN HANDLE hKey,
	IN OUT PKEY_VALUE_ENTRY ValueNameArray,
	IN ULONG nElementsValueNameArray,
	OUT PVOID ValueDataBuffer,
	IN OUT PULONG ValueDataBufferSize,
	OUT PULONG SizeRequired
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetInformationKey(
	IN HANDLE hKey,
	IN KEY_SET_INFORMATION_CLASS KeySetInfoClass,
	IN PKEY_WRITE_TIME_INFORMATION pInfoBuffer,
	IN ULONG pInfoBufferLength
);

//NTSTATUS
//NTAPI
//ZwSetInformationKey(
//	IN HANDLE hKey,
//	IN KEY_SET_INFORMATION_CLASS KeySetInfoClass,
//	IN PKEY_WRITE_TIME_INFORMATION pInfoBuffer,
//	IN ULONG pInfoBufferLength
//);

NTSYSAPI
NTSTATUS
NTAPI
NtRestoreKey(
	IN HANDLE hKey,
	IN HANDLE hFile,
	IN ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRestoreKey(
	IN HANDLE hKey,
	IN HANDLE hFile,
	IN ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
NtReplaceKey(
	IN POBJECT_ATTRIBUTES NewHiveFile,
	IN HANDLE hKey,
	IN POBJECT_ATTRIBUTES BackupHiveFile
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplaceKey(
	IN POBJECT_ATTRIBUTES NewHiveFile,
	IN HANDLE hKey,
	IN POBJECT_ATTRIBUTES BackupHiveFile
);


typedef struct StackInfo_t {
	ULONG Unknown1;
	ULONG Unknown2;
	ULONG TopOfStack;
	ULONG OnePageBelowTopOfStack;
	ULONG BottomOfStack;
} STACKINFO, *PSTACKINFO;

NTSYSAPI
NTSTATUS
NTAPI
NtCreateThread(
	OUT PHANDLE phThread,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE hProcess,
	OUT PCLIENT_ID pClientId,
	IN PCONTEXT pContext,
	OUT PSTACKINFO pStackInfo,
	IN BOOLEAN bSuspended
);

NTSTATUS
NTAPI
ZwCreateThread(
	OUT PHANDLE phThread,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE hProcess,
	OUT PCLIENT_ID pClientId,
	IN PCONTEXT pContext,
	OUT PSTACKINFO pStackInfo,
	IN BOOLEAN bSuspended
);

NTSYSAPI
NTSTATUS
NTAPI
NtTerminateThread(
	IN HANDLE hThread,
	IN ULONG ExitCode
);

NTSTATUS
NTAPI
ZwTerminateThread(
	IN HANDLE hThread,
	IN ULONG ExitCode
);

NTSYSAPI
NTSTATUS
NTAPI
NtGetContextThread(
	IN HANDLE hThread,
	IN OUT PCONTEXT pContext
);

NTSTATUS
NTAPI
ZwGetContextThread(
	IN HANDLE hThread,
	IN OUT PCONTEXT pContext
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetContextThread(
	IN HANDLE hThread,
	IN PCONTEXT pContext
);

NTSTATUS
NTAPI
ZwSetContextThread(
	IN HANDLE hThread,
	IN PCONTEXT pContext
);

NTSYSAPI
NTSTATUS
NTAPI
NtSuspendThread(
	IN HANDLE hThread,
	OUT PULONG pSuspendCount
);

NTSTATUS
NTAPI
ZwSuspendThread(
	IN HANDLE hThread,
	OUT PULONG pSuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
NtResumeThread(
	IN HANDLE hThread,
	OUT PULONG pSuspendCount
);

NTSTATUS
NTAPI
ZwResumeThread(
	IN HANDLE hThread,
	OUT PULONG pSuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
NtTestAlert(
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTestAlert(
);

NTSYSAPI
NTSTATUS
NTAPI
NtAlertThread(
	HANDLE hThread
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertThread(
	HANDLE hThread
);

NTSYSAPI
NTSTATUS
NTAPI
NtAlertResumeThread(
	HANDLE hThread,
	OUT PULONG pOldSuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertResumeThread(
	HANDLE hThread,
	OUT PULONG pOldSuspendCount
);


NTSYSAPI
NTSTATUS
NTAPI
NtQueueApcThread(
	IN HANDLE hThread,
	IN PKNORMAL_ROUTINE ApcRoutine,
	IN PVOID NormalContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
);

NTSTATUS
NTAPI
ZwQueueApcThread(
	IN HANDLE hThread,
	IN PKNORMAL_ROUTINE ApcRoutine,
	IN PVOID NormalContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
);

NTSYSAPI
NTSTATUS
NTAPI
NtContinue(
	PCONTEXT pNewContext,
	BOOLEAN bTestAlert
);

NTSYSAPI
NTSTATUS
NTAPI
ZwContinue(
	PCONTEXT pNewContext,
	BOOLEAN bTestAlert
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenThread(
	OUT PHANDLE phThread,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID pClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThread(
	OUT PHANDLE phThread,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID pClientId
);

NTSYSAPI
NTSTATUS
NTAPI
NtYieldExecution(
);

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution(
);

NTSYSAPI
NTSTATUS
NTAPI
NtAccessCheck(
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	HANDLE hTokenClient,
	ACCESS_MASK DesiredAccess,
	PGENERIC_MAPPING pGenericMapping,
	PPRIVILEGE_SET pPrivilegeSet,
	PULONG pPrivilegeSetLength,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheck(
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	HANDLE hTokenClient,
	ACCESS_MASK DesiredAccess,
	PGENERIC_MAPPING pGenericMapping,
	PPRIVILEGE_SET pPrivilegeSet,
	PULONG pPrivilegeSetLength,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus
);

#ifdef NT50
typedef struct _OBJECT_TYPE_LIST {
	USHORT Level;
	USHORT Sbz;
	GUID *ObjectType;
} OBJECT_TYPE_LIST, *POBJECT_TYPE_LIST;

NTSYSAPI
NTSTATUS
NTAPI
NtAccessCheckByType(
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	PSID PrincipalSelfSid,
	HANDLE hClientToken,
	ACCESS_MASK DesiredAccess,
	POBJECT_TYPE_LIST ObjectTypeList,
	ULONG ObjectTypeListLength,
	PGENERIC_MAPPING pGenericMapping,
	PPRIVILEGE_SET pPrivilegeSet,
	PULONG pPrivilegeSetLength,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckByType(
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	PSID PrincipalSelfSid,
	HANDLE hClientToken,
	ACCESS_MASK DesiredAccess,
	POBJECT_TYPE_LIST ObjectTypeList,
	ULONG ObjectTypeListLength,
	PGENERIC_MAPPING pGenericMapping,
	PPRIVILEGE_SET pPrivilegeSet,
	PULONG pPrivilegeSetLength,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus
);

typedef enum _AUDIT_EVENT_TYPE {
	AuditEventObjectAccess,
	AuditEventDirectoryServiceAccess
} AUDIT_EVENT_TYPE, *PAUDIT_EVENT_TYPE;

NTSYSAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeAndAuditAlarm(
	PUNICODE_STRING SubSystemName,
	PVOID HandleId,
	PUNICODE_STRING ObjectTypeName,
	PUNICODE_STRING ObjectName,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	PSID PrincipalSelfSid,
	ACCESS_MASK DesiredAccess,
	AUDIT_EVENT_TYPE AuditType,
	ULONG Flags,
	POBJECT_TYPE_LIST ObjectTypeList,
	ULONG ObjectTypeListLength,
	PGENERIC_MAPPING pGenericMapping,
	BOOLEAN bObjectCreation,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus,
	PBOOLEAN bGenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeAndAuditAlarm(
	PUNICODE_STRING SubSystemName,
	PVOID HandleId,
	PUNICODE_STRING ObjectTypeName,
	PUNICODE_STRING ObjectName,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	PSID PrincipalSelfSid,
	ACCESS_MASK DesiredAccess,
	AUDIT_EVENT_TYPE AuditType,
	ULONG Flags,
	POBJECT_TYPE_LIST ObjectTypeList,
	ULONG ObjectTypeListLength,
	PGENERIC_MAPPING pGenericMapping,
	BOOLEAN bObjectCreation,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus,
	PBOOLEAN bGenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeResultList(
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	PSID PrincipalSelfSid,
	HANDLE hClientToken,
	ACCESS_MASK DesiredAccess,
	POBJECT_TYPE_LIST ObjectTypeList,
	ULONG ObjectTypeListLength,
	PGENERIC_MAPPING pGenericMapping,
	PPRIVILEGE_SET pPrivilegeSet,
	PULONG pPrivilegeSetLength,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultList(
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	PSID PrincipalSelfSid,
	HANDLE hClientToken,
	ACCESS_MASK DesiredAccess,
	POBJECT_TYPE_LIST ObjectTypeList,
	ULONG ObjectTypeListLength,
	PGENERIC_MAPPING pGenericMapping,
	PPRIVILEGE_SET pPrivilegeSet,
	PULONG pPrivilegeSetLength,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus
);

NTSYSAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarm(
	PUNICODE_STRING SubSystemName,
	PVOID HandleId,
	PUNICODE_STRING ObjectTypeName,
	PUNICODE_STRING ObjectName,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	PSID PrincipalSelfSid,
	ACCESS_MASK DesiredAccess,
	AUDIT_EVENT_TYPE AuditType,
	ULONG Flags,
	POBJECT_TYPE_LIST ObjectTypeList,
	ULONG ObjectTypeListLength,
	PGENERIC_MAPPING pGenericMapping,
	BOOLEAN bObjectCreation,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus,
	PBOOLEAN bGenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultListAndAuditAlarm(
	PUNICODE_STRING SubSystemName,
	PVOID HandleId,
	PUNICODE_STRING ObjectTypeName,
	PUNICODE_STRING ObjectName,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	PSID PrincipalSelfSid,
	ACCESS_MASK DesiredAccess,
	AUDIT_EVENT_TYPE AuditType,
	ULONG Flags,
	POBJECT_TYPE_LIST ObjectTypeList,
	ULONG ObjectTypeListLength,
	PGENERIC_MAPPING pGenericMapping,
	BOOLEAN bObjectCreation,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus,
	PBOOLEAN bGenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
NtImpersonateAnonymousToken(
	IN HANDLE hThread
);

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateAnonymousToken(
	IN HANDLE hThread
);

typedef enum {
    LT_DONT_CARE,
    LT_LOWEST_LATENCY
} LATENCY_TIME;

NTSYSAPI
NTSTATUS
NTAPI
NtRequestWakeupLatency(
	IN LATENCY_TIME Latency
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestWakeupLatency(
	IN LATENCY_TIME Latency
);

NTSYSAPI
NTSTATUS
NTAPI
NtAreMappedFilesTheSame(
	IN PVOID VirtualAddress1,
	IN PVOID VirtualAddress2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAreMappedFilesTheSame(
	IN PVOID VirtualAddress1,
	IN PVOID VirtualAddress2
);


#endif

NTSYSAPI
NTSTATUS
NTAPI
NtAccessCheckAndAuditAlarm(
	PUNICODE_STRING SubSystemName,
	PVOID HandleId,
	PUNICODE_STRING ObjectTypeName,
	PUNICODE_STRING ObjectName,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	ACCESS_MASK DesiredAccess,
	PGENERIC_MAPPING pGenericMapping,
	BOOLEAN bObjectCreation,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus,
	PBOOLEAN bGenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckAndAuditAlarm(
	PUNICODE_STRING SubSystemName,
	PVOID HandleId,
	PUNICODE_STRING ObjectTypeName,
	PUNICODE_STRING ObjectName,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	ACCESS_MASK DesiredAccess,
	PGENERIC_MAPPING pGenericMapping,
	BOOLEAN bObjectCreation,
	PACCESS_MASK pAccessGranted,
	PNTSTATUS AccessGrantedReturnStatus,
	PBOOLEAN bGenerateOnClose
);

typedef struct _SID_AND_ATTRIBUTES {
	PSID Sid;
	ULONG Attributes;
}SID_AND_ATTRIBUTES, * PSID_AND_ATTRIBUTES;


typedef struct _TOKEN_GROUPS {
	ULONG GroupCount;
	SID_AND_ATTRIBUTES Groups[ANYSIZE_ARRAY];
}TOKEN_GROUPS, *PTOKEN_GROUPS;

NTSYSAPI
NTSTATUS
NTAPI
NtAdjustGroupsToken(
	IN HANDLE hToken,
	IN BOOLEAN ResetToDefault,
	IN PTOKEN_GROUPS pNewTokenGroups,
	OUT ULONG pOldTokenGroupsLength,
	OUT PTOKEN_GROUPS pOldTokenGroups,
	OUT PULONG pOldTokenGroupsActualLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustGroupsToken(
	IN HANDLE hToken,
	IN BOOLEAN ResetToDefault,
	IN PTOKEN_GROUPS pNewTokenGroups,
	OUT ULONG pOldTokenGroupsLength,
	OUT PTOKEN_GROUPS pOldTokenGroups,
	OUT PULONG pOldTokenGroupsActualLength OPTIONAL
);

typedef struct _TOKEN_PRIVILEGES {
    ULONG PrivilegeCount;
    LUID_AND_ATTRIBUTES Privileges[ANYSIZE_ARRAY];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES;

#define TOKEN_ASSIGN_PRIMARY    (0x0001)
#define TOKEN_DUPLICATE         (0x0002)
#define TOKEN_IMPERSONATE       (0x0004)
#define TOKEN_QUERY             (0x0008)
#define TOKEN_QUERY_SOURCE      (0x0010)
#define TOKEN_ADJUST_PRIVILEGES (0x0020)
#define TOKEN_ADJUST_GROUPS     (0x0040)
#define TOKEN_ADJUST_DEFAULT    (0x0080)

#define TOKEN_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED  |\
                          TOKEN_ASSIGN_PRIMARY      |\
                          TOKEN_DUPLICATE           |\
                          TOKEN_IMPERSONATE         |\
                          TOKEN_QUERY               |\
                          TOKEN_QUERY_SOURCE        |\
                          TOKEN_ADJUST_PRIVILEGES   |\
                          TOKEN_ADJUST_GROUPS       |\
                          TOKEN_ADJUST_DEFAULT)


#define TOKEN_READ       (STANDARD_RIGHTS_READ      |\
                          TOKEN_QUERY)


#define TOKEN_WRITE      (STANDARD_RIGHTS_WRITE     |\
                          TOKEN_ADJUST_PRIVILEGES   |\
                          TOKEN_ADJUST_GROUPS       |\
                          TOKEN_ADJUST_DEFAULT)

#define TOKEN_EXECUTE    (STANDARD_RIGHTS_EXECUTE)

typedef enum _TOKEN_TYPE {
    TokenPrimary = 1,
    TokenImpersonation
    } TOKEN_TYPE;
typedef TOKEN_TYPE *PTOKEN_TYPE;



	
NTSYSAPI
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
	IN HANDLE hToken,
    IN BOOLEAN DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES pNewPrivlegeSet,
    IN ULONG PreviousPrivilegeSetBufferLength OPTIONAL,
    PTOKEN_PRIVILEGES pPreviousPrivlegeSet OPTIONAL,
    PULONG PreviousPrivlegeSetReturnLength OPTIONAL
);

NTSTATUS
NTAPI
ZwAdjustPrivilegesToken(
	IN HANDLE hToken,
    IN BOOLEAN DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES pNewPrivlegeSet,
    IN ULONG PreviousPrivilegeSetBufferLength OPTIONAL,
    PTOKEN_PRIVILEGES pPreviousPrivlegeSet OPTIONAL,
    PULONG PreviousPrivlegeSetReturnLength OPTIONAL
);


NTSYSAPI
NTSTATUS
NTAPI
NtCloseObjectAuditAlarm(
	IN PUNICODE_STRING SubSystemName,
	IN PVOID HandleId,
	IN BOOLEAN bGenerateOnClose
);


NTSYSAPI
NTSTATUS
NTAPI
ZwCloseObjectAuditAlarm(
	IN PUNICODE_STRING SubSystemName,
	IN PVOID HandleId,
	IN BOOLEAN bGenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
NtDeleteObjectAuditAlarm(
	IN PUNICODE_STRING SubSystemName,
	IN PVOID HandleId,
	IN BOOLEAN bGenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteObjectAuditAlarm(
	IN PUNICODE_STRING SubSystemName,
	IN PVOID HandleId,
	IN BOOLEAN bGenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
NtDuplicateToken(
	IN HANDLE hToken,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes, //Describing quality of service structure and security descriptor and OBJ_INHERIT flag
	IN BOOLEAN bMakeTokenEffectiveOnly,
	IN TOKEN_TYPE TokenType,
	OUT PHANDLE phNewToken
);


NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateToken(
	IN HANDLE hToken,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes, //Describing quality of service structure and security descriptor and OBJ_INHERIT flag
	IN BOOLEAN bMakeTokenEffectiveOnly,
	IN TOKEN_TYPE TokenType,
	OUT PHANDLE phNewToken
);

NTSYSAPI
NTSTATUS
NTAPI
NtImpersonateThread(
	IN HANDLE hThread,
	IN HANDLE hThreadToImpersonate,
	IN PSECURITY_QUALITY_OF_SERVICE Qos
);

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateThread(
	IN HANDLE hThread,
	IN HANDLE hThreadToImpersonate,
	IN PSECURITY_QUALITY_OF_SERVICE Qos
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	IN HANDLE hTokenClient,
	IN ACCESS_MASK DesiredAccess,
	IN ACCESS_MASK GrantedAccess,
	IN PPRIVILEGE_SET pPrivilegeSet,
	IN BOOLEAN bObjectCreation,
	IN BOOLEAN bAccessGranted,
	OUT PBOOLEAN bGenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	IN HANDLE hTokenClient,
	IN ACCESS_MASK DesiredAccess,
	IN ACCESS_MASK GrantedAccess,
	IN PPRIVILEGE_SET pPrivilegeSet,
	IN BOOLEAN bObjectCreation,
	IN BOOLEAN bAccessGranted,
	OUT PBOOLEAN bGenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
	IN HANDLE hProcess,
	IN ACCESS_MASK DesiredAccess,
	OUT PHANDLE phToken
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken(
	IN HANDLE hProcess,
	IN ACCESS_MASK DesiredAccess,
	OUT PHANDLE phToken
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenThreadToken(
	IN HANDLE hThread,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN bUseContextOfProcess,
	OUT PHANDLE phToken
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken(
	IN HANDLE hThread,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN bUseContextOfProcess,
	OUT PHANDLE phToken
);

NTSYSAPI
NTSTATUS
NTAPI
NtPrivilegeCheck(
	IN HANDLE hToken,
	PPRIVILEGE_SET pPrivilegeSet,
	PBOOLEAN pbHasPrivileges
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegeCheck(
	IN HANDLE hToken,
	IN PPRIVILEGE_SET pPrivilegeSet,
	OUT PBOOLEAN pbHasPrivileges
);

NTSYSAPI
NTSTATUS
NTAPI
NtPrivilegeObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN HANDLE hToken,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET pPrivilegeSet,
    IN BOOLEAN AccessGranted
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegeObjectAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN HANDLE hToken,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET pPrivilegeSet,
    IN BOOLEAN AccessGranted
);

NTSYSAPI
NTSTATUS
NTAPI
NtPrivilegedServiceAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
    IN PUNICODE_STRING ServiceName,
    IN HANDLE hToken,
    IN PPRIVILEGE_SET pPrivilegeSet,
    IN BOOLEAN AccessGranted
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegedServiceAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
    IN PUNICODE_STRING ServiceName,
    IN HANDLE hToken,
    IN PPRIVILEGE_SET pPrivilegeSet,
    IN BOOLEAN AccessGranted
);

typedef enum _TOKEN_INFORMATION_CLASS {
    TokenUser = 1,
    TokenGroups,
    TokenPrivileges,
    TokenOwner,
    TokenPrimaryGroup,
    TokenDefaultDacl,
    TokenSource,
    TokenType,
    TokenImpersonationLevel,
    TokenStatistics
} TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;


NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationToken(
	IN HANDLE hToken,
	IN TOKEN_INFORMATION_CLASS TokenInfoClass,
	OUT PVOID TokenInfoBuffer,
	IN ULONG TokenInfoBufferLength,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationToken(
	IN HANDLE hToken,
	IN TOKEN_INFORMATION_CLASS TokenInfoClass,
	OUT PVOID TokenInfoBuffer,
	IN ULONG TokenInfoBufferLength,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetInformationToken(
	IN HANDLE hToken,
	IN TOKEN_INFORMATION_CLASS TokenInfoClass,
	IN PVOID TokenInfoBuffer,
	IN ULONG TokenInfoBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationToken(
	IN HANDLE hToken,
	IN TOKEN_INFORMATION_CLASS TokenInfoClass,
	IN PVOID TokenInfoBuffer,
	IN ULONG TokenInfoBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
NtQuerySecurityObject(
	IN HANDLE hObject,
	IN SECURITY_INFORMATION SecurityInfoRequested,
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	IN ULONG pSecurityDescriptorLength,
	OUT PULONG BytesRequired
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySecurityObject(
	IN HANDLE hObject,
	IN SECURITY_INFORMATION SecurityInfoRequested,
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	IN ULONG pSecurityDescriptorLength,
	OUT PULONG BytesRequired
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetSecurityObject(
	IN HANDLE hObject,
	IN SECURITY_INFORMATION SecurityInfoRequested,
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSecurityObject(
	IN HANDLE hObject,
	IN SECURITY_INFORMATION SecurityInfoRequested,
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor
);


#define SEC_FILE           0x800000     
#define SEC_IMAGE         0x1000000     
#define SEC_RESERVE       0x4000000     
#define SEC_COMMIT        0x8000000     
#define SEC_NOCACHE      0x10000000     


NTSYSAPI
NTSTATUS
NTAPI
NtCreateSection (
	OUT PHANDLE phSection,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PLARGE_INTEGER MaximumSize OPTIONAL,
	IN ULONG SectionPageProtection,
	IN ULONG AllocationAttributes,
	IN HANDLE hFile OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSection (
	OUT PHANDLE phSection,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PLARGE_INTEGER MaximumSize OPTIONAL,
	IN ULONG SectionPageProtection,
	IN ULONG AllocationAttributes,
	IN HANDLE hFile OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
NtExtendSection(
	IN HANDLE hSection,
	IN OUT PLARGE_INTEGER ExtendSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwExtendSection(
	IN HANDLE hSection,
	IN OUT PLARGE_INTEGER ExtendSize
);


typedef enum _SECTION_INFORMATION_CLASS {
	SectionBasicInfo,
	SectionDetailedInfo,
} SECTION_INFORMATION_CLASS, *PSECTION_INFORMATION_CLASS;

typedef struct SectionBasicInfo_t {
	ULONG Unknown;
	ULONG AllocationAttributes;
	LARGE_INTEGER MaximumSize;
} SECTION_BASIC_INFO, *PSECTION_BASIC_INFO;

//SectionDetailedInfo works only on image mapped sections
typedef struct SectionDetailedInfo_t {
	char UnknownData[0x30];
} SECTION_DETAILED_INFO, *PSECTION_DETAILED_INFO;


NTSYSAPI
NTSTATUS
NTAPI
NtQuerySection(
	IN HANDLE hSection,
	IN SECTION_INFORMATION_CLASS SectionInfoClass,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenSection(
	OUT PHANDLE phSection,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);


NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSection(
	OUT PHANDLE phSection,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);


NTSYSAPI
NTSTATUS
NTAPI
NtMapViewOfSection(
	IN HANDLE hSection,
	IN HANDLE hProcess,
	IN OUT PVOID *BaseAddress,
	IN ULONG ZeroBits,
	IN ULONG CommitSize,
	IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PULONG ViewSize,
	IN SECTION_INHERIT InheritDisposition,
	IN ULONG AllocationType,
	IN ULONG Protect
);

NTSYSAPI
NTSTATUS
NTAPI
ZwMapViewOfSection(
	IN HANDLE hSection,
	IN HANDLE hProcess,
	IN OUT PVOID *BaseAddress,
	IN ULONG ZeroBits,
	IN ULONG CommitSize,
	IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PULONG ViewSize,
	IN SECTION_INHERIT InheritDisposition,
	IN ULONG AllocationType,
	IN ULONG Protect
);

NTSYSAPI
NTSTATUS
NTAPI
NtUnmapViewOfSection(
	IN HANDLE hProcess,
	IN PVOID BaseAddress
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnmapViewOfSection(
	IN HANDLE hProcess,
	IN PVOID BaseAddress
);

NTSYSAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
	IN HANDLE hProces,
	IN OUT PVOID *PreferredBaseAddress,
	IN ULONG nLowerZeroBits,
	IN OUT PULONG SizeRequestedAllocated,
	IN ULONG AllocationType,
	IN ULONG ProtectionAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
	IN HANDLE hProces,
	IN OUT PVOID *PreferredBaseAddress,
	IN ULONG nLowerZeroBits,
	IN OUT PULONG SizeRequestedAllocated,
	IN ULONG AllocationType,
	IN ULONG ProtectionAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
	IN HANDLE hProcess,
	IN OUT PVOID StartingAddress,
	IN OUT PULONG SizeRequestedReleased,
	IN ULONG ReleaseType
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFreeVirtualMemory(
	IN HANDLE hProcess,
	IN OUT PVOID StartingAddress,
	IN OUT PULONG SizeRequestedReleased,
	IN ULONG ReleaseType
);

NTSYSAPI
NTSTATUS
NTAPI
NtFlushVirtualMemory(
	IN HANDLE hProcess,
	IN OUT PVOID StartingAddress,
	IN OUT PULONG SizeToFlush,
	OUT PIO_STATUS_BLOCK pIoStatusBlock
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushVirtualMemory(
	IN HANDLE hProcess,
	IN OUT PVOID StartingAddress,
	IN OUT PULONG SizeToFlush,
	OUT PIO_STATUS_BLOCK pIoStatusBlock
);

typedef struct _MEMORY_BASIC_INFORMATION {
	PVOID BaseAddress;
	PVOID AllocationBase;
	ULONG AllocationProtect;
	ULONG RegionSize;
	ULONG State;
	ULONG Protect;
	ULONG Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

typedef struct _BACKEDUP_SECTION_FILENAME_INFO {
	UNICODE_STRING BackedupSectionFileName;
	WCHAR Filename[1];
} MEMORY_BACKEDUP_SECTION_FILENAME_INFO, *PMEMORY_BACKEDUP_SECTION_FILENAME_INFO;


typedef enum _MEMORY_INFO_CLASS {
	MemoryBasicInformation,
	WorkingSetInfo,
	BackedupSectionFileNameInfo
} MEMORY_INFO_CLASS;


NTSYSAPI
NTSTATUS
NTAPI
NtQueryVirtualMemory(
	IN HANDLE hProcess,
	IN PVOID BaseAddress,
	IN MEMORY_INFO_CLASS MemoryInfoClass,
	OUT PVOID MemoryBasicInfo,
	IN ULONG MemoryBasicInfoSize,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVirtualMemory(
	IN HANDLE hProcess,
	IN PVOID BaseAddress,
	IN MEMORY_INFO_CLASS MemoryInfoClass,
	OUT PVOID MemoryBasicInfo,
	IN ULONG MemoryBasicInfoSize,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
NtProtectVirtualMemory(
	IN HANDLE hProcess,
	IN OUT PVOID *BaseAddress,
	IN OUT PULONG RegionSize,
	IN ULONG Protect,
	OUT PULONG OldProtect
);

NTSYSAPI
NTSTATUS
NTAPI
ZwProtectVirtualMemory(
	IN HANDLE hProcess,
	IN OUT PVOID *BaseAddress,
	IN OUT PULONG RegionSize,
	IN ULONG Protect,
	OUT PULONG OldProtect
);


NTSYSAPI
NTSTATUS
NTAPI
NtLockVirtualMemory(
	IN HANDLE hProcess,
	IN OUT PVOID *BaseAddress,
	IN OUT PULONG RegionSize,
	IN ULONG Unknown //(valid values are 1,2,3, VirtualAlloc uses 1
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLockVirtualMemory(
	IN HANDLE hProcess,
	IN OUT PVOID *BaseAddress,
	IN OUT PULONG RegionSize,
	IN ULONG Unknown //(valid values are 1,2,3, VirtualLock uses 1
);

NTSYSAPI
NTSTATUS
NTAPI
NtUnlockVirtualMemory(
	IN HANDLE hProcess,
	IN OUT PVOID *BaseAddress,
	IN OUT PULONG RegionSize,
	IN ULONG Unknown //(valid values are 1,2,3, VirtualUnlock uses 1
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnlockVirtualMemory(
	IN HANDLE hProcess,
	IN OUT PVOID *BaseAddress,
	IN OUT PULONG RegionSize,
	IN ULONG Unknown //(valid values are 1,2,3, VirtualUnlock uses 1
);

NTSYSAPI
NTSTATUS
NTAPI
NtReadVirtualMemory(
	IN HANDLE hProcess,
	IN PVOID BaseAddress,
	OUT PVOID Buffer,
	IN ULONG BytesToRead,
	OUT PULONG BytesRead
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadVirtualMemory(
	IN HANDLE hProcess,
	IN PVOID BaseAddress,
	OUT PVOID Buffer,
	IN ULONG BytesToRead,
	OUT PULONG BytesRead
);

NTSYSAPI
NTSTATUS
NTAPI
NtWriteVirtualMemory(
	IN HANDLE hProcess,
	IN PVOID BaseAddress,
	IN PVOID Buffer,
	IN ULONG BytesToWrite,
	OUT PULONG BytesWritten
);


NTSYSAPI
NTSTATUS
NTAPI
ZwWriteVirtualMemory(
	IN HANDLE hProcess,
	IN PVOID BaseAddress,
	IN PVOID Buffer,
	IN ULONG BytesToWrite,
	OUT PULONG BytesWritten
);


NTSYSAPI
NTSTATUS
NTAPI
NtCancelIoFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK IoStatusBlock
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCancelIoFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK IoStatusBlock
);


NTSYSAPI
NTSTATUS
NTAPI
NtCreateFile(
    OUT PHANDLE phFile,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateFile(
    OUT PHANDLE phFile,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
);


NTSYSAPI
NTSTATUS
NTAPI
NtCreateIoCompletion(
	OUT PHANDLE phIoCompletionPort,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG nConcurrentThreads
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateIoCompletion(
	OUT PHANDLE phIoCompletionPort,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG nConcurrentThreads
);


NTSYSAPI
NTSTATUS
NTAPI
NtOpenIoCompletion(
	OUT PHANDLE phIoCompletionPort,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenIoCompletion(
	OUT PHANDLE phIoCompletionPort,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

typedef struct _OVERLAPPED {
	ULONG   Internal;
	ULONG   InternalHigh;
	ULONG   Offset;
	ULONG   OffsetHigh;
	HANDLE  hEvent;
} OVERLAPPED, *LPOVERLAPPED;

NTSYSAPI
NTSTATUS
NTAPI
NtSetIoCompletion(
	IN HANDLE hIoCompletionPort,
	ULONG CompletionKey,
	LPOVERLAPPED pOverlapped,
	NTSTATUS NtStatus,
	ULONG NumberOfBytesTransferred
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetIoCompletion(
	IN HANDLE hIoCompletionPort,
	ULONG CompletionKey,
	LPOVERLAPPED pOverlapped,
	NTSTATUS NtStatus,
	ULONG NumberOfBytesTransferred
);

typedef enum _IOCOMPLETIONPORT_INFO_CLASS {
	IoCompletionPortBasicInfo
} IOCOMPLETIONPORT_INFO_CLASS;

typedef struct IoCompletionPortBasicInformation_t {
	ULONG NumberOfEvents;
} IOCOMPLETIONPORT_BASIC_INFO, *PIOCOMPLETIONPORT_BASIC_INFO;


NTSYSAPI
NTSTATUS
NTAPI
NtQueryIoCompletion(
	IN HANDLE hIoCompletionPort,
	IN IOCOMPLETIONPORT_INFO_CLASS InfoClass,
	OUT PVOID Buffer,
	IN ULONG BufferLen,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryIoCompletion(
	IN HANDLE hIoCompletionPort,
	IN IOCOMPLETIONPORT_INFO_CLASS InfoClass,
	OUT PVOID Buffer,
	IN ULONG BufferLen,
	OUT PULONG BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
NtRemoveIoCompletion(
	IN HANDLE hIoCompletion,
	OUT PULONG lpCompletionKey,
	OUT LPOVERLAPPED *pOverlapped,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PLARGE_INTEGER Timeout
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRemoveIoCompletion(
	IN HANDLE hIoCompletion,
	OUT PULONG lpCompletionKey,
	OUT LPOVERLAPPED *pOverlapped,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PLARGE_INTEGER Timeout
);

NTSYSAPI
NTSTATUS
NTAPI
NtDeleteFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN ULONG DeviceIoControlCode,
	IN PVOID InBuffer OPTIONAL,
	IN ULONG InBufferLength,
	OUT PVOID OutBuffer OPTIONAL,
	IN ULONG OutBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN ULONG DeviceIoControlCode,
	IN PVOID InBuffer OPTIONAL,
	IN ULONG InBufferLength,
	OUT PVOID OutBuffer OPTIONAL,
	IN ULONG OutBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
NtFlushBuffersFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock
);

NTSYSAPI
NTSTATUS
NTAPI
NtfsControlFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN ULONG FileSystemControlCode,
	IN PVOID InBuffer OPTIONAL,
	IN ULONG InBufferLength,
	OUT PVOID OutBuffer OPTIONAL,
	IN ULONG OutBufferLength
);
	
NTSYSAPI
NTSTATUS
NTAPI
ZwfsControlFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN ULONG FileSystemControlCode,
	IN PVOID InBuffer OPTIONAL,
	IN ULONG InBufferLength,
	OUT PVOID OutBuffer OPTIONAL,
	IN ULONG OutBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
NtLockFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PLARGE_INTEGER FileOffset,
	IN PLARGE_INTEGER Length,
	IN PULONG LockOperationKey,
	IN BOOLEAN bFailIfNotPossibleAtThisPoint,
	IN BOOLEAN bExclusiveLock
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLockFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PLARGE_INTEGER FileOffset,
	IN PLARGE_INTEGER Length,
	IN PULONG LockOperationKey,
	IN BOOLEAN bFailIfNotPossibleAtThisPoint,
	IN BOOLEAN bExclusiveLock
);

NTSYSAPI
NTSTATUS
NTAPI
NtUnlockFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PLARGE_INTEGER FileOffset,
	IN PLARGE_INTEGER Length,
	IN PULONG LockOperationKey
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnlockFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PLARGE_INTEGER FileOffset,
	IN PLARGE_INTEGER Length,
	IN PULONG LockOperationKey
);

NTSYSAPI
NTSTATUS
NTAPI
NtNotifyChangeDirectoryFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID ChangeBuffer,
	IN ULONG ChangeBufferLength,
	IN ULONG NotifyFilter,
	IN BOOLEAN bWatchSubtree
);
	
NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeDirectoryFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID ChangeBuffer,
	IN ULONG ChangeBufferLength,
	IN ULONG NotifyFilter,
	IN BOOLEAN bWatchSubtree
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenFile(
	OUT PHANDLE phFile,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN ULONG ShareMode,
	IN ULONG OpenMode
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenFile(
	OUT PHANDLE phFile,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN ULONG ShareMode,
	IN ULONG OpenMode
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryAttributesFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PFILE_BASIC_INFORMATION pFileBasicInfo
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryAttributesFile(
	IN OBJECT_ATTRIBUTES ObjectAttributes,
	OUT PFILE_BASIC_INFORMATION pFileBasicInfo
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryDirectoryFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID FileInformationBuffer,
	IN ULONG FileInformationBufferLength,
	IN FILE_INFORMATION_CLASS FileInfoClass,
	IN BOOLEAN bReturnOnlyOneEntry,
	IN PUNICODE_STRING PathMask OPTIONAL,
	IN BOOLEAN bRestartQuery
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID FileInformationBuffer,
	IN ULONG FileInformationBufferLength,
	IN FILE_INFORMATION_CLASS FileInfoClass,
	IN BOOLEAN bReturnOnlyOneEntry,
	IN PUNICODE_STRING PathMask OPTIONAL,
	IN BOOLEAN bRestartQuery
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryEaFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID QueryEaBuffer,
	IN ULONG QueryEaBufferLength,
	IN BOOLEAN bReturnSingleEa,
	IN PVOID pListEa,
	IN ULONG pListEaLength,
	IN PULONG ListEaIndex,
	IN BOOLEAN bRestartQuery
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryEaFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID QueryEaBuffer,
	IN ULONG QueryEaBufferLength,
	IN BOOLEAN bReturnSingleEa,
	IN PVOID pListEa,
	IN ULONG pListEaLength,
	IN PULONG ListEaIndex,
	IN BOOLEAN bRestartQuery
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetEaFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PVOID EaBuffer,
	IN ULONG EaBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetEaFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PVOID EaBuffer,
	IN ULONG EaBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID FileInformationBuffer,
	IN ULONG FileInformationBufferLength,
	IN FILE_INFORMATION_CLASS FileInfoClass
);
	
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID FileInformationBuffer,
	IN ULONG FileInformationBufferLength,
	IN FILE_INFORMATION_CLASS FileInfoClass
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetInformationFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PVOID FileInformationBuffer,
	IN ULONG FileInformationBufferLength,
	IN FILE_INFORMATION_CLASS FileInfoClass
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PVOID FileInformationBuffer,
	IN ULONG FileInformationBufferLength,
	IN FILE_INFORMATION_CLASS FileInfoClass
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryVolumeInformationFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID VolumeInformationBuffer,
	IN ULONG VolumeInformationBufferLength,
	IN FS_INFORMATION_CLASS FileSystemInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID VolumeInformationBuffer,
	IN ULONG VolumeInformationBufferLength,
	IN FS_INFORMATION_CLASS FileSystemInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetVolumeInformationFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PVOID VolumeInformationBuffer,
	IN ULONG VolumeInformationBufferLength,
	IN FS_INFORMATION_CLASS FileSystemInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetVolumeInformationFile(
	IN HANDLE hFile,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PVOID VolumeInformationBuffer,
	IN ULONG VolumeInformationBufferLength,
	IN FS_INFORMATION_CLASS FileSystemInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
NtReadFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID ReadBuffer,
	IN ULONG ReadBufferLength,
	IN PLARGE_INTEGER FileOffset OPTIONAL,
	IN PULONG LockOperationKey
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PVOID ReadBuffer,
	IN ULONG ReadBufferLength,
	IN PLARGE_INTEGER FileOffset OPTIONAL,
	IN PULONG LockOperationKey OPTIONAL
);

//Windows 2000 only
//typedef void * PVOID64;


//Windows 2000 only
NTSYSAPI
NTSTATUS
NTAPI
NtReadFileScatter(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PFILE_SEGMENT_ELEMENT aSegmentArray,
	IN ULONG nBytesToRead,
	IN PLARGE_INTEGER FileOffset OPTIONAL,
	IN PULONG LockOperationKey
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadFileScatter(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PFILE_SEGMENT_ELEMENT aSegmentArray,
	IN ULONG nBytesToRead,
	IN PLARGE_INTEGER FileOffset OPTIONAL,
	IN PULONG LockOperationKey
);

NTSYSAPI
NTSTATUS
NTAPI
NtWriteFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PVOID WriteBuffer,
	IN ULONG WriteBufferLength,
	IN PLARGE_INTEGER FileOffset OPTIONAL,
	IN PULONG LockOperationKey OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN PVOID WriteBuffer,
	IN ULONG WriteBufferLength,
	IN PLARGE_INTEGER FileOffset OPTIONAL,
	IN PULONG LockOperationKey OPTIONAL
);

//Windows 2000 only
NTSYSAPI
NTSTATUS
NTAPI
NtWriteFileGathter(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PFILE_SEGMENT_ELEMENT aSegmentArray,
	IN ULONG nBytesToWrite,
	IN PLARGE_INTEGER FileOffset OPTIONAL,
	IN PULONG LockOperationKey
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFileGathter(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	OUT PFILE_SEGMENT_ELEMENT aSegmentArray,
	IN ULONG nBytesToWrite,
	IN PLARGE_INTEGER FileOffset OPTIONAL,
	IN PULONG LockOperationKey
);

NTSYSAPI
NTSTATUS
NTAPI
NtAllocateLocallyUniqueId(
	OUT PLUID pLuid
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateLocallyUniqueId(
	OUT PLUID pLuid
);


NTSYSAPI
NTSTATUS
NTAPI
NtDisplayString(
	IN PUNICODE_STRING pString
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(
	IN PUNICODE_STRING pString
);

#define JOB_OBJECT_ASSIGN_PROCESS           (0x0001)
#define JOB_OBJECT_SET_ATTRIBUTES           (0x0002)
#define JOB_OBJECT_QUERY                    (0x0004)
#define JOB_OBJECT_TERMINATE                (0x0008)
#define JOB_OBJECT_SET_SECURITY_ATTRIBUTES  (0x0010)
#define JOB_OBJECT_ALL_ACCESS       (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1F )


NTSYSAPI
NTSTATUS
NTAPI
NtCreateJobObject(
	OUT PHANDLE phJob,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

typedef 
NTSTATUS 
(NTAPI *PFNNTCREATEJOBOBJECT)(	
	OUT PHANDLE phJob,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);


NTSYSAPI
NTSTATUS
NTAPI
ZwCreateJobObject(
	OUT PHANDLE phJob,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenJobObject(
	OUT PHANDLE phJob,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

typedef 
NTSTATUS 
(NTAPI *PFNNTOPENJOBOBJECT)(	
	OUT PHANDLE phJob,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenJobObject(
	OUT PHANDLE phJob,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
NtAssignProcessToJobObject(
	IN HANDLE hJob,
	IN HANDLE hProcess
);

typedef 
NTSTATUS 
(NTAPI *PFNNTASSIGNPROCESSTOJOBOBJECT)(	
	IN HANDLE hJob,
	IN HANDLE hProcess
);


NTSYSAPI
NTSTATUS
NTAPI
ZwAssignProcessToJobObject(
	IN HANDLE hJob,
	IN HANDLE hProcess
);

NTSYSAPI
NTSTATUS
NTAPI
NtTerminateJobObject(
	IN HANDLE hJob,
	IN NTSTATUS ExitCode
);

typedef 
NTSTATUS 
(NTAPI *PFNNTTERMINATEJOBOBJECT)(	
	IN HANDLE hJob,
	IN NTSTATUS ExitCode
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateJobObject(
	IN HANDLE hJob,
	IN NTSTATUS ExitCode
);



typedef enum _JOBOBJECTINFOCLASS {
	JobObjectBasicAccountingInformation = 1,
	JobObjectBasicLimitInformation,
	JobObjectBasicProcessIdList,
	JobObjectBasicUIRestrictions,
	JobObjectSecurityLimitInformation,
	JobObjectEndOfJobTimeInformation,
	JobObjectAssociateCompletionPortInformation,
	JobObjectBasicAndIoAccountingInformation,
	JobObjectExtendedLimitInformation,
	MaxJobObjectInfoClass
} JOBOBJECTINFOCLASS;


typedef struct _JOBOBJECT_BASIC_ACCOUNTING_INFORMATION {
	LARGE_INTEGER TotalUserTime;
	LARGE_INTEGER TotalKernelTime;
	LARGE_INTEGER ThisPeriodTotalUserTime;
	LARGE_INTEGER ThisPeriodTotalKernelTime;
	ULONG TotalPageFaultCount;
	ULONG TotalProcesses;
	ULONG ActiveProcesses;
	ULONG TotalTerminatedProcesses;
} JOBOBJECT_BASIC_ACCOUNTING_INFORMATION, *PJOBOBJECT_BASIC_ACCOUNTING_INFORMATION;

typedef struct _JOBOBJECT_BASIC_LIMIT_INFORMATION {
    LARGE_INTEGER PerProcessUserTimeLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    ULONG LimitFlags;
    ULONG MinimumWorkingSetSize;
    ULONG MaximumWorkingSetSize;
    ULONG ActiveProcessLimit;
    ULONG Affinity;
    ULONG PriorityClass;
} JOBOBJECT_BASIC_LIMIT_INFORMATION, *PJOBOBJECT_BASIC_LIMIT_INFORMATION;

typedef struct _JOBOBJECT_BASIC_PROCESS_ID_LIST {
    ULONG NumberOfAssignedProcesses;
    ULONG NumberOfProcessIdsInList;
    ULONG ProcessIdList[1];
} JOBOBJECT_BASIC_PROCESS_ID_LIST, *PJOBOBJECT_BASIC_PROCESS_ID_LIST;

typedef struct _JOBOBJECT_BASIC_UI_RESTRICTIONS {
    ULONG UIRestrictionsClass;
} JOBOBJECT_BASIC_UI_RESTRICTIONS, *PJOBOBJECT_BASIC_UI_RESTRICTIONS;

typedef struct _JOBOBJECT_SECURITY_LIMIT_INFORMATION {
    ULONG SecurityLimitFlags ;
    HANDLE JobToken ;
    PTOKEN_GROUPS SidsToDisable ;
    PTOKEN_PRIVILEGES PrivilegesToDelete ;
    PTOKEN_GROUPS RestrictedSids ;
} JOBOBJECT_SECURITY_LIMIT_INFORMATION, *PJOBOBJECT_SECURITY_LIMIT_INFORMATION ;

typedef struct _JOBOBJECT_END_OF_JOB_TIME_INFORMATION {
    ULONG EndOfJobTimeAction;
} JOBOBJECT_END_OF_JOB_TIME_INFORMATION, *PJOBOBJECT_END_OF_JOB_TIME_INFORMATION;

typedef struct _JOBOBJECT_ASSOCIATE_COMPLETION_PORT {
    PVOID CompletionKey;
    HANDLE CompletionPort;
} JOBOBJECT_ASSOCIATE_COMPLETION_PORT, *PJOBOBJECT_ASSOCIATE_COMPLETION_PORT;

typedef struct _JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION {
	JOBOBJECT_BASIC_ACCOUNTING_INFORMATION BasicInfo;
	IO_COUNTERS IoInfo;
} JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION, *PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION;

typedef struct _JOBOBJECT_EXTENDED_LIMIT_INFORMATION {
    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
    IO_COUNTERS IoInfo;
	ULONG ProcessMemoryLimit;
    ULONG JobMemoryLimit;
    ULONG PeakProcessMemoryUsed;
    ULONG PeakJobMemoryUsed;
} JOBOBJECT_EXTENDED_LIMIT_INFORMATION, *PJOBOBJECT_EXTENDED_LIMIT_INFORMATION;

#define JOB_OBJECT_TERMINATE_AT_END_OF_JOB  0
#define JOB_OBJECT_POST_AT_END_OF_JOB       1

//
// Completion Port Messages for job objects
//
// These values are returned via the lpNumberOfBytesTransferred parameter
//

#define JOB_OBJECT_MSG_END_OF_JOB_TIME          1
#define JOB_OBJECT_MSG_END_OF_PROCESS_TIME      2
#define JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT     3
#define JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO      4
#define JOB_OBJECT_MSG_NEW_PROCESS              6
#define JOB_OBJECT_MSG_EXIT_PROCESS             7
#define JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS    8
#define JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT     9
#define JOB_OBJECT_MSG_JOB_MEMORY_LIMIT         10


//
// Basic Limits
//
#define JOB_OBJECT_LIMIT_WORKINGSET                 0x00000001
#define JOB_OBJECT_LIMIT_PROCESS_TIME               0x00000002
#define JOB_OBJECT_LIMIT_JOB_TIME                   0x00000004
#define JOB_OBJECT_LIMIT_ACTIVE_PROCESS             0x00000008
#define JOB_OBJECT_LIMIT_AFFINITY                   0x00000010
#define JOB_OBJECT_LIMIT_PRIORITY_CLASS             0x00000020
#define JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME          0x00000040
#define JOB_OBJECT_LIMIT_SCHEDULING_CLASS           0x00000080

//
// Extended Limits
//
#define JOB_OBJECT_LIMIT_PROCESS_MEMORY             0x00000100
#define JOB_OBJECT_LIMIT_JOB_MEMORY                 0x00000200
#define JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION 0x00000400
#define JOB_OBJECT_LIMIT_BREAKAWAY_OK               0x00000800
#define JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK        0x00001000

#define JOB_OBJECT_LIMIT_RESERVED1                  0x00002000
#define JOB_OBJECT_LIMIT_RESERVED2                  0x00004000
#define JOB_OBJECT_LIMIT_RESERVED3                  0x00008000
#define JOB_OBJECT_LIMIT_RESERVED4                  0x00010000
#define JOB_OBJECT_LIMIT_RESERVED5                  0x00020000
#define JOB_OBJECT_LIMIT_RESERVED6                  0x00040000


#define JOB_OBJECT_LIMIT_VALID_FLAGS            0x0007ffff

#define JOB_OBJECT_BASIC_LIMIT_VALID_FLAGS      0x000000ff
#define JOB_OBJECT_EXTENDED_LIMIT_VALID_FLAGS   0x00001fff
#define JOB_OBJECT_RESERVED_LIMIT_VALID_FLAGS   0x0007ffff

//
// UI restrictions for jobs
//

#define JOB_OBJECT_UILIMIT_NONE             0x00000000

#define JOB_OBJECT_UILIMIT_HANDLES          0x00000001
#define JOB_OBJECT_UILIMIT_READCLIPBOARD    0x00000002
#define JOB_OBJECT_UILIMIT_WRITECLIPBOARD   0x00000004
#define JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS 0x00000008
#define JOB_OBJECT_UILIMIT_DISPLAYSETTINGS  0x00000010
#define JOB_OBJECT_UILIMIT_GLOBALATOMS      0x00000020
#define JOB_OBJECT_UILIMIT_DESKTOP          0x00000040
#define JOB_OBJECT_UILIMIT_EXITWINDOWS      0x00000080

#define JOB_OBJECT_UILIMIT_ALL              0x000000FF

#define JOB_OBJECT_UI_VALID_FLAGS           0x000000FF

#define JOB_OBJECT_SECURITY_NO_ADMIN            0x00000001
#define JOB_OBJECT_SECURITY_RESTRICTED_TOKEN    0x00000002
#define JOB_OBJECT_SECURITY_ONLY_TOKEN          0x00000004
#define JOB_OBJECT_SECURITY_FILTER_TOKENS       0x00000008




NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationJobObject(
	IN HANDLE hJob,
	IN JOBOBJECTINFOCLASS JobObjectInfoClass,
	OUT PVOID JobObjectInfoBuffer,
	IN ULONG JobObjectInfoBufferLength,
	OUT PULONG BytesReturned
);

typedef 
NTSTATUS 
(NTAPI *PFNNTQUERYINFORMATIONJOBOBJECT)(	
	IN HANDLE hJob,
	IN JOBOBJECTINFOCLASS JobObjectInfoClass,
	OUT PVOID JobObjectInfoBuffer,
	IN ULONG JobObjectInfoBufferLength,
	OUT PULONG BytesReturned
);


NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationJobObject(
	IN HANDLE hJob,
	IN JOBOBJECTINFOCLASS JobObjectInfoClass,
	OUT PVOID JobObjectInfoBuffer,
	IN ULONG JobObjectInfoBufferLength,
	OUT PULONG BytesReturned
);


NTSYSAPI
NTSTATUS
NTAPI
NtSetInformationJobObject(
	IN HANDLE hJob,
	IN JOBOBJECTINFOCLASS JobObjectInfoClass,
	IN PVOID JobObjectInfoBuffer,
	IN ULONG JobObjectInfoBufferLength
);

typedef 
NTSTATUS 
(NTAPI *PFNNTSETINFORMATIONJOBOBJECT)(	
	IN HANDLE hJob,
	IN JOBOBJECTINFOCLASS JobObjectInfoClass,
	IN PVOID JobObjectInfoBuffer,
	IN ULONG JobObjectInfoBufferLength
);


NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationJobObject(
	IN HANDLE hJob,
	IN JOBOBJECTINFOCLASS JobObjectInfoClass,
	IN PVOID JobObjectInfoBuffer,
	IN ULONG JobObjectInfoBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryDefaultUILanguage(
	OUT PUSHORT DefaultUILanguage
);

typedef 
NTSTATUS 
(NTAPI *PFNNTQUERYDEFAULTUILANGUAGE)(	
	OUT PUSHORT DefaultUILanguage
);


NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultUILanguage(
	OUT PUSHORT DefaultUILanguage
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryInstallUILanguage(
	OUT PUSHORT InstallUILanguage
);

typedef 
NTSTATUS 
(NTAPI *PFNNTQUERYINSTALLUILANGUAGE)(	
	OUT PUSHORT InstallUILanguage
);


NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInstallUILanguage(
	OUT PUSHORT InstallUILanguage
);


NTSYSAPI
NTSTATUS
NTAPI
NtSetDefaultUILanguage(
	IN USHORT DefaultUILanguage
);

typedef 
NTSTATUS 
(NTAPI *PFNNTSETDEFAULTUILANGUAGE)(	
	IN USHORT DefaultUILanguage
);


NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultUILanguage(
	IN USHORT DefaultUILanguage
);

NTSYSAPI
NTSTATUS
NTAPI
NtRaiseHardError(
	NTSTATUS NtStatus,
	ULONG nParameters,
	ULONG ParametersMask,
	PVOID *ParameterList,
	ULONG Unknown1,
	PULONG Unknown2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRaiseHardError(
	NTSTATUS NtStatus,
	ULONG nParameters,
	ULONG ParametersMask,
	PVOID *ParameterList,
	ULONG Unknown1,
	PULONG Unknown2
);

NTSYSAPI
NTSTATUS
NTAPI
NtCreatePagingFile(
	IN PUNICODE_STRING PagingFileName,
	IN PLARGE_INTEGER InitialSize,
	IN PLARGE_INTEGER MaxSize,
	IN ULONG Unused OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreatePagingFile(
	IN PUNICODE_STRING PagingFileName,
	IN PLARGE_INTEGER InitialSize,
	IN PLARGE_INTEGER MaxSize,
	IN ULONG Unused OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateProcess(
	OUT PHANDLE phProcess,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE hParentProcess,
	IN BOOLEAN bInheritParentHandles,
	IN HANDLE hSection OPTIONAL,
	IN HANDLE hDebugPort OPTIONAL,
	IN HANDLE hExceptionPort OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProcess(
	OUT PHANDLE phProcess,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE hParentProcess,
	IN BOOLEAN bInheritParentHandles,
	IN HANDLE hSection OPTIONAL,
	IN HANDLE hDebugPort OPTIONAL,
	IN HANDLE hExceptionPort OPTIONAL
);

/*ExitProcess makes two calls to this system service. first time it
passes 0 as the process handle and exitcode and second time, it passes 
current process handle (0xFFFFFFFF) and exitcode.
TerminateProcess makes only one call passing the process handle and 
exit code as the parameter
*/
NTSYSAPI
NTSTATUS
NTAPI
NtTerminateProcess(
	IN HANDLE hProcess,
	IN ULONG ExitCode
);

//NTSYSAPI
//NTSTATUS
//NTAPI
//ZwTerminateProcess(
//	IN HANDLE hProcess,
//	IN ULONG ExitCode
//);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenProcess(
	OUT PHANDLE phProcess,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID pClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess(
	OUT PHANDLE phProcess,
	IN ACCESS_MASK AccessMask,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID pClientId
);


/*
Following information classes are valid for NtQueryInformationProcess
	ProcessBasicInformation
    ProcessQuotaLimits
    ProcessIoCounters
    ProcessVmCounters
    ProcessTimes
    ProcessDebugPort
    ProcessLdtInformation
    ProcessDefaultHardErrorMode
    ProcessPooledUsageAndLimits
    ProcessWorkingSetWatch
    ProcessPriorityClass
    ProcessWx86Information
    ProcessHandleCount
    ProcessPriorityBoost
	ProcessDeviceMap
    ProcessSessionInformation
    ProcessWow64Information

Following information classes are valid for NtSetInformationProcess
    ProcessQuotaLimits
    ProcessBasePriority
    ProcessRaisePriority
    ProcessDebugPort
    ProcessExceptionPort
    ProcessAccessToken
    ProcessLdtInformation
    ProcessLdtSize
    ProcessDefaultHardErrorMode
    ProcessIoPortHandlers          
    ProcessWorkingSetWatch
    ProcessUserModeIOPL
    ProcessEnableAlignmentFaultFixup
    ProcessPriorityClass
    ProcessAffinityMask
    ProcessPriorityBoost
    ProcessDeviceMap
    ProcessSessionInformation
    ProcessForegroundInformation
    ProcessWow64Information 
*/


//Undocumented structure layouts returned by various process information classes

//ProcessBasePriority
typedef struct BasePriority_t {
	ULONG BasePriority;
} BASEPRIORITYINFO, *PBASEPRIORITYINFO;

//ProcessRaisePriority
typedef struct RaisePriority_t {
	ULONG RaisePriority;
} RAISEPRIORITYINFO, *PRAISEPRIORITYINFO;

//ProcessDebugPort
typedef struct DebugPort_t {
	HANDLE hDebugPort;
} DEBUGPORTINFO, *PDEBUGPORTINFO;

//ProcessExceptionPort
typedef struct ExceptionPort_t {
	HANDLE hExceptionPort;
} EXCEPTIONPORTINFO, *PEXCEPTIONPORTINFO;


//ProcessLdtInformation
typedef struct _LDT_ENTRY {
	USHORT  LimitLow;
	USHORT  BaseLow;
	union {
		struct {
			UCHAR   BaseMid;
			UCHAR   Flags1;
			UCHAR   Flags2;
			UCHAR   BaseHi;
		} Bytes;
		struct {
			ULONG   BaseMid : 8;
			ULONG   Type : 5;
			ULONG   Dpl : 2;
			ULONG   Pres : 1;
			ULONG   LimitHi : 4;
			ULONG   Sys : 1;
			ULONG   Reserved_0 : 1;
			ULONG   Default_Big : 1;
			ULONG   Granularity : 1;
			ULONG   BaseHi : 8;
		} Bits;
	} HighWord;
} LDT_ENTRY, *PLDT_ENTRY;


#define LDT_TABLE_SIZE  ( 8 * 1024 * sizeof(LDT_ENTRY) )

typedef struct _LDT_INFORMATION {
	ULONG Start;
	ULONG Length;
	LDT_ENTRY LdtEntries[1];
} PROCESS_LDT_INFORMATION, *PPROCESS_LDT_INFORMATION;

//ProcessLdtSize
typedef struct _LDT_SIZE {
	ULONG Length;
} PROCESS_LDT_SIZE, *PPROCESS_LDT_SIZE;

#define SEM_FAILCRITICALERRORS      0x0001
#define SEM_NOGPFAULTERRORBOX       0x0002
#define SEM_NOALIGNMENTFAULTEXCEPT  0x0004
#define SEM_NOOPENFILEERRORBOX      0x8000

//ProcessDefaultHardErrorMode
typedef struct HardErrorMode_t {
	ULONG HardErrorMode;
} HARDERRORMODEINFO, *PHARDERRORMODEINFO;

//ProcessUserModeIOPL
typedef struct Iopl_t {
	ULONG Iopl;
} IOPLINFO, *PIOPLINFO;

//ProcessEnableAlignmentFaultFixup
typedef struct AllignmentFault_t {
	BOOLEAN bEnableAllignmentFaultFixup;
} ALLIGNMENTFAULTFIXUPINFO, *PALLIGNMENTFAULTFIXUPINFO;

#define KRNL_NORMAL_PRIORITY_CLASS       0x02
#define KRNL_IDLE_PRIORITY_CLASS         0x01
#define KRNL_HIGH_PRIORITY_CLASS         0x03
#define KRNL_REALTIME_PRIORITY_CLASS     0x04

//ProcessPriorityClass
typedef struct PriorityClass_t {
	UCHAR Unknown;
	UCHAR PriorityClass;
} PRIORITYCLASSINFO, *PPRIORITYCLASSINFO;

//ProcessWx86Information
typedef struct x86_t {
	ULONG x86Info;
} X86INFO, *PX86INFO;

//ProcessHandleCount
typedef struct HandleCount_t {
	ULONG HandleCount;
} HANDLECOUNTINFO, *PHANDLECOUNTINFO;

//ProcessAffinityMask
typedef struct AffinityMask_t {
	ULONG AffinityMask;
} AFFINITYMASKINFO, *PAFFINITYMASKINFO;

//ProcessPriorityBoost
typedef struct PriorityBoost_t {
	ULONG bPriorityBoostEnabled;
} PRIORITYBOOSTINFO, *PPRIORITYBOOSTINFO;

//ProcessDeviceMap

#define DRIVE_UNKNOWN		0
#define DRIVE_NO_ROOT_DIR	1
#define DRIVE_REMOVABLE		2
#define DRIVE_FIXED			3
#define DRIVE_REMOTE		4	
#define DRIVE_CDROM			5
#define DRIVE_RAMDISK		6


//ProcessSessionInformation



NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
	IN HANDLE hProcess,
	IN PROCESSINFOCLASS ProcessInfoClass,
	OUT PVOID ProcessInfoBuffer,
	IN ULONG ProcessInfoBufferLength,
	OUT PULONG BytesReturned OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
	IN HANDLE hProcess,
	IN PROCESSINFOCLASS ProcessInfoClass,
	OUT PVOID ProcessInfoBuffer,
	IN ULONG ProcessInfoBufferLength,
	OUT PULONG BytesReturned OPTIONAL
);


NTSYSAPI
NTSTATUS
NTAPI
NtSetInformationProcess(
	IN HANDLE hProcess,
	IN PROCESSINFOCLASS ProcessInfoClass,
	IN PVOID ProcessInfoBuffer,
	IN ULONG ProcessInfoBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
	IN HANDLE hProcess,
	IN PROCESSINFOCLASS ProcessInfoClass,
	IN PVOID ProcessInfoBuffer,
	IN ULONG ProcessInfoBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationThread(
	IN HANDLE hThread,
	IN THREADINFOCLASS ThreadInfoClass,
	OUT PVOID ThreadInfoBuffer,
	IN ULONG ThreadInfoBufferLength,
	OUT PULONG BytesReturned OPTIONAL
);


NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationThread(
	IN HANDLE hThread,
	IN THREADINFOCLASS ThreadInfoClass,
	OUT PVOID ThreadInfoBuffer,
	IN ULONG ThreadInfoBufferLength,
	OUT PULONG BytesReturned OPTIONAL
);


NTSYSAPI
NTSTATUS
NTAPI
NtSetInformationThread(
	IN HANDLE hThread,
	IN THREADINFOCLASS ThreadInfoClass,
	IN PVOID ThreadInfoBuffer,
	IN ULONG ThreadInfoBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
	IN HANDLE hThread,
	IN THREADINFOCLASS ThreadInfoClass,
	IN PVOID ThreadInfoBuffer,
	IN ULONG ThreadInfoBufferLength
);

/*
Following information classes are valid for NtQueryInformationProcess
	ThreadBasicInformation
	ThreadTimes
	ThreadDescriptorTableEntry
	ThreadQuerySetWin32StartAddress
	ThreadPerformanceCount
	ThreadAmILastThread
	ThreadPriorityBoost
	ThreadIsIoPending


Following information classes are valid for NtSetInformationProcess
	ThreadPriority
	ThreadBasePriority
	ThreadAffinityMask
	ThreadImpersonationToken
	ThreadEnableAlignmentFaultFixup
	ThreadEventPair
	ThreadQuerySetWin32StartAddress
	ThreadZeroTlsCell
	ThreadIdealProcessor
	ThreadPriorityBoost
	ThreadSetTlsArrayAddress
	ThreadHideFromDebugger
*/

//Undocumented structure layouts returned by various process information classes

//ThreadBasicInformation
typedef struct _THREAD_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PVOID TebBaseAddress;
	ULONG UniqueProcessId;
	ULONG UniqueThreadId;
    KAFFINITY AffinityMask;
    KPRIORITY BasePriority;
	ULONG DiffProcessPriority;
} THREAD_BASIC_INFORMATION;

//ThreadPriority
typedef struct _THREAD_PRIORITY {
	ULONG Priority;
} THREAD_PRIORITY, *PTHREAD_PRIORITY;

//ThreadBasePriority
typedef struct _THREAD_BASE_PRIORITY {
	ULONG IncBasePriority;
} THREAD_BASE_PRIORITY, *PTHREAD_BASE_PRIORITY;

//ThreadAffinityMask
typedef struct ThreadAffinityMask_t {
	ULONG ThreadAffinityMask;
} THREADAFFINITYMASKINFO, *PTHREADAFFINITYMASKINFO;

//ThreadDescriptorTableEntry
typedef struct _DESCRIPTOR_TABLE_ENTRY {
    ULONG Selector;
    LDT_ENTRY Descriptor;
} DESCRIPTOR_TABLE_ENTRY, *PDESCRIPTOR_TABLE_ENTRY;

//ThreadEventPair
typedef struct _EVENT_PAIR {
	HANDLE hEventPair;
} EVENTPAIRINFO, *PEVENTPAIRINFO;

//ThreadQuerySetWin32StartAddress
typedef struct _WIN32_START_ADDRESS {
	PVOID Win32StartAddress;
} WIN32_START_ADDRESS, *PWIN32_START_ADDRESS;

//ThreadZeroTlsCell
typedef struct _ZERO_TLSCELL {
	ULONG TlsIndex;
} ZERO_TLSCELL, *PZERO_TLSCELL;

//ThreadPerformanceCount
typedef struct _PERFORMANCE_COUNTER {
	ULONG Count1;
	ULONG Count2;
} PERFORMANCE_COUNTER_INFO, *PPERFORMANCE_COUNTER_INFO;

//ThreadAmILastThread
typedef struct _AMI_LAST_THREAD {
	ULONG bAmILastThread;
} AMI_LAST_THREADINFO, *PAMI_LAST_THREADINFO;

//ThreadIdealProcessor
typedef struct _IDEAL_PROCESSOR {
	ULONG IdealProcessor;
} IDEAL_PROCESSORINFO, *PIDEAL_PROCESSORINFO;

//ThreadSetTlsArrayAddress
typedef struct _TLS_ARRAY {
	ULONG *pTlsArray;
} TLS_ARRAYINFO, PTLS_ARRAYINFO;


//ThreadIsIoPending
typedef struct _IS_IO_PENDING {
	ULONG bIsIOPending;
} IS_IO_PENDINGINFO, PIS_IO_PENDINGINFO;

//ThreadHideFromDebugger
typedef struct _HIDE_FROM_DEBUGGER {
	ULONG bHideFromDebugger;
} HIDE_FROM_DEBUGGERINFO, PHIDE_FROM_DEBUGGERINFO;


//Start System information

/*
Information classes valid from NtQuerySystemInformation
	SystemBasicInfo,					//0
	SystemProcessorInfo,				//1
	SystemPerformanceInfo,				//2
	SystemTimeInfo,						//3
	SystemProcessThreadInfo,			//5
	SystemServiceDescriptorTableInfo,	//6
	SystemIoConfigInfo,					//7
	SystemProcessorTimeInfo,			//8
	SystemNtGlobalFlagInfo,				//9
	SystemModuleInfo,					//11
	SystemResourceLockInfo,				//12
	SystemHandleInfo,					//16
	SystemObjectInformation,			//17
	SystemPageFileInformation,			//18
	SystemInstructionEmulationInfo,		//19
	SystemCacheInfo,					//21
	SystemPoolTagInfo,					//22
	SystemProcessorScheduleInfo,		//23
	SystemDpcInfo,						//24
	SystemTimerInfo,					//28
	SystemCrashDumpSectionInfo,			//32
	SystemProcessorFaultCountInfo,		//33
	SystemCrashDumpStateInfo,			//34
	SystemDebuggerInfo,					//35
	SystemThreadSwitchCounters,			//36
	SystemQuotaInfo,					//37
	SystemTimeZoneInfo,					//44
	SystemLookasideInfo,				//45

Information classes valid from NtSetSystemInformation
	SystemNtGlobalFlagInfo,				//9
	SystemCacheInfo,					//21
	SystemDpcInfo,						//24
	SystemLoadSystemImage,				//26	//Callable only from Kernel mode
	SystemUnloadSystemImage,			//27	//Callable only from Kernel mode
	SystemTimerInfo,					//28
	SystemQuotaInfo,					//37
	SystemLoadDriverInfo,				//38
	SystemPrioritySeparation,			//39
*/
 
typedef enum _SYSTEMINFOCLASS {
	SystemBasicInfo,					//0
	SystemProcessorInfo,				//1
	SystemPerformanceInfo,				//2
	SystemTimeInfo,						//3
	SystemPathInfo,						//4
	SystemProcessThreadInfo,			//5
	SystemServiceDescriptorTableInfo,	//6
	SystemIoConfigInfo,					//7
	SystemProcessorTimeInfo,			//8
	SystemNtGlobalFlagInfo,				//9
	SystemNotImplemented1,				//10
	SystemModuleInfo,					//11
	SystemResourceLockInfo,				//12
	SystemNotImplemented2,				//13
	SystemNotImplemented3,				//14
	SystemNotImplemented4,				//15
	SystemHandleInfo,					//16
	//TODO
	SystemObjectInformation,			//17
	//TODO
	SystemPageFileInformation,			//18
	SystemInstructionEmulationInfo,		//19
	SystemNotUsed1,						//20
	SystemCacheInfo,					//21
	SystemPoolTagInfo,					//22
	SystemProcessorScheduleInfo,		//23
	SystemDpcInfo,						//24
	SystemNotImplemented5,				//25
	SystemLoadSystemImage,				//26	//Callable only from Kernel mode
	SystemUnloadSystemImage,			//27	//Callable only from Kernel mode
	SystemTimerInfo,					//28
	SystemNotImplemented6,				//29
	SystemNotImplemented7,				//30
	SystemNotImplemented8,				//31
	SystemCrashDumpSectionInfo,			//32
	/* The SystemCrashDumpSectionInfo class returns handle to the crash dump
	information section. This will succeed only when the system has thrown
	BSOD */
	SystemProcessorFaultCountInfo,		//33
	SystemCrashDumpStateInfo,			//34
	/* The SystemCrashDumpStateInfo class returns handle to the crash dump
	information section. This will succeed only when the system has thrown
	BSOD */
	SystemDebuggerInfo,					//35
	SystemThreadSwitchCounters,			//36
	SystemQuotaInfo,					//37
	SystemLoadDriverInfo,				//38
	SystemPrioritySeparation,			//39
	SystemNotImplemented9,				//40
	SystemNotImplemented10,				//41
	SystemNotUsed6,						//42
	SystemNotUsed7,						//43
	SystemTimeZoneInfo,					//44
	SystemLookasideInfo,				//45
} SYSTEMINFOCLASS;

//Structure definitions for the information classes set/returned by NtQuerySystemInformation
//and NtSetSystemInformation

//SystemBasicInfo
typedef struct BasicMachineInfo {
	ULONG AlwaysZero;
	ULONG KeMaximumIncrement;
	ULONG MmPageSize;
	ULONG MmNumberOfPhysicalPages;
	ULONG MmLowestPhysicalPage;
	ULONG MmHighestPhysicalPage;
	ULONG MmLowestUserAddress;
	ULONG MmLowestUserAddress1;
	ULONG MmHighestUserAddress;
	ULONG KeActiveProcessors;
	char KeNumberProcessors;
} BASICSYSTEMINFO, *PBASICSYSTEMINFO;

//SystemProcessorInfo
typedef struct ProcessorInfo {
	USHORT KeProcessorArchitecture;
	USHORT KeProcessorLevel;
	USHORT KeProcessorRevision;
	USHORT AlwaysZero;
	ULONG KeFeatureBits;
} PROCESSORSYSTEMINFO, *PPROCESSORSYSTEMINFO;

//SystemPerformanceInfo
typedef struct PerformanceInfo {
	LARGE_INTEGER TotalProcessorTime;
	LARGE_INTEGER IoReadTransferCount;
	LARGE_INTEGER IoWriteTransferCount;
	LARGE_INTEGER IoOtherTransferCount;
	ULONG IoReadOperationCount;
	ULONG IoWriteOperationCount;
	ULONG IoOtherOperationCount;
	ULONG MmAvailablePages;
	ULONG MmTotalCommitedPages;
	ULONG MmTotalCommitLimit;
	ULONG MmPeakLimit;
	ULONG PageFaults;
	ULONG WriteCopies;
	ULONG TransitionFaults;
	ULONG Unknown1;
	ULONG DemandZeroFaults;
	ULONG PagesInput;
	ULONG PagesRead;
	ULONG Unknown2;
	ULONG Unknown3;
	ULONG PagesOutput;
	ULONG PageWrites;
	ULONG Unknown4;
	ULONG Unknown5;
	ULONG PoolPagedBytes;
	ULONG PoolNonPagedBytes;
	ULONG Unknown6;
	ULONG Unknown7;
	ULONG Unknown8;
	ULONG Unknown9;
	ULONG MmTotalSystemFreePtes;
	ULONG MmSystemCodepage;
	ULONG MmTotalSystemDriverPages;
	ULONG MmTotalSystemCodePages;
	ULONG Unknown10;
	ULONG Unknown11;
	ULONG Unknown12;
	ULONG MmSystemCachePage;
	ULONG MmPagedPoolPage;
	ULONG MmSystemDriverPage;
	ULONG CcFastReadNoWait;
	ULONG CcFastReadWait;
	ULONG CcFastReadResourceMiss;
	ULONG CcFastReadNotPossible;
	ULONG CcFastMdlReadNoWait;
	ULONG CcFastMdlReadWait;
	ULONG CcFastMdlReadResourceMiss;
	ULONG CcFastMdlReadNotPossible;
	ULONG CcMapDataNoWait;
	ULONG CcMapDataWait;
	ULONG CcMapDataNoWaitMiss;
	ULONG CcMapDataWaitMiss;
	ULONG CcPinMappedDataCount;
	ULONG CcPinReadNoWait;
	ULONG CcPinReadWait;
	ULONG CcPinReadNoWaitMiss;
	ULONG CcPinReadWaitMiss;
	ULONG CcCopyReadNoWait;
	ULONG CcCopyReadWait;
	ULONG CcCopyReadNoWaitMiss;
	ULONG CcCopyReadWaitMiss;
	ULONG CcMdlReadNoWait;
	ULONG CcMdlReadWait;
	ULONG CcMdlReadNoWaitMiss;
	ULONG CcMdlReadWaitMiss;
	ULONG CcReadaheadIos;
	ULONG CcLazyWriteIos;
	ULONG CcLazyWritePages;
	ULONG CcDataFlushes;
	ULONG CcDataPages;
	ULONG ContextSwitches;
	ULONG Unknown13;
	ULONG Unknown14;
	ULONG SystemCalls;
} PERFORMANCEINFO, *PPERFORMANCEINFOINFO;

//SystemTimeInfo
typedef struct TimeInfo {
	LARGE_INTEGER KeBootTime;
	LARGE_INTEGER KeSystemTime;
	LARGE_INTEGER ExpTimeZoneBias;
	ULONG ExpTimeZoneId;
	ULONG Unused;
} TIMESYSTEMINFO, *PTIMESYSTEMINFO;

//SystemProcessThreadInfo
typedef struct ThreadSysInfo_t {
	LARGE_INTEGER ThreadKernelTime;
	LARGE_INTEGER ThreadUserTime;
	LARGE_INTEGER ThreadCreateTime;
	ULONG TickCount;
	ULONG StartEIP;
	CLIENT_ID ClientId;
	ULONG DynamicPriority;
	ULONG BasePriority;
	ULONG nSwitches;
	ULONG Unknown;
	KWAIT_REASON WaitReason;
}THREADSYSINFO, *PTHREADSYSINFO;

typedef struct ProcessThreadSystemInfo {
	ULONG RelativeOffset;
	ULONG nThreads;
	ULONG Unused1[6];
	LARGE_INTEGER ProcessCreateTime;
	LARGE_INTEGER ProcessUserTime;
	LARGE_INTEGER ProcessKernelTime;
	UNICODE_STRING ProcessName;
	ULONG BasePriority;
	ULONG ProcessId;
	ULONG ParentProcessId;
	ULONG HandleCount;
	ULONG Unused2[2];
	ULONG PeakVirtualSizeBytes;
	ULONG TotalVirtualSizeBytes;
	ULONG nPageFaults;
	ULONG PeakWorkingSetSizeBytes;
	ULONG TotalWorkingSetSizeBytes;
	ULONG PeakPagedPoolUsagePages;
	ULONG TotalPagedPoolUsagePages;
	ULONG PeakNonPagedPoolUsagePages;
	ULONG TotalNonPagedPoolUsagePages;
	ULONG TotalPageFileUsageBytes;
	ULONG PeakPageFileUsageBytes;
	ULONG TotalPrivateBytes;
	THREADSYSINFO ThreadSysInfo[1];
} PROCESSTHREADSYSTEMINFO, *PPROCESSTHREADSYSTEMINFO;

//SystemServiceDescriptorTableInfo
typedef struct ServiceDescriptorTableSystemInfo {
	ULONG BufferLength;
	ULONG NumberOfSystemServiceTables;
	ULONG NumberOfServices[1];
	ULONG ServiceCounters[1];
} SERVICEDESCRIPTORTABLESYSTEMINFO, *PSERVICEDESCRIPTORTABLESYSTEMINFO;

//SystemIoConfigInfo
typedef struct IoConfigSystemInfo {
    ULONG DiskCount;
    ULONG FloppyCount;
    ULONG CdRomCount;
    ULONG TapeCount;
    ULONG SerialCount;
    ULONG ParallelCount;
} IOCONFIGSYSTEMINFO, *PIOCONFIGSYSTEMINFO;

//SystemProcessorTimeInfo
typedef struct ProcessorTimeSystemInfo {
	LARGE_INTEGER TotalProcessorRunTime;
	LARGE_INTEGER TotalProcessorTime;
	LARGE_INTEGER TotalProcessorUserTime;
	LARGE_INTEGER TotalDPCTime;
	LARGE_INTEGER TotalInterruptTime;
	ULONG TotalInterrupts;
	ULONG Unused;
} PROCESSORTIMESYSTEMINFO, *PPROCESSORTIMESYSTEMINFO;

//SystemNtGlobalFlagInfo
typedef struct GlobalFlagInfo {
	ULONG NtGlobalFlag;
} NTGLOBALFLAGSYSTEMINFO, *PNTGLOBALFLAGSYSTEMINFO;

//SystemModuleInfo
typedef struct ModuleInfo_t {
	ULONG Unused;
	ULONG Always0;
	ULONG ModuleBaseAddress;
	ULONG ModuleSize;
	ULONG Unknown;
	ULONG ModuleEntryIndex;
	USHORT ModuleNameLength; /* Length of module name not including the path, this field contains valid value only for NTOSKRNL module*/
	USHORT ModulePathLength; /*Length of 'directory path' part of modulename*/
	char ModuleName[256];
} DRIVERMODULEINFO, *PDRIVERMODULEINFO;

typedef struct DriverModuleSystemInfo {
	ULONG nDriverModules;
	DRIVERMODULEINFO DriverModuleInfo[1];
} DRIVERMODULESYSTEMINFO, *PDRIVERMODULESYSTEMINFO;

//SystemResourceLockInfo
typedef struct ResourceLockInfo_t {
	ULONG ResourceAddress;
	ULONG Always1;
	ULONG Unknown;
	ULONG ActiveCount;
	ULONG ContentionCount;
	ULONG Unused[2];
	ULONG NumberOfSharedWaiters;
	ULONG NumberOfExclusiveWaiters;
} RESOURCELOCKINFO, *PRESOURCELOCKINFO;

typedef struct SystemResourceLockInfo {
	ULONG nSystemResourceLocks;
	RESOURCELOCKINFO ResourceLockInfo[1];
} SYSTEMRESOURCELOCKINFO, *PSYSTEMRESOURCELOCKINFO;

//SystemHandleInfo
typedef struct HandleInfo{
	ULONG Pid;
	USHORT  ObjectType;
	USHORT  HandleValue;
	PVOID ObjectPointer;
	ULONG AccessMask;
} HANDLEINFO, *PHANDLEINFO;

typedef struct SystemHandleInfo {
	ULONG nHandleEntries;
	HANDLEINFO HandleInfo[1];
} SYSTEMHANDLEINFO, *PSYSTEMHANDLEINFO;

//SystemPageFileInformation
typedef struct SystemPageFileInfo {
	ULONG RelativeOffset;
	ULONG CurrentSizePages;
	ULONG TotalUsedPages;
	ULONG PeakUsedPages;
	UNICODE_STRING uPagefileFileName;
} SYSTEMPAGEFILEINFO, *PSYSTEMPAGEFILEINFO;

//SystemInstructionEmulationInfo
typedef struct SystemInstructionEmulationCountInfo {
	ULONG VdmSegmentNotPresentCount;
	ULONG VdmINSWCount;
	ULONG VdmESPREFIXCount;
	ULONG VdmCSPREFIXCount;
	ULONG VdmSSPREFIXCount;
	ULONG VdmDSPREFIXCount;
	ULONG VdmFSPREFIXCount;
	ULONG VdmGSPREFIXCount;
	ULONG VdmOPER32PREFIXCount;
	ULONG VdmADDR32PREFIXCount;
	ULONG VdmINSBCount;
	ULONG VdmINSWV86Count;
	ULONG VdmOUTSBCount;
	ULONG VdmOUTSWCount;
	ULONG VdmPUSHFCount;
	ULONG VdmPOPFCount;
	ULONG VdmINTNNCount;
	ULONG VdmINTOCount;
	ULONG VdmIRETCount;
	ULONG VdmINBIMMCount;
	ULONG VdmINWIMMCount;
	ULONG VdmOUTBIMMCount;
	ULONG VdmOUTWIMMCount;
	ULONG VdmINBCount;
	ULONG VdmINWCount;
	ULONG VdmOUTBCount;
	ULONG VdmOUTWCount;
	ULONG VdmLOCKPREFIXCount;
	ULONG VdmREPNEPREFIXCount;
	ULONG VdmREPPREFIXCount;
	ULONG VdmHLTCount;
	ULONG VdmCLICount;
	ULONG VdmSTICount;
	ULONG VdmBopCount;
} SYSTEMINSTRUCTIONEMULATIONCOUNTINFO, *PSYSTEMINSTRUCTIONEMULATIONCOUNTINFO;

//SystemCacheInfo
typedef struct SystemCacheInfo {
	ULONG TotalSize;
	ULONG PeakSize;
	ULONG TotalPageFaultCount;
	ULONG MinWorkingSet;
	ULONG MaxWorkingSet;
	ULONG Unused[4];
} SYSTEMCACHEINFO, *PSYSTEMCACHEINFO;

//SystemPoolTagInfo
typedef struct PoolTagInfo {
	ULONG PoolTag;
	ULONG PagedAllocs;
	ULONG PagedFrees;
	ULONG PagedBytes;
	ULONG NonPagedAllocs;
	ULONG NonPagedFrees;
	ULONG NonPagedBytes;
} POOLTAGINFO, *PPOOLTAGINFO;

typedef struct PoolTagSystemInfo {
	ULONG nTags;
	POOLTAGINFO PoolTagInfo[1];
} POOLTAGSYSTEMINFO, *PPOOLTAGSYSTEMINFO;

//SystemProcessorScheduleInfo
typedef struct ProcessorScheduleInfo_t {
	ULONG nContextSwitches;
	ULONG nDPCQueued;
	ULONG nDPCRate;
	ULONG TimerResolution;
	ULONG nDPCBypasses;
	ULONG nAPCBypasses;
} PROCESSORSCHEDULEINFO, *PPROCESSORSCHEDULEINFO;

//SystemDpcInfo
typedef struct DpcSystemInfo {
	ULONG Unused;
	ULONG KiMaximumDpcQueueDepth;
	ULONG KiMinimumDpcRate;
	ULONG KiAdjustDpcThreshold;
	ULONG KiIdealDpcRate;
} DPCSYSTEMINFO, *PDPCSYSTEMINFO;

//SystemLoadSystemImage
typedef struct LoadSystemImageInfo {
	UNICODE_STRING DriverRegistryEntry; //input
	PVOID ModuleBaseAddress; //output
	PVOID pModuleEntryStructure; //output
	PVOID ModuleEntryPoint; //output
	PVOID Unknown; //output
} LOADSYSTEMIMAGEINFO, *PLOADSYSTEMIMAGEINFO;

//SystemUnloadSystemImage
typedef struct UnloadSystemImageInfo {
	PVOID pModuleEntryStructure;
} UNLOADSYSTEMIMAGEINFO, *PUNLOADSYSTEMIMAGEINFO;

//SystemTimerInfo
typedef struct TimerSystemInfo {
	ULONG KeTimeAdjustment;
	ULONG KeMaximumIncrement;
	BOOLEAN KeTimeSynchronization;
} TIMERSYSTEMINFO, *PTIMERSYSTEMINFO;

//SystemProcessorFaultCountInfo
typedef struct ProcessorSystemFaultCountInfo {
	ULONG nAlignmentFixup;
	ULONG nExceptionDispatches;
	ULONG nFloatingEmulation;
	ULONG Unknown;
} PROCESSORSYSTEMFAULTCOUNTINFO, *PPROCESSORSYSTEMFAULTCOUNTINFO;

//SystemDebuggerInfo
typedef struct DebuggerSystemInfo {
	BOOLEAN bKdDebuggerEnabled;
	BOOLEAN bKdDebuggerPresent;
} DEBUGGERSYSTEMINFO, *PDEBUGGERSYSTEMINFO;

//SystemQuotaInfo
typedef struct QuotaInfo {
	ULONG CmpGlobalQuota;
	ULONG CmpGlobalQuotaUsed;
	ULONG MmSizeofPagedPoolInBytes;
} QUOTAINFO, *PQUOTAINFO;

//SystemLoadDriverInfo
typedef struct LoadDriverInfo {
	UNICODE_STRING DriverRegistryEntry;
} LOADDRIVERINFO, *PLOADDRIVERINFO;

NTSYSAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
	IN SYSTEMINFOCLASS SystemInfoClass,
	OUT PVOID SystemInfoBuffer,
	IN ULONG SystemInfoBufferSize,
	OUT PULONG BytesReturned OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
	IN SYSTEMINFOCLASS SystemInfoClass,
	OUT PVOID SystemInfoBuffer,
	IN ULONG SystemInfoBufferSize,
	OUT PULONG BytesReturned OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetSystemInformation(
	IN SYSTEMINFOCLASS SystemInfoClass,
	IN PVOID SystemInfoBuffer,
	IN ULONG SystemInfoBufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemInformation(
	IN SYSTEMINFOCLASS SystemInfoClass,
	IN PVOID SystemInfoBuffer,
	IN ULONG SystemInfoBufferSize
);


NTSYSAPI
NTSTATUS
NTAPI
NtCreateProfile(
	OUT PHANDLE hProfile,
	IN HANDLE hProcess,
	IN PVOID CodeBaseAddress,
	IN ULONG CodeSize,
	IN ULONG ProfileBucketSize,
	OUT PVOID ProfileResultBuffer,
	IN ULONG ProfileResultBufferSize,
	IN ULONG Unknown1,
	IN ULONG ValidProcessorsForProfilingMap
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProfile(
	OUT PHANDLE hProfile,
	IN HANDLE hProcess,
	IN PVOID CodeBaseAddress,
	IN ULONG CodeSize,
	IN ULONG ProfileBucketSize,
	OUT PVOID ProfileResultBuffer,
	IN ULONG ProfileResultBufferSize,
	IN ULONG Unknown1,
	IN ULONG ValidProcessorsForProfilingMap
);

NTSYSAPI
NTSTATUS
NTAPI
NtStartProfile(
	IN HANDLE hProfile
);

NTSYSAPI
NTSTATUS
NTAPI
ZwStartProfile(
	IN HANDLE hProfile
);

NTSYSAPI
NTSTATUS
NTAPI
NtStopProfile(
	IN HANDLE hProfile
);

NTSYSAPI
NTSTATUS
NTAPI
ZwStopProfile(
	IN HANDLE hProfile
);


NTSYSAPI
NTSTATUS
NTAPI
NtQueryIntervalProfile(
	IN KPROFILE_SOURCE ProfileSource,
	OUT PULONG Interval
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryIntervalProfile(
	IN KPROFILE_SOURCE ProfileSource,
	OUT PULONG Interval
);


NTSYSAPI
NTSTATUS
NTAPI
NtSetIntervalProfile(
	IN ULONG ProfileInterval,
	IN KPROFILE_SOURCE ProfileSource
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetIntervalProfile(
	IN ULONG ProfileInterval,
	IN KPROFILE_SOURCE ProfileSource
);

NTSYSAPI
NTSTATUS
NTAPI
NtQuerySystemEnvironmentValue(
	PUNICODE_STRING Variablename, //Only allowed value for this is "LastKnownGood"
	PVOID ValueBuffer,			// Returns "TRUE" or "FALSE"
	USHORT ValueBufferSize,
	PUSHORT BytesReturned
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemEnvironmentValue(
	PUNICODE_STRING Variablename, //Only allowed value for this is "LastKnownGood"
	PVOID ValueBuffer,			// Returns "TRUE" or "FALSE"
	USHORT ValueBufferSize,
	PUSHORT BytesReturned
);


/* The information, whether LastKnownGood is "TRUE" or "FALSE" is kept in CMOS
status register (offset 0x0B in CMOS) */
NTSYSAPI
NTSTATUS
NTAPI
NtSetSystemEnvironmentValue(
	PUNICODE_STRING Variablename, //Only allowed value for this is "LastKnownGood"
	PUNICODE_STRING ValueBuffer			// Only allowed value for this is "TRUE" or "FALSE"
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemEnvironmentValue(
	PUNICODE_STRING Variablename, //Only allowed value for this is "LastKnownGood"
	PUNICODE_STRING ValueBuffer			// Only allowed value for this is "TRUE" or "FALSE"
);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateNamedPipeFile(
	OUT HANDLE hPipe,
	IN ACCESS_MASK DesiredAccess,
	IN OBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
	IN ULONG PipeType,
	IN ULONG PipeReadMode,
	IN ULONG PipeWaitMode,
	IN ULONG nMaxInstances,
	IN ULONG nOutBufferSize,
	IN ULONG nInBufferSize,
	IN PLARGE_INTEGER DefaultTimeOut
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateNamedPipeFile(
	OUT HANDLE hPipe,
	IN ACCESS_MASK DesiredAccess,
	IN OBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
	IN ULONG PipeType,
	IN ULONG PipeReadMode,
	IN ULONG PipeWaitMode,
	IN ULONG nMaxInstances,
	IN ULONG nOutBufferSize,
	IN ULONG nInBufferSize,
	IN PLARGE_INTEGER DefaultTimeOut
);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateMailSlotFile(
	OUT HANDLE hMailSlot,
	IN ACCESS_MASK DesiredAccess,
	IN OBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
    IN ULONG CreateOptions,
	IN ULONG Unknown, //CreateMailSlot calls always passes 0
	IN ULONG nMaxMessageSize,
	IN PLARGE_INTEGER ReadTimeout
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateMailSlotFile(
	OUT HANDLE hMailSlot,
	IN ACCESS_MASK DesiredAccess,
	IN OBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
    IN ULONG CreateOptions,
	IN ULONG Unknown, //CreateMailSlot calls always passes 0
	IN ULONG nMaxMessageSize,
	IN PLARGE_INTEGER ReadTimeout
);

NTSYSAPI
NTSTATUS
NTAPI
NtFlushInstructionCache(
	IN HANDLE hProcess,
	IN PVOID BaseAddressRegion,
	IN ULONG RegionSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushInstructionCache(
	IN HANDLE hProcess,
	IN PVOID BaseAddressRegion,
	IN ULONG RegionSize
);

// This system service calls a HAL function called KeFlushWriteBuffer. This function
// just returns on intel implementation. Probably on other platforms, it does some
// real work. The WRITE_PORT_UCHAR etc, macros calls this function.
NTSYSAPI
NTSTATUS
NTAPI
NtFlushWriteBuffer(
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushWriteBuffer(
);

NTSYSAPI
NTSTATUS
NTAPI
NtQueryDefaultLocale(
	IN BOOLEAN bSystemOrThreadLocale, //System-FALSE, Thread=TRUE
	OUT PULONG DefaultLocale
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale(
	IN BOOLEAN bSystemOrThreadLocale, //System-TRUE, Thread=FALSE
	OUT PULONG DefaultLocale
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetDefaultLocale(
	IN BOOLEAN bSystemOrThreadLocale, //System-FALSE, Thread=TRUE
	IN ULONG DefaultLocale
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale(
	IN BOOLEAN bSystemOrThreadLocale, //System-FALSE, Thread=TRUE
	IN ULONG DefaultLocale
);
/*

Following system services return STATUS_NOT_IMPLEMENTED

NtAllocateVirtualMemory64 (6 parameters)
NtFreeVirtualMemory64 (4 parameters)
NtMapViewOfVlmSection (7 parameters)
NtProtectVirtualMemory64 (5 parameters)
NtQueryVirtualMemory64 (6 parameters)
NtReadFile64 (9 parameters)
NtReadVirtualMemory64 (5 parameters)
NtUnmapViewOfVlmSection (2 parameters)
NtWriteFile64 (9 parameters)
NtWriteVirtualMemory64 (5 parameters)
NtCreateChannel (2 parameters)
NtListenChannel (2 parameters)
NtOpenChannel (2 parameters)
NtReplyWaitSendChannel (3 parameters)
NtSendWaitReplyChannel (4 parameters)
NtSetContextChannel (1 parameters)
*/

#endif /* _UNDOCNT_H */
