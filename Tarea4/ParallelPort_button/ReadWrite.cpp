// Read/Write request processors for ParallelPort_button driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

VOID OnCancelReadWrite(PDEVICE_OBJECT fdo, PIRP Irp);

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchCreate(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchCreate
	PAGED_CODE();
	KdPrint((DRIVERNAME " - Entering DispatchCreate"));
	KdPrint((DRIVERNAME " - IRP_MJ_CREATE\n"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	NTSTATUS status = STATUS_SUCCESS;

	if (NT_SUCCESS(status))
		{						// okay to open
		if (InterlockedIncrement(&pdx->handles) == 1)
			{					// first open handle
			}					// okay to open
		}					// first open handle
	return CompleteRequest(Irp, status, 0);
	}							// DispatchCreate

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchClose(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchClose
	PAGED_CODE();
	KdPrint((DRIVERNAME " - Entering DispatchClose"));
	KdPrint((DRIVERNAME " - IRP_MJ_CLOSE\n"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	if (InterlockedDecrement(&pdx->handles) == 0)
		{						// no more open handles
		}						// no more open handles

	return CompleteRequest(Irp, STATUS_SUCCESS, 0);
	}							// DispatchClose

///////////////////////////////////////////////////////////////////////////////
// TODO If you use separate queues for reads and writes, you need to arrange to
// call StartPacket and CancelRequests with the right queue argument. The easiest
// way to do that is to have separate dispatch and cancel functions for IRP_MJ_READ and
// IRP_MJ_WRITE.

#pragma PAGEDCODE

#pragma PAGEDCODE

NTSTATUS DispatchRead(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchRead
	PAGED_CODE();
	KdPrint((DRIVERNAME " - Entering DispatchRead"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	

	PUCHAR pDato = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
	unsigned char dato = 0x00;
	unsigned char mask_status[]={0x80, 0x20, 0x10, 0x08, 0x80, 0x20, 0x10, 0x08};
	unsigned char control;
	unsigned char status;
	unsigned char MASK_CONTROL = 0x0B;
	unsigned char MASK_STATUS = 0x80;
	unsigned char i;
	unsigned char aux = 0x01;

	KdPrint(("PTOPAR_T3 - Entering DispatchRead\n"));
	
	for(i = 0; i < 8; i++, aux <<= 1)
	{
		if(i == 0) {
			control  = READ_PORT_UCHAR((unsigned char *) 0x37A);
			control = control ^ MASK_CONTROL;
			control = control & 0xF7;
			control = control ^ MASK_CONTROL;
			WRITE_PORT_UCHAR((unsigned char *) 0x37A, control);
		}

		if(i == 4) {
			control  = READ_PORT_UCHAR((unsigned char *) 0x37A);
			control = control ^ MASK_CONTROL;
			control = control | 0x08;
			control = control ^ MASK_CONTROL;
			WRITE_PORT_UCHAR((unsigned char *) 0x37A, control);
		}

			status = READ_PORT_UCHAR((unsigned char *) 0x379);
			status = status ^ MASK_STATUS;
			KdPrint(("PTOPAR_T3 ReadData = %d\n", status));
			if((status & mask_status[i]) != 0) {
				dato = dato | aux;
			}
	}

	KdPrint(("PTOPAR_T3 - DispatchRead %d\n", dato));

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 1;
	*pDato = dato;
	return STATUS_SUCCESS;
}

NTSTATUS DispatchWrite(PDEVICE_OBJECT fdo, PIRP Irp)
	{							
	PAGED_CODE();
	KdPrint((DRIVERNAME " - Entering DispatchWrite"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	PUCHAR pDato = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
	
	unsigned short aux = 0;
	unsigned char dato = (unsigned char)*pDato;
	unsigned char dataL;
	unsigned char dataH;
	unsigned char display;
	unsigned char control;
	unsigned char enable1 = 0x02;
	unsigned char enable2 = 0x08;
	unsigned char MASK_CONTROL = 0x0B;
	
	KdPrint(("PTOPAR_T3 - Entering DispatchWrite\n"));
	KdPrint(("PTOPAR_T3 - DispatchWrite Data Input: %x\n", dato));

	//Obtener primer byte de dato
	dataL = dato & 0x0F;
	KdPrint(("PTOPAR_T3 - DispatchWrite Low Data: %x\n", dataL));

	switch(dataL) {
	case 0:
		display = 72;
		break;
	case 1:
		display = 123;
		break;
	case 2:
		display = 44;
		break;
	case 3:
		display = 41;
		break;
	case 4:
		display = 27;
		break;
	case 5:
		display = 137;
		break;
	case 6:
		display = 136;
		break;
	case 7:
		display = 107;
		break;
	case 8:
		display = 8;
		break;
	case 9:
		display = 11;
		break;
	case 10:
		display = 10;
		break;
	case 11:
		display = 152;
		break;
	case 12:
		display = 204;
		break;
	case 13:
		display = 56;
		break;
	case 14:
		display = 140;
		break;
	case 15:
		display = 142;
		break;
	}

	//Leer registro de control
	control = READ_PORT_UCHAR((unsigned char *) 0x37A);		
	control = control ^ MASK_CONTROL;						
	control = control & !(enable1);				//				
	control = control ^MASK_CONTROL;
	//Escribir transicion negativa
	WRITE_PORT_UCHAR((unsigned char *) 0x37A, control);
	//Colocar dato en lineas de datos
	WRITE_PORT_UCHAR((unsigned char *)0x378, display);			
	control = control ^ MASK_CONTROL;
	control = control | enable1;								
	control = control ^MASK_CONTROL;
	//Escribir transicion positiva
	WRITE_PORT_UCHAR((unsigned char *) 0x37A, control);	

	//Obtener segundo byte de dato
	dataH = dato & 0xF0;
	dataH >>= 4;
	display = dataH;

	KdPrint(("PTOPAR_T3 - DispatchWrite High Data: %x\n", dataH));

	switch(dataH) {
	case 0:
		display = 72;
		break;
	case 1:
		display = 123;
		break;
	case 2:
		display = 44;
		break;
	case 3:
		display = 41;
		break;
	case 4:
		display = 27;
		break;
	case 5:
		display = 137;
		break;
	case 6:
		display = 136;
		break;
	case 7:
		display = 107;
		break;
	case 8:
		display = 8;
		break;
	case 9:
		display = 11;
		break;
	case 10:
		display = 10;
		break;
	case 11:
		display = 152;
		break;
	case 12:
		display = 204;
		break;
	case 13:
		display = 56;
		break;
	case 14:
		display = 140;
		break;
	case 15:
		display = 142;
		break;
	}

	//Leer registro de control
	control = READ_PORT_UCHAR((unsigned char *) 0x37A);		
	control = control ^ MASK_CONTROL;						
	control = control & !(enable2);				//				
	control = control ^MASK_CONTROL;
	//Escribir transicion negativa
	WRITE_PORT_UCHAR((unsigned char *) 0x37A, control);
	//Colocar dato en lineas de datos
	WRITE_PORT_UCHAR((unsigned char *)0x378, display);			
	control = control ^ MASK_CONTROL;
	control = control | enable2;								
	control = control ^MASK_CONTROL;
	//Escribir transicion positiva
	WRITE_PORT_UCHAR((unsigned char *) 0x37A, control);	


	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 1;
	return STATUS_SUCCESS;
}						// DispatchReadWrite

#pragma LOCKEDCODE

VOID OnCancelReadWrite(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// OnCancelReadWrite
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	CancelRequest(&pdx->dqReadWrite, Irp);
	}							// OnCancelReadWrite

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated)
	{							// StartDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status;

	// Identify the I/O resources we're supposed to use.

	PHYSICAL_ADDRESS portbase;
	BOOLEAN gotport = FALSE;
	
	if (!translated)
		return STATUS_DEVICE_CONFIGURATION_ERROR;		// no resources assigned??

	PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = translated->PartialDescriptors;
	ULONG nres = translated->Count;
	for (ULONG i = 0; i < nres; ++i, ++resource)
		{						// for each resource
		switch (resource->Type)
			{					// switch on resource type

		case CmResourceTypePort:
			portbase = resource->u.Port.Start;
			pdx->nports = resource->u.Port.Length;
			pdx->mappedport = (resource->Flags & CM_RESOURCE_PORT_IO) == 0;
			KdPrint(("Port base = %X\n", (PUCHAR) portbase.QuadPart));
			gotport = TRUE;
			break;

		default:
			KdPrint((DRIVERNAME " - Unexpected I/O resource type %d\n", resource->Type));
			break;
			}					// switch on resource type
		}						// for each resource

	// Verify that we got all the resources we were expecting

	if (!(TRUE
		&& gotport
		))
		{
		KdPrint((DRIVERNAME " - Didn't get expected I/O resources\n"));
		return STATUS_DEVICE_CONFIGURATION_ERROR;
		}

	if (pdx->mappedport)
		{						// map port address for RISC platform
		pdx->portbase = (PUCHAR) MmMapIoSpace(portbase, pdx->nports, MmNonCached);
		if (!pdx->portbase)
			{
			KdPrint((DRIVERNAME " - Unable to map port range %I64X, length %X\n", portbase, pdx->nports));
			return STATUS_INSUFFICIENT_RESOURCES;
			}
		}						// map port address for RISC platform
	else
		pdx->portbase = (PUCHAR) portbase.QuadPart;

	return STATUS_SUCCESS;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////
// TODO If you use separate queues for reads and writes, you will need separate
// StartIo routines too.

#pragma LOCKEDCODE

VOID StartIo(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// StartIo
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status;

	// TODO Initiate read request. Be sure to set the "busy" flag before starting
	// an operation that might generate an interrupt.

	}							// StartIo

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN oktouch /* = FALSE */)
	{							// StopDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	if (pdx->portbase && pdx->mappedport)
		MmUnmapIoSpace(pdx->portbase, pdx->nports);
	pdx->portbase = NULL;
	}							// StopDevice
