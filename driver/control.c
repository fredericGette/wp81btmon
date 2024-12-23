// Legacy style driver
// Send IOCTL to the Bluetooh filter.

#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

VOID printBufferContent(PVOID buffer, size_t bufSize)
{
	CHAR hexString[256];
	CHAR chrString[256];
	CHAR tempString[8];
	size_t length;
	RtlZeroMemory(hexString, 256);
	RtlZeroMemory(chrString, 256);
	RtlZeroMemory(tempString, 8);
	unsigned char *p = (unsigned char*)buffer;
	unsigned int i = 0;
	BOOLEAN multiLine = FALSE;
	for(; i<bufSize && i < 608; i++)
	{
		RtlStringCbPrintfA(tempString, 8, "%02X ", p[i]);
		RtlStringCbCatA(hexString, 256, tempString);

		RtlStringCbPrintfA(tempString, 8, "%c", p[i]>31 && p[i]<127 ? p[i] : '.' );
		RtlStringCbCatA(chrString, 256, tempString);

		if ((i+1)%38 == 0)
		{
			DbgPrint("Control!%s%s", hexString, chrString);
			RtlZeroMemory(hexString, 256);
			RtlZeroMemory(chrString, 256);
			multiLine = TRUE;
		}
	}
	RtlStringCbLengthA(hexString,256,&length);
	if (length != 0)
	{
		CHAR padding[256];
		RtlZeroMemory(padding, 256);
		if (multiLine)
		{
			RtlStringCbPrintfA(padding, 256, "%*s", 3*(38-(i%38)),"");
		}

		DbgPrint("Control!%s%s%s", hexString, padding, chrString);
	}

	if (i == 608)
	{
		DbgPrint("Control!...");
	}	
}

void DriverUnload(PDRIVER_OBJECT DriverObject) {
	
	DbgPrint("Control!DriverUnload");
	
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\wp81controldevice");
	// delete symbolic link
	IoDeleteSymbolicLink(&symLink);
	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS DeviceCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	
	DbgPrint("Control!DeviceCreate");
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return STATUS_SUCCESS;
}

NTSTATUS DeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	
	DbgPrint("Control!DeviceClose");
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return STATUS_SUCCESS;
}

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS SendAnIoctl(PDEVICE_OBJECT TargetDevice, 
					ULONG IoControlCode, 
					PVOID pInputBuffer, 
					size_t InputBufferLength, 
					PVOID pOutputBuffer, 
					size_t OutputBufferLength,
					ULONG_PTR* information)
{
    NTSTATUS        status = STATUS_SUCCESS;
    KEVENT          event;
    IO_STATUS_BLOCK iosb;
    PIRP            irp;
	PVOID 			pInputBuffer2;
	PVOID			pOutputBuffer2;
    
	DbgPrint("Control!SendAnIoctl TargetDevice=0x%p IoControlCode=0x%X pInputBuffer=0x%p InputBufferLength=0x%p pOutputBuffer=0x%p OutputBufferLength=0x%X",TargetDevice, IoControlCode, pInputBuffer, InputBufferLength, pOutputBuffer, OutputBufferLength);

	// Copy input and output buffer to "non paged" memory buffer to avoid error 0xC000000D with the Bluetooth driver stack.
	pInputBuffer2 = ExAllocatePoolWithTag(NonPagedPoolNx, InputBufferLength, 'wp81');	
	pOutputBuffer2 = ExAllocatePoolWithTag(NonPagedPoolNx, OutputBufferLength, 'wp81');	

	if (InputBufferLength > 0) {
		DbgPrint("Control! InputBuffer:");
		printBufferContent(pInputBuffer, InputBufferLength);
		
		RtlCopyMemory(pInputBuffer2, pInputBuffer, InputBufferLength);
	}

	KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        TargetDevice,
                                        pInputBuffer2,
                                        InputBufferLength,
                                        pOutputBuffer2,
                                        OutputBufferLength,
                                        FALSE,
                                        &event,
                                        &iosb);
    if (irp == NULL) {
        goto Exit;
    }
 
    status = IoCallDriver(TargetDevice, irp);

	// Wait for the completion of the request
    if (status == STATUS_PENDING) {
		DbgPrint("Control! Wait STATUS_PENDING");

        KeWaitForSingleObject(&event, 
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = iosb.Status;
		*information = iosb.Information;
    }

	if (*information > 0) {
		DbgPrint("Control! status=0x%X information=%u OutputBuffer2:", status, *information);
		printBufferContent(pOutputBuffer2, *information);
		
		RtlCopyMemory(pOutputBuffer, pOutputBuffer2, *information);
	}

Exit:
	ExFreePoolWithTag(pInputBuffer2, 'wp81');
	ExFreePoolWithTag(pOutputBuffer2, 'wp81');
	DbgPrint("Control!End SendAnIoctl");
    return status;
}

