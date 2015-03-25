// Main program for InterruptIODriver driver
// Copyright (C) 2001 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo);
VOID DriverUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS OnRequestComplete(IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PKEVENT pev);

UNICODE_STRING servkey;

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
	{							// DriverEntry
	KdPrint((DRIVERNAME " - Entering DriverEntry: DriverObject %8.8lX\n", DriverObject));

	// Save the name of the service key

	servkey.Buffer = (PWSTR) ExAllocatePool(PagedPool, RegistryPath->Length + sizeof(WCHAR));
	if (!servkey.Buffer)
		{
		KdPrint((DRIVERNAME " - Unable to allocate %d bytes for copy of service key name\n", RegistryPath->Length + sizeof(WCHAR)));
		return STATUS_INSUFFICIENT_RESOURCES;
		}
	servkey.MaximumLength = RegistryPath->Length + sizeof(WCHAR);
	RtlCopyUnicodeString(&servkey, RegistryPath);
	servkey.Buffer[RegistryPath->Length / 2] = 0;	// add a null terminator

	// Initialize function pointers

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchReadWrite;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchReadWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchControl;
	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = DispatchWmi;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	
	return STATUS_SUCCESS;
	}							// DriverEntry

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
	{							// DriverUnload
	PAGED_CODE();
	KdPrint((DRIVERNAME " - Entering DriverUnload: DriverObject %8.8lX\n", DriverObject));
	RtlFreeUnicodeString(&servkey);
	}							// DriverUnload

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo)
	{							// AddDevice
	PAGED_CODE();
	KdPrint((DRIVERNAME " - Entering AddDevice: DriverObject %8.8lX, pdo %8.8lX\n", DriverObject, pdo));

	NTSTATUS status;

	// Create a function device object to represent the hardware we're managing.

	PDEVICE_OBJECT fdo;
	#define xsize sizeof(DEVICE_EXTENSION)
	
	UNICODE_STRING devname;
	WCHAR namebuf[32];
	static LONG devcount = -1;
	_snwprintf(namebuf, arraysize(namebuf), L"\\Device\\INTERRUPTIODRIVER_%d", InterlockedIncrement(&devcount));
	RtlInitUnicodeString(&devname, namebuf);

	status = IoCreateDevice(DriverObject, xsize, &devname,
		FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &fdo);
	if (!NT_SUCCESS(status))
		{						// can't create device object
		KdPrint((DRIVERNAME " - IoCreateDevice failed - %X\n", status));
		return status;
		}						// can't create device object
	
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// From this point forward, any error will have side effects that need to
	// be cleaned up. Using a do-once block allows us to modify the program
	// easily without losing track of the side effects.

	do
		{						// finish initialization
		pdx->DeviceObject = fdo;
		pdx->Pdo = pdo;
		IoInitializeRemoveLock(&pdx->RemoveLock, 0, 0, 0);
		pdx->state = STOPPED;		// device starts in the stopped state

		// TODO If you need several queues instead of just one, follow the suggestion in
		// driver.h about creating an array of pointers. Your code at this point would
		// then look like this:
		//	InitializeQueue(&pdx->dqRead, StartIoRead);
		//	InitializeQueue(&pdx->dqWrite, StartIoWrite);
		//	pdx->queues[0] = &pdx->dqRead;
		//	pdx->queues[1] = &pdx->dqWrite;

		InitializeQueue(&pdx->dqReadWrite, StartIo);

		// Make a copy of the device name

		pdx->devname.Buffer = (PWCHAR) ExAllocatePool(NonPagedPool, devname.MaximumLength);
		if (!pdx->devname.Buffer)
			{					// can't allocate buffer
			status = STATUS_INSUFFICIENT_RESOURCES;
			KdPrint((DRIVERNAME " - Unable to allocate %d bytes for copy of name\n", devname.MaximumLength));
			break;
			}					// can't allocate buffer
		pdx->devname.MaximumLength = devname.MaximumLength;
		RtlCopyUnicodeString(&pdx->devname, &devname);

		// Declare the buffering method we'll use for read/write requests

		fdo->Flags |= DO_BUFFERED_IO;

		// Initialize DPC object

		IoInitializeDpcRequest(fdo, DpcForIsr);

		// Link our device object into the stack leading to the PDO
		
		pdx->LowerDeviceObject = IoAttachDeviceToDeviceStack(fdo, pdo);
		if (!pdx->LowerDeviceObject)
			{						// can't attach device
			KdPrint((DRIVERNAME " - IoAttachDeviceToDeviceStack failed\n"));
			status = STATUS_DEVICE_REMOVED;
			break;
			}						// can't attach device

		// Set power management flags in the device object

		fdo->Flags |= DO_POWER_PAGABLE;

		// Indicate that our initial power state is D0 (fully on). Also indicate that
		// we have a pagable power handler (otherwise, we'll never get idle shutdown
		// messages!)

		pdx->syspower = PowerSystemWorking;
		pdx->devpower = PowerDeviceD0;
		POWER_STATE state;
		state.DeviceState = PowerDeviceD0;
		PoSetPowerState(fdo, DevicePowerState, state);

		// Clear the "initializing" flag so that we can get IRPs

		fdo->Flags &= ~DO_DEVICE_INITIALIZING;
		}						// finish initialization
	while (FALSE);

	if (!NT_SUCCESS(status))
		{					// need to cleanup
		if (pdx->devname.Buffer)
			RtlFreeUnicodeString(&pdx->devname);
		if (pdx->LowerDeviceObject)
			IoDetachDevice(pdx->LowerDeviceObject);
		IoDeleteDevice(fdo);
		}					// need to cleanup

	return status;
	}							// AddDevice

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info)
	{							// CompleteRequest
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
	}							// CompleteRequest

NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status)
	{							// CompleteRequest
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
	}							// CompleteRequest

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS ForwardAndWait(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// ForwardAndWait
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	PAGED_CODE();
	
	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) OnRequestComplete,
		(PVOID) &event, TRUE, TRUE, TRUE);

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	IoCallDriver(pdx->LowerDeviceObject, Irp);
	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	return Irp->IoStatus.Status;
	}							// ForwardAndWait

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS OnRequestComplete(IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PKEVENT pev)
	{							// OnRequestComplete
	KeSetEvent(pev, 0, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
	}							// OnRequestComplete

///////////////////////////////////////////////////////////////////////////////

VOID EnableAllInterfaces(PDEVICE_EXTENSION pdx, BOOLEAN enable)
	{							// EnableAllInterfaces
	}							// EnableAllInterfaces

///////////////////////////////////////////////////////////////////////////////

VOID DeregisterAllInterfaces(PDEVICE_EXTENSION pdx)
	{							// DeregisterAllInterfaces
	}							// DeregisterAllInterfaces

// This dispatch routine for IRP_MJ_SYSTEM_CONTROL is a dummy generated by the
// wizard because the Driver Verifier requires one. Note that this routine
// only appears here because you did *not* specify this IRP type in the wizard
// dialogs.

NTSTATUS DispatchWmi(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchWmi
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(pdx->LowerDeviceObject, Irp);
	}							// DispatchWmi

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID RemoveDevice(IN PDEVICE_OBJECT fdo)
	{							// RemoveDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status;

	RtlFreeUnicodeString(&pdx->devname);

	if (pdx->LowerDeviceObject)
		IoDetachDevice(pdx->LowerDeviceObject);

	IoDeleteDevice(fdo);
	}							// RemoveDevice

#if DBG && defined(_X86_)
#pragma LOCKEDCODE

extern "C" void __declspec(naked) __cdecl _chkesp()
	{
	_asm je okay
	ASSERT(!DRIVERNAME " - Stack pointer mismatch!");
okay:
	_asm ret
	}

#endif // DBG