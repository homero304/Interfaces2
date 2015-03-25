// DevQueue.h -- Declarations for custom IRP queuing package
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#ifndef DEVQUEUE_H
#define DEVQUEUE_H

typedef VOID (__stdcall *PQNOTIFYFUNC)(PVOID);

typedef struct _DEVQUEUE {
	LIST_ENTRY head;
	KSPIN_LOCK lock;
	PDRIVER_STARTIO StartIo;
	LONG stallcount;
	PIRP CurrentIrp;
	KEVENT evStop;
	PQNOTIFYFUNC notify;
	PVOID notifycontext;
	NTSTATUS abortstatus;
	} DEVQUEUE, *PDEVQUEUE;

VOID NTAPI AbortAllRequests(PDEVQUEUE* q, ULONG nq, NTSTATUS status);
VOID NTAPI AbortRequests(PDEVQUEUE pdq, NTSTATUS status);
VOID NTAPI AllowAllRequests(PDEVQUEUE* q, ULONG nq);
VOID NTAPI AllowRequests(PDEVQUEUE pdq);
NTSTATUS NTAPI AreRequestsBeingAborted(PDEVQUEUE pdq);
VOID NTAPI CancelRequest(PDEVQUEUE pdq, PIRP Irp);
BOOLEAN	NTAPI CheckAnyBusyAndStall(PDEVQUEUE* q, ULONG nq, PDEVICE_OBJECT fdo);
BOOLEAN NTAPI CheckBusyAndStall(PDEVQUEUE pdq);
VOID NTAPI CleanupAllRequests(PDEVQUEUE* q, ULONG nq, PFILE_OBJECT fop, NTSTATUS status);
VOID NTAPI CleanupRequests(PDEVQUEUE pdq, PFILE_OBJECT fop, NTSTATUS status);
PIRP NTAPI GetCurrentIrp(PDEVQUEUE pdq);
VOID NTAPI InitializeQueue(PDEVQUEUE pdq, PDRIVER_STARTIO StartIo);
VOID NTAPI RestartAllRequests(PDEVQUEUE* q, ULONG nq, PDEVICE_OBJECT fdo);
VOID NTAPI RestartRequests(PDEVQUEUE pdq, PDEVICE_OBJECT fdo);
VOID NTAPI StallAllRequests(PDEVQUEUE* q, ULONG nq);
VOID NTAPI StallRequests(PDEVQUEUE pdq);
NTSTATUS NTAPI StallAllRequestsAndNotify(PDEVQUEUE* q, ULONG nq, PQNOTIFYFUNC notify, PVOID context);
NTSTATUS NTAPI StallRequestsAndNotify(PDEVQUEUE pdq, PQNOTIFYFUNC notify, PVOID context);
PIRP NTAPI StartNextPacket(PDEVQUEUE pdq, PDEVICE_OBJECT fdo);
VOID NTAPI StartPacket(PDEVQUEUE pdq, PDEVICE_OBJECT fdo, PIRP Irp, PDRIVER_CANCEL cancel);
VOID NTAPI WaitForCurrentIrp(PDEVQUEUE pdq);
VOID NTAPI WaitForCurrentIrps(PDEVQUEUE* q, ULONG nq);

#endif
