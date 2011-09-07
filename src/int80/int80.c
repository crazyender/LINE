/**
 * $Id: int80.c,v 1.2 2001/04/25 21:22:33 mvines Exp $
 */
#include <ntddk.h>
#include "int80.h"
#include "intel.h"

/*************************************/
/*  Select one of the following:     */
/*    "undocnt.h" for Windows NT     */
/*    "undocnt2k.h" or Windows 2000  */
/*************************************/

//#include "undocnt.h"
#include "undocnt2k.h"
#include "syscall.h"
/*************************************/






int OldHandler;
ULONG *ServiceCounterTable;
ULONG ServiceCounterTableSize;
int NumberOfServices;






ULONG GetProcessorCount()
{
	KAFFINITY procs;
	ULONG count = KeQueryActiveProcessorCount(&procs);
	return count;
}

NTSTATUS AddInterrupt(void) 
{
	ULONG processorCount;
	ULONG currentNo;
	PKDPC   tmp_dpc;
	ULONG index = 0;
	PKDPC   pdpc;
	processorCount = GetProcessorCount();
	if( processorCount == 1 )
	{
		SetupSystemCall(0, 0, 0, 0);
	}
	else
	{

		currentNo = KeGetCurrentProcessorNumber();
		SetupSystemCall(0, currentNo, 0, 0);
		gFinishedDPC = 1;
		tmp_dpc = (PKDPC)ExAllocatePoolWithTag(NonPagedPool, sizeof(KDPC)*processorCount, 'rm');
		pdpc = tmp_dpc;
		if( tmp_dpc )
		{
			for(index = 0 ; index < processorCount; index++, *tmp_dpc++ )
			{
				if( index == currentNo )
					continue;
				KeInitializeDpc(tmp_dpc, (PKDEFERRED_ROUTINE)SetupSystemCall, (PVOID)index);
				KeSetTargetProcessorDpc(tmp_dpc, index);
				KeInsertQueueDpc(tmp_dpc, NULL, NULL);
			}

			while( InterlockedCompareExchange(&gFinishedDPC, processorCount, processorCount) != processorCount )
			{
				__asm nop
			}

			ExFreePoolWithTag(pdpc, 'rm');
		}

	}

}





#pragma pack()


NTSTATUS DriverSpecificInitialization()
{
  int k=0;
 
  PIdtEntry_t IdtEntry;

    
  ServiceCounterTable=ExAllocatePool(PagedPool, SERVICECOUNTERS_BUFSIZE);

  if (!ServiceCounterTable) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  memset(ServiceCounterTable, 0, SERVICECOUNTERS_BUFSIZE);
  *ServiceCounterTable=SERVICECOUNTERS_BUFSIZE;

  /* Get the Base and Limit of IDTR Register */
  _asm sidt buffer
  
  IdtEntry=(PIdtEntry_t)Idtr->Base;
  return STATUS_SUCCESS;
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT  DriverObject, 
                     IN PUNICODE_STRING RegistryPath)
{
  MYDRIVERENTRY(DRIVER_DEVICE_NAME, FILE_DEVICE_HOOKINT, 
                DriverSpecificInitialization());
  return ntStatus;
}


void makeretval(void *f,const char *s)
{
  char *c=(char *)f;
  memcpy(c,ServiceCounterTable,8);
  strcpy(c+8,s);
}


NTSTATUS DriverDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  PIO_STACK_LOCATION  irpStack;
  PVOID               ioBuffer;
  ULONG               inputBufferLength;
  ULONG               outputBufferLength;
  ULONG               ioControlCode;
  NTSTATUS            ntStatus;

  Irp->IoStatus.Status      = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  irpStack = IoGetCurrentIrpStackLocation (Irp);

  ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
  inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
  outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

  switch (irpStack->MajorFunction) {
    
  case IRP_MJ_DEVICE_CONTROL:
    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (ioControlCode) {
    case IOCTL_HOOKINT_SYSTEM_SERVICE_USAGE:
      {
        int i;
  
        /* Check if sufficient sized buffer is provided to hold
           the counters for system service usage */
        if (outputBufferLength>=ServiceCounterTableSize) {
          /* Copy the counter information in user supplied buffer */
          memcpy(ioBuffer, ServiceCounterTable, ServiceCounterTableSize);
  
          /* Fill in the number of bytes to be returned to the caller */
          Irp->IoStatus.Information = ServiceCounterTableSize;
        } else {
          Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        break;
      }

    case IOCTL_ADDINT_SYSTEM_SERVICE_USAGE:
      if (useCount <= 0)  {
        if (STATUS_SUCCESS == AddInterrupt()) {
          makeretval(ioBuffer,"hooked");
        } else {
          makeretval(ioBuffer,"unable to hook");
        }
        
      } else {
        useCount++;
        makeretval(ioBuffer,"hooked already");
      }

      Irp->IoStatus.Information = SERVICECOUNTERS_BUFSIZE;
      break;
    
    case IOCTL_CALLPORT_SYSTEM_SERVICE_USAGE:
      makeretval(ioBuffer,"foo");

      Irp->IoStatus.Information = SERVICECOUNTERS_BUFSIZE;
      break;
    
    case IOCTL_REMOVEINT_SYSTEM_SERVICE_USAGE:
      if (useCount > 1) {
        useCount--;
        makeretval(ioBuffer,"not unhooked, multiple hooks");
        
      } else if(useCount == 1) {
        RemoveInterrupt();
        makeretval(ioBuffer,"unhooked");
        
      } else {
        makeretval(ioBuffer,"not hooked");
      }
      Irp->IoStatus.Information = SERVICECOUNTERS_BUFSIZE;
      break;
    
    default:
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      break;

    }
  
    break;
  }

  ntStatus = Irp->IoStatus.Status;

  IoCompleteRequest (Irp,IO_NO_INCREMENT);
  return ntStatus;
}


VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
  WCHAR               deviceLinkBuffer[]  = L"\\DosDevices\\"DRIVER_DEVICE_NAME;
  UNICODE_STRING      deviceLinkUnicodeString;
  PIdtEntry_t         IdtEntry;

  ExFreePool(ServiceCounterTable);

  if (useCount) {
    RemoveInterrupt();
  }

  RtlInitUnicodeString(&deviceLinkUnicodeString, deviceLinkBuffer);

  IoDeleteSymbolicLink(&deviceLinkUnicodeString);
  IoDeleteDevice (DriverObject->DeviceObject);
}
