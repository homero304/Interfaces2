// Plug and Play handlers for ParallelPort_button driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

NTSTATUS DefaultPnpHandler(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleCancelRemove(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleCancelStop(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleQueryCapabilities(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleQueryRemove(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleQueryStop(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleRemoveDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleStartDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleStopDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleSurpriseRemoval(IN PDEVICE_OBJECT fdo, IN PIRP Irp);

VOID AdjustDeviceCapabilities(PDEVICE_EXTENSION pdx, PDEVICE_CAPABILITIES pdc);
VOID AdjustWakeCapabilities(PDEVICE_CAPABILITIES pdc, DEVICE_POWER_STATE dstate);
#if DBG
VOID ShowResources(IN PCM_PARTIAL_RESOURCE_LIST list);

static char* statenames[] = {
	"STOPPED",
	"WORKING",
	"PENDINGSTOP",
	"PENDINGREMOVE",
	"SURPRISEREMOVED",
	"REMOVED",
	};

#endif

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// DispatchPnp
	PAGED_CODE();
	KdPrint((DRIVERNAME " - Entering DispatchPNP"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status);

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ASSERT(stack->MajorFunction == IRP_MJ_PNP);

	static NTSTATUS (*fcntab[])(IN PDEVICE_OBJECT fdo, IN PIRP Irp) = {
		HandleStartDevice,		// IRP_MN_START_DEVICE
		HandleQueryRemove,		// IRP_MN_QUERY_REMOVE_DEVICE
		HandleRemoveDevice,		// IRP_MN_REMOVE_DEVICE
		HandleCancelRemove,		// IRP_MN_CANCEL_REMOVE_DEVICE
		HandleStopDevice,		// IRP_MN_STOP_DEVICE
		HandleQueryStop,		// IRP_MN_QUERY_STOP_DEVICE
		HandleCancelStop,		// IRP_MN_CANCEL_STOP_DEVICE
		DefaultPnpHandler,		// IRP_MN_QUERY_DEVICE_RELATIONS
		DefaultPnpHandler,		// IRP_MN_QUERY_INTERFACE
		HandleQueryCapabilities,// IRP_MN_QUERY_CAPABILITIES
		DefaultPnpHandler,		// IRP_MN_QUERY_RESOURCES
		DefaultPnpHandler,		// IRP_MN_QUERY_RESOURCE_REQUIREMENTS
		DefaultPnpHandler,		// IRP_MN_QUERY_DEVICE_TEXT
		DefaultPnpHandler,		// IRP_MN_FILTER_RESOURCE_REQUIREMENTS
		DefaultPnpHandler,		// 
		DefaultPnpHandler,		// IRP_MN_READ_CONFIG
		DefaultPnpHandler,		// IRP_MN_WRITE_CONFIG
		DefaultPnpHandler,		// IRP_MN_EJECT
		DefaultPnpHandler,		// IRP_MN_SET_LOCK
		DefaultPnpHandler,		// IRP_MN_QUERY_ID
		DefaultPnpHandler,		// IRP_MN_QUERY_PNP_DEVICE_STATE
		DefaultPnpHandler,		// IRP_MN_QUERY_BUS_INFORMATION
		DefaultPnpHandler,		// IRP_MN_DEVICE_USAGE_NOTIFICATION
		HandleSurpriseRemoval,	// IRP_MN_SURPRISE_REMOVAL
		};

	ULONG fcn = stack->MinorFunction;
	if (fcn >= arraysize(fcntab))
		{						// unknown function
		status = DefaultPnpHandler(fdo, Irp); // some function we don't know about
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
		return status;
		}						// unknown function

#if DBG
	static char* fcnname[] = {
		"IRP_MN_START_DEVICE",
		"IRP_MN_QUERY_REMOVE_DEVICE",
		"IRP_MN_REMOVE_DEVICE",
		"IRP_MN_CANCEL_REMOVE_DEVICE",
		"IRP_MN_STOP_DEVICE",
		"IRP_MN_QUERY_STOP_DEVICE",
		"IRP_MN_CANCEL_STOP_DEVICE",
		"IRP_MN_QUERY_DEVICE_RELATIONS",
		"IRP_MN_QUERY_INTERFACE",
		"IRP_MN_QUERY_CAPABILITIES",
		"IRP_MN_QUERY_RESOURCES",
		"IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
		"IRP_MN_QUERY_DEVICE_TEXT",
		"IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
		"",
		"IRP_MN_READ_CONFIG",
		"IRP_MN_WRITE_CONFIG",
		"IRP_MN_EJECT",
		"IRP_MN_SET_LOCK",
		"IRP_MN_QUERY_ID",
		"IRP_MN_QUERY_PNP_DEVICE_STATE",
		"IRP_MN_QUERY_BUS_INFORMATION",
		"IRP_MN_DEVICE_USAGE_NOTIFICATION",
		"IRP_MN_SURPRISE_REMOVAL",
		};

	KdPrint((DRIVERNAME " - PNP Request (%s)\n", fcnname[fcn]));
#endif // DBG

	status = (*fcntab[fcn])(fdo, Irp);
	if (fcn != IRP_MN_REMOVE_DEVICE)
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
	}							// DispatchPnp

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

NTSTATUS DefaultPnpHandler(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// DefaultPnpHandler
	IoSkipCurrentIrpStackLocation(Irp);
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	return IoCallDriver(pdx->LowerDeviceObject, Irp);
	}							// DefaultPnpHandler

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleCancelRemove(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleCancelRemove
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	if (pdx->state == PENDINGREMOVE)
		{						// we succeeded earlier query

		// Lower-level drivers are presumably in the pending-remove state as
		// well, so we need to tell them that the remove has been cancelled
		// before we start sending IRPs down to them.

		NTSTATUS status = ForwardAndWait(fdo, Irp); // wait for lower layers
		if (NT_SUCCESS(status))
			{					// completed successfully
			KdPrint((DRIVERNAME " - To %s from PENDINGREMOVE\n", statenames[pdx->prevstate]));

			// TODO If you use multiple queues, code this instead:
			//	RestartAllRequests(pdx->queues, arraysize(pdx->queues), fdo);

			RestartRequests(&pdx->dqReadWrite, fdo);
			}					// completed successfully
		else
			KdPrint((DRIVERNAME " - Status %8.8lX returned by PDO for IRP_MN_CANCEL_REMOVE_DEVICE", status));

		return CompleteRequest(Irp, status);
		}						// we succeeded earlier query
	
	return DefaultPnpHandler(fdo, Irp); // unexpected cancel
	}							// HandleCancelRemove

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleCancelStop(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleCancelStop
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	if (pdx->state == PENDINGSTOP)
		{						// we succeeded earlier query

		// Lower level drivers are presumably in the pending-stop state as
		// well, so we need to tell them that the stop has been cancelled
		// before we start sending IRPs down to them.

		NTSTATUS status = ForwardAndWait(fdo, Irp); // wait for lower layers
		if (NT_SUCCESS(status))
			{					// completed successfully
			KdPrint((DRIVERNAME " - To WORKING from PENDINGSTOP\n"));
			pdx->state = WORKING;

			// TODO If you use multiple queues, code this instead:
			//	RestartAllRequests(pdx->queues, arraysize(pdx->queues), fdo);

			RestartRequests(&pdx->dqReadWrite, fdo);
			}					// completed successfully
		else
			KdPrint((DRIVERNAME " - Status %8.8lX returned by PDO for IRP_MN_CANCEL_STOP_DEVICE", status));
		
		return CompleteRequest(Irp, status);
		}						// we succeeded earlier query

	return DefaultPnpHandler(fdo, Irp); // unexpected cancel
	}							// HandleCancelStop

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleQueryCapabilities(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleQueryCapabilities
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_QUERY_CAPABILITIES);
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	PDEVICE_CAPABILITIES pdc = stack->Parameters.DeviceCapabilities.Capabilities;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Check to be sure we know how to handle this version of the capabilities structure

	if (pdc->Version < 1)
		return DefaultPnpHandler(fdo, Irp);

	NTSTATUS status = ForwardAndWait(fdo, Irp);
	if (NT_SUCCESS(status))
		{						// IRP succeeded
		stack = IoGetCurrentIrpStackLocation(Irp);
		pdc = stack->Parameters.DeviceCapabilities.Capabilities;

		// TODO Modify any capabilities that must be set on the way back up

		AdjustDeviceCapabilities(pdx, pdc);	// compensate for bus driver shortcoming
		pdx->devcaps = *pdc;	// save capabilities for whoever needs to see them
		}						// IRP succeeded

	return CompleteRequest(Irp, status);
	}							// HandleQueryCapabilities

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleQueryRemove(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleQueryRemove
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_QUERY_REMOVE_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	if (pdx->state == WORKING)
		{						// currently working

	#ifdef _X86_

		// Win98 doesn't check for open handles before allowing a remove to proceed,
		// and it may deadlock in IoReleaseRemoveLockAndWait if handles are still
		// open.

		if (win98 && pdx->DeviceObject->ReferenceCount)
			{
			KdPrint((DRIVERNAME " - Failing removal query due to open handles\n"));
			return CompleteRequest(Irp, STATUS_DEVICE_BUSY);
			}

	#endif
		// TODO If you use multiple queues, code this instead:
		//	if (CheckAnyBusyAndStall(pdx->queues, arraysize(pdx->queues), fdo);
		if (CheckBusyAndStall(&pdx->dqReadWrite))
			return CompleteRequest(Irp, STATUS_UNSUCCESSFUL);
		KdPrint((DRIVERNAME " - To PENDINGREMOVE from %s\n", statenames[pdx->state]));
		}						// currently working

	// Save current state for restoration if the query gets cancelled.
	// (We can now be stopped or working)

	pdx->prevstate = pdx->state;
	pdx->state = PENDINGREMOVE;
	return DefaultPnpHandler(fdo, Irp);
	}							// HandleQueryRemove

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleQueryStop(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleQueryStop
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_QUERY_STOP_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	
	// Boot devices may get this query before they even start, so check to see
	// if we're in the WORKING state before doing anything.

	if (pdx->state != WORKING)
		return DefaultPnpHandler(fdo, Irp);

		// TODO If you use multiple queues, code this instead:
		//	if (CheckAnyBusyAndStall(pdx->queues, arraysize(pdx->queues), fdo);
		if (CheckBusyAndStall(&pdx->dqReadWrite))
			return CompleteRequest(Irp, STATUS_UNSUCCESSFUL);
	KdPrint((DRIVERNAME " - To PENDINGSTOP from %s\n", statenames[pdx->state]));
	pdx->state = PENDINGSTOP;
	return DefaultPnpHandler(fdo, Irp);
	}							// HandleQueryStop

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleRemoveDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleRemoveDevice
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_REMOVE_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Cancel any queued IRPs and start rejecting new ones

	// TODO If you use multiple queues, code this instead:
	//	AbortAllRequests(pdx->queues, arraysize(pdx->queues), STATUS_DELETE_PENDING);

	AbortRequests(&pdx->dqReadWrite, STATUS_DELETE_PENDING);

	// Disable all device interfaces. This triggers PnP notifications that will 
	// allow apps to close their handles.

	DeregisterAllInterfaces(pdx);

	// Release our I/O resources

	StopDevice(fdo, pdx->state == WORKING);

	KdPrint((DRIVERNAME " - To REMOVED from %s\n", statenames[pdx->state]));
	pdx->state = REMOVED;

	// Let lower-level drivers handle this request. Ignore whatever
	// result eventuates.

	NTSTATUS status = DefaultPnpHandler(fdo, Irp);

	// Wait for all claims against this device to vanish before removing
	// the device object.

	IoReleaseRemoveLockAndWait(&pdx->RemoveLock, Irp);

	// Remove the device object

	RemoveDevice(fdo);

	return status;				// lower-level completed IoStatus already
	}							// HandleRemoveDevice

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleStartDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleStartDevice
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_START_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	NTSTATUS status = ForwardAndWait(fdo, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status);

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	PCM_PARTIAL_RESOURCE_LIST raw;
	if (stack->Parameters.StartDevice.AllocatedResources)
		raw = &stack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList;
	else
		raw = NULL;

	PCM_PARTIAL_RESOURCE_LIST translated;
	if (stack->Parameters.StartDevice.AllocatedResourcesTranslated)
		translated = &stack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList;
	else
		translated = NULL;

	status = StartDevice(fdo, raw, translated); 

	// While we were in the stopped state, we were stalling incoming requests.
	// Now we can release any pending IRPs and start processing new ones

	if (NT_SUCCESS(status))
		{						// started okay

		// Enable all registered device interfaces.

		EnableAllInterfaces(pdx, TRUE);

		KdPrint((DRIVERNAME " - To WORKING from %s\n", statenames[pdx->state]));
		pdx->state = WORKING;
		// TODO If you use multiple queues, code this instead:
		//	AllowAllRequests(pdx->queues, arraysize(pdx->queues));
		//	RestartAllRequests(pdx->queues, arraysize(pdx->queues), fdo);
		AllowRequests(&pdx->dqReadWrite); // in case we got a bogus STOP
		RestartRequests(&pdx->dqReadWrite, fdo);
		}						// started okay

	return CompleteRequest(Irp, status);
	}							// HandleStartDevice

///////////////////////////////////////////////////////////////////////////////	

NTSTATUS HandleStopDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleStopDevice
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_STOP_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// We're supposed to always get a query before we're stopped, so
	// we should already be in the PENDINGSTOP state. There's a Win98 bug that
	// can sometimes cause us to get a STOP instead of a REMOVE, in which case
	// we should start rejecting IRPs

	if (pdx->state != PENDINGSTOP)
		{						// no previous query
		KdPrint((DRIVERNAME " - STOP with no previous QUERY_STOP!\n"));
		// TODO If you use multiple queues, code this instead:
		//	AbortAllRequests(pdx->queues, arraysize(pdx->queues, STATUS_DELETE_PENDING);
		AbortRequests(&pdx->dqReadWrite, STATUS_DELETE_PENDING);
		}						// no previous query
	StopDevice(fdo, pdx->state == WORKING);
	KdPrint((DRIVERNAME " - To STOPPED from %s\n", statenames[pdx->state]));
	pdx->state = STOPPED;
	return DefaultPnpHandler(fdo, Irp);
	}							// HandleStopDevice

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleSurpriseRemoval(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleSurpriseRemoval
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_SURPRISE_REMOVAL);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Cancel any queued IRPs and start rejecting new ones
	// TODO If you use multiple queues, code this instead:
	//	AbortAllRequests(pdx->queues, arraysize(pdx->queues, STATUS_DELETE_PENDING);
	AbortRequests(&pdx->dqReadWrite, STATUS_DELETE_PENDING);
	EnableAllInterfaces(pdx, FALSE);
	KdPrint((DRIVERNAME " - To SURPRISEREMOVED from %s\n", statenames[pdx->state]));

	BOOLEAN oktouch = pdx->state == WORKING;
	pdx->state = SURPRISEREMOVED;
	StopDevice(fdo, oktouch);

	return DefaultPnpHandler(fdo, Irp);
	}							// HandleSurpriseRemoval
	
///////////////////////////////////////////////////////////////////////////////
// AdjustDeviceCapabilities corrects the reported device capabilities to
// workaround the fact that many back-level bus drivers simply don't report
// them correctly. Cf. toaster.sys sample in the DDK and talk given by
// Adrian Oney (no relation) at WinHEC 2002.

#pragma PAGEDCODE

__inline SYSTEM_POWER_STATE& operator++(SYSTEM_POWER_STATE& ss) {ss = (SYSTEM_POWER_STATE) (ss + 1); return ss;}
__inline SYSTEM_POWER_STATE& operator--(SYSTEM_POWER_STATE& ss) {ss = (SYSTEM_POWER_STATE) (ss - 1); return ss;}

VOID AdjustDeviceCapabilities(PDEVICE_EXTENSION pdx, PDEVICE_CAPABILITIES pdc)
	{							// AdjustDeviceCapabilities

	// Adjust power capabilities to compensate for bus driver written to
	// WDM 1.0 spec. First infer D1 and D2 capabilities from S->D state map.

	for (SYSTEM_POWER_STATE sstate = PowerSystemSleeping1; sstate <= PowerSystemHibernate; ++sstate)
		{					// for each S-state
		if (pdc->DeviceState[sstate] == PowerDeviceD1)
			pdc->DeviceD1 = TRUE;
		if (pdc->DeviceState[sstate] == PowerDeviceD2)
			pdc->DeviceD2 = TRUE;
		}					// for each S-state

	// Set the WakeFromDx flags based on the reported DeviceWake state and
	// on the D-state corresponding to the reported SystemWake state

	AdjustWakeCapabilities(pdc, pdc->DeviceWake);
	if (pdc->SystemWake != PowerSystemUnspecified)
		AdjustWakeCapabilities(pdc, pdc->DeviceState[pdc->SystemWake]);

	// Find the deepest D-state from which this device can wake the system

	DEVICE_POWER_STATE wakestate = PowerDeviceD0;	// assume no wakeup capability

	if (pdc->WakeFromD3)
		wakestate = PowerDeviceD3;
	else if (pdc->WakeFromD2)
		wakestate = PowerDeviceD2;
	else if (pdc->WakeFromD1)
		wakestate = PowerDeviceD1;
	else if (pdc->WakeFromD0)
		wakestate = PowerDeviceD0;

	// If SystemWake is specified, the corresponding D-state had better be at
	// least as powered as the state we just discovered, or else there's a bug
	// in our bus driver...

	if (pdc->SystemWake != PowerSystemUnspecified)
		{
		ASSERT(pdc->DeviceState[pdc->SystemWake] <= wakestate);
		}

	// If SystemWake wasn't specified, infer it from the S->D state map by
	// finding the lowest S-state whose D-state is at least as powered as the
	// lowest D-state from which wakeup is possible (I think I got that right...)

	else if (wakestate != PowerDeviceD0 && wakestate != PowerDeviceUnspecified)
		{						// infer system wake state
		for (sstate = PowerSystemSleeping3; sstate >= PowerSystemWorking; --sstate)
			{					// for each S-state
			if (pdc->DeviceState[sstate] != PowerDeviceUnspecified
				&& pdc->DeviceState[sstate] <= wakestate)
				{				// found the S-state
				KdPrint((DRIVERNAME " - Inferring that wakeup from S%d state possible\n", sstate - 1));
				pdc->SystemWake = sstate;
				break;
				}				// found the S-state
			}					// for each S-state
		}						// infer system wake state
	}							// AdjustDeviceCapabilities

///////////////////////////////////////////////////////////////////////////////
// AdjustWakeCapabilities adjusts the wakeup capabilities for a device.	

VOID AdjustWakeCapabilities(PDEVICE_CAPABILITIES pdc, DEVICE_POWER_STATE dstate)
	{							// AdjustWakeCapabilities
	switch (dstate)
		{						// select on D-state
	case PowerDeviceUnspecified:
		break;
	case PowerDeviceD0:
		pdc->WakeFromD0 = TRUE;
		break;
	case PowerDeviceD1:
		pdc->DeviceD1 = TRUE;
		pdc->WakeFromD1 = TRUE;
		break;
	case PowerDeviceD2:
		pdc->DeviceD2 = TRUE;
		pdc->WakeFromD2 = TRUE;
		break;
	case PowerDeviceD3:
		pdc->WakeFromD3 = TRUE;
		break;
	default:
		ASSERT(FALSE);
		}						// select on D-state
	}							// AdjustWakeCapabilities

///////////////////////////////////////////////////////////////////////////////	

#if DBG

VOID ShowResources(IN PCM_PARTIAL_RESOURCE_LIST list)
	{							// ShowResources
	PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = list->PartialDescriptors;
	ULONG nres = list->Count;
	ULONG i;

	for (i = 0; i < nres; ++i, ++resource)
		{						// for each resource
		ULONG type = resource->Type;

		static char* name[] = {
			"CmResourceTypeNull",
			"CmResourceTypePort",
			"CmResourceTypeInterrupt",
			"CmResourceTypeMemory",
			"CmResourceTypeDma",
			"CmResourceTypeDeviceSpecific",
			"CmResourceTypeBusNumber",
			"CmResourceTypeDevicePrivate",
			"CmResourceTypeAssignedResource",
			"CmResourceTypeSubAllocateFrom",
			};

		KdPrint(("    type %s", type < arraysize(name) ? name[type] : "unknown"));

		switch (type)
			{					// select on resource type
		case CmResourceTypePort:
		case CmResourceTypeMemory:
			KdPrint((" start %8X%8.8lX length %X\n",
				resource->u.Port.Start.HighPart, resource->u.Port.Start.LowPart,
				resource->u.Port.Length));
			break;
		
		case CmResourceTypeInterrupt:
			KdPrint(("  level %X, vector %X, affinity %X\n",
				resource->u.Interrupt.Level, resource->u.Interrupt.Vector,
				resource->u.Interrupt.Affinity));
			break;
	
		case CmResourceTypeDma:
			KdPrint(("  channel %d, port %X\n",
				resource->u.Dma.Channel, resource->u.Dma.Port));
			}					// select on resource type
		}						// for each resource
	}							// ShowResources

#endif // DBG
