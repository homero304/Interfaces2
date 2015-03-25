// Control.cpp -- IOCTL handlers for ParallelPort_IO_Interrupt driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"
#include "ioctls.h"

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchControl(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchControl
	PAGED_CODE();
	static char programmersNames[] = "Jose Fernando Luna - A01175073 | Homero Marin - A00810150";
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	NTSTATUS status = STATUS_SUCCESS;
	ULONG info = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	switch (code)
		{						// process request

	case GET_DRIVER_VERSION_CTL_CODE:				// code == 0x800
		{						// GET_DRIVER_VERSION_CTL_CODE
			if(cbout < sizeof(ULONG)) {
				status = STATUS_INVALID_BUFFER_SIZE;
			} else {
				PULONG pVersion = (PULONG) Irp->AssociatedIrp.SystemBuffer;
				*pVersion = 0x20150003;
				info = sizeof(ULONG);
			}
		break;
		}						// GET_DRIVER_VERSION_CTL_CODE

	case GET_PROGRAMMERS_NAMES_CTL_CODE:				// code == 0x801
		{						// GET_PROGRAMMERS_NAMES_CTL_CODE
			if(cbout < sizeof(ULONG)) {
				status = STATUS_INVALID_BUFFER_SIZE;
			} else {
				PUCHAR pProgrammers = (PUCHAR) Irp->AssociatedIrp.SystemBuffer;
				unsigned int programmersLength = strlen(programmersNames);
				for(unsigned int i = 0; i < programmersLength; i++) {
					*pProgrammers = programmersNames[i];
					pProgrammers++;
				}
				*pProgrammers = '\0';
				info = programmersLength + 1;
			}
		break;
		}						// GET_PROGRAMMERS_NAMES_CTL_CODE

	case INTERRUPT_IRP_PENDING:				// code == 0x802
		{						// INTERRUPT_IRP_PENDING
			pdx->NotifyIrp = Irp;
			IoMarkIrpPending(Irp);
			return STATUS_PENDING;
		break;
		}						// INTERRUPT_IRP_PENDING

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;

		}						// process request

	return CompleteRequest(Irp, status, info);
	}							// DispatchControl

#pragma LOCKEDCODE				// force inline functions into nonpaged code

