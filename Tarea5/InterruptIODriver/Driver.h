// Declarations for InterruptIODriver driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#pragma once

#define DRIVERNAME "INTERRUPTIODRIVER"				// for use in messages
#define LDRIVERNAME L"INTERRUPTIODRIVER"				// for use in UNICODE string constants
#include "DevQueue.h"

///////////////////////////////////////////////////////////////////////////////
// Device extension structure

enum DEVSTATE {
	STOPPED,								// device stopped
	WORKING,								// started and working
	PENDINGSTOP,							// stop pending
	PENDINGREMOVE,							// remove pending
	SURPRISEREMOVED,						// removed by surprise
	REMOVED,								// removed
	};

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT DeviceObject;			// device object this extension belongs to
	PDEVICE_OBJECT LowerDeviceObject;		// next lower driver in same stack
	PDEVICE_OBJECT Pdo;						// the PDO
	IO_REMOVE_LOCK RemoveLock;				// removal control locking structure
	UNICODE_STRING devname;
	DEVSTATE state;							// current state of device
	DEVSTATE prevstate;						// state prior to removal query
	DEVICE_POWER_STATE devpower;			// current device power state
	SYSTEM_POWER_STATE syspower;			// current system power state
	DEVICE_CAPABILITIES devcaps;			// copy of most recent device capabilities

	// TODO Instead of using a single DEVQUEUE for both reads and writes, you could
	// create several queues and define an array of pointers to them. I.e.:
	//	DEVQUEUE dqRead;
	//	DEVQUEUE dqWrite;
	//	PDEVQUEUE queues[2];

	DEVQUEUE dqReadWrite;					// queue for reads and writes
	LONG handles;							// # open handles
	PKINTERRUPT InterruptObject;			// address of interrupt object
	PUCHAR portbase;						// I/O port base address
	ULONG nports;							// number of assigned ports

	// TODO add additional per-device declarations

	BOOLEAN mappedport;						// true if we mapped port addr in StartDevice
	BOOLEAN busy;							// true if device busy with a request
	BOOLEAN StalledForPower;				// power management has stalled IRP queue
	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

///////////////////////////////////////////////////////////////////////////////
// Global functions

VOID RemoveDevice(IN PDEVICE_OBJECT fdo);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status);
NTSTATUS ForwardAndWait(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS SendDeviceSetPower(PDEVICE_EXTENSION fdo, DEVICE_POWER_STATE state, BOOLEAN wait = FALSE);
VOID SendAsyncNotification(PVOID context);
VOID EnableAllInterfaces(PDEVICE_EXTENSION pdx, BOOLEAN enable);
VOID DeregisterAllInterfaces(PDEVICE_EXTENSION pdx);
NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated);

// TODO If you decide to queue reads and writes separately, you need
// separate StartIo routines too. E.g.:
//	VOID StartIoRead(PDEVICE_OBJECT fdo, PIRP Irp);
//	VOID StartioWrite(PDEVICE_OBJECT fdo, PIRP Irp);

VOID StartIo(PDEVICE_OBJECT fdo, PIRP Irp);
VOID StopDevice(PDEVICE_OBJECT fdo, BOOLEAN oktouch = FALSE);
VOID DpcForIsr(PKDPC Dpc, PDEVICE_OBJECT fdo, PIRP junk, PDEVICE_EXTENSION pdx);

// I/O request handlers

NTSTATUS DispatchCreate(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchClose(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchReadWrite(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchControl(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPower(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchWmi(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp);

#define win98 FALSE
extern UNICODE_STRING servkey;