/**
* Query the registry to find the memory address of the filter object.
*/
NTSTATUS queryFilterDeviceObject(PDEVICE_OBJECT *pFilterDeviceObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE hRegister;
	ULONG ulSize;
	PKEY_VALUE_PARTIAL_INFORMATION info;
	ULONG filterDeviceObjectAddr;
	OBJECT_ATTRIBUTES objectAttributes;
	UNICODE_STRING usKeyName;
	UNICODE_STRING usValueName;
	RtlInitUnicodeString(&usKeyName, L"\\REGISTRY\\MACHINE\\System\\CurrentControlSet\\Enum\\SystemBusQc\\SMD_BT\\4&315a27b&0&4097\\Device Parameters");
	RtlInitUnicodeString(&usValueName, L"wp81DeviceObjectPointer");
	InitializeObjectAttributes(&objectAttributes,
	                           &usKeyName,
	                           OBJ_CASE_INSENSITIVE,
	                           NULL,
	                           NULL );
	status = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Control!ZwOpenKey failed 0x%x", status);
	}
	else {
		status = ZwQueryValueKey(hRegister,
	                           &usValueName,
	                           KeyValuePartialInformation ,
	                           NULL,
	                           0,
	                           &ulSize);
		if (status==STATUS_OBJECT_NAME_NOT_FOUND || ulSize==0) {
			DbgPrint("Control!First ZwQueryValueKey failed 0x%x", status);
		}
		else {
			DbgPrint("Control!ulSize=0x%X", ulSize);
			info = ExAllocatePoolWithTag(PagedPool, ulSize, 'wp81');			
			status = ZwQueryValueKey(hRegister,
	                           &usValueName,
	                           KeyValuePartialInformation ,
	                           info,
	                           ulSize,
	                           &ulSize);
			if (!NT_SUCCESS(status)) {
				DbgPrint("Control!Second ZwQueryValueKey failed 0x%x", status);
			}
			else
			{
				DbgPrint("Control!info->Type=0x%X info->DataLength=0x%X", info->Type, info->DataLength);

				RtlMoveMemory(&filterDeviceObjectAddr, info->Data, info->DataLength);
				DbgPrint("Control!filterDeviceObjectAddr=0x%X", filterDeviceObjectAddr);
				*pFilterDeviceObject = (PDEVICE_OBJECT)filterDeviceObjectAddr;
			}
			ExFreePoolWithTag(info, 'wp81');
		}
		ZwClose(hRegister);
	}

	return status;
}

NTSTATUS DriverDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT pFilterDeviceObject;
	ULONG IoControlCode;
	size_t InputBufferLength;
	size_t OutputBufferLength;
	PVOID pOutputBuffer;
	PVOID pInputBuffer;
	ULONG_PTR information = 0;

    // We must manage the input/output buffer by ourself. Otherwise the Bluetooh driver stack will reject our IOCtls (ERROR_INVALID_USER_BUFFER).
	// https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/buffer-descriptions-for-i-o-control-codes#method_neither
	IoControlCode = Irp->Tail.Overlay.CurrentStackLocation->Parameters.DeviceIoControl.IoControlCode;
	InputBufferLength = Irp->Tail.Overlay.CurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	OutputBufferLength = Irp->Tail.Overlay.CurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength;	

	DbgPrint("Control!DriverDispatch IoControlCode=0x%X InputBufferLength=0x%X OutputBufferLength=0x%X",IoControlCode,InputBufferLength,OutputBufferLength);

	pInputBuffer = Irp->Tail.Overlay.CurrentStackLocation->Parameters.DeviceIoControl.Type3InputBuffer;
	pOutputBuffer = Irp->UserBuffer;
	DbgPrint("Control!DriverDispatch pInputBuffer=0x%p pOutputBuffer=0x%p", pInputBuffer, pOutputBuffer);

	status = queryFilterDeviceObject(&pFilterDeviceObject);
	if (NT_SUCCESS(status)) {
		DbgPrint("Control!pFilterDeviceObject=0x%p", pFilterDeviceObject);
		DbgPrint("Control!FDO Type=%d (3=Device), Size=%d, Driver=0x%p",pFilterDeviceObject->Type, pFilterDeviceObject->Size, pFilterDeviceObject->DriverObject);
		status = SendAnIoctl(pFilterDeviceObject, IoControlCode, pInputBuffer, InputBufferLength, pOutputBuffer, OutputBufferLength, &information);	
	}
	else {
		DbgPrint("Control!queryFilterDeviceObject failed 0x%x", status);
	}

	return CompleteIrp(Irp, status, information);
}

NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	
	UNREFERENCED_PARAMETER(RegistryPath);
	
	DbgPrint("Control!DriverEntry");
	
	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DeviceCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DeviceClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDispatch;
	
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\wp81controldevice");
	
	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(
		DriverObject, // our driver object
		0, // no need for extra bytes
		&devName, // the device name
		FILE_DEVICE_UNKNOWN, // device type
		0, // characteristics flags
		FALSE, // not exclusive
		&DeviceObject); // the resulting pointer
	if (!NT_SUCCESS(status)) {
		DbgPrint("Control!Failed to create device object (0x%x)", status);
		return status;
	}
	
	NT_ASSERT(DeviceObject);
	
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\wp81controldevice");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Control!Failed to create symbolic link (0x%x)", status);
		IoDeleteDevice(DeviceObject);
		return status;
	}
	NT_ASSERT(NT_SUCCESS(status));
	
	return STATUS_SUCCESS;
}