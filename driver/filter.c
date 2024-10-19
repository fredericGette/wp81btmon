// WDF filter
// Blocks IOCTL sent by the official Bluetooth stack.
// And process IOCTL sent by the control driver.

#include <ntifs.h>
#include <wdf.h>
#include <ntstrsafe.h>

#define ABSOLUTE(wait) (wait)
#define RELATIVE(wait) (-(wait))

#define NANOSECONDS(nanos) \
(((signed __int64)(nanos)) / 100L)

#define MICROSECONDS(micros) \
(((signed __int64)(micros)) * NANOSECONDS(1000L))

#define MILLISECONDS(milli) \
(((signed __int64)(milli)) * MICROSECONDS(1000L))

typedef struct _DEVICEFILTER_CONTEXT
{
	CHAR Name[32];
} DEVICEFILTER_CONTEXT, *PDEVICEFILTER_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICEFILTER_CONTEXT, GetFilterContext);

typedef struct _COMPLETION_CONTEXT
{
	CHAR Name[32];
    ULONG uid;
} COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;

// When set, blocks the IOTCL sent by the official Bluetooth stack.
BOOLEAN StopBTHX = FALSE;

VOID printBufferContent(PVOID buffer, size_t bufSize, CHAR* Name, ULONG uid)
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
			DbgPrint("HCI!%s!%08X!%s%s", Name, uid, hexString, chrString);
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

		DbgPrint("HCI!%s!%08X!%s%s%s",Name, uid, hexString, padding, chrString);
	}

	if (i == 608)
	{
		DbgPrint("HCI!%s!%08X!...",Name, uid);
	}	
}

VOID
FilterRequestCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
   )
{
    UNREFERENCED_PARAMETER(Target);
	
	NTSTATUS status;
	PCOMPLETION_CONTEXT completionContext = Context;
	size_t OutputBufferLength;
	PIRP irp;
	UCHAR MajorFunction;
	PVOID  buffer = NULL;
	size_t  bufSize = 0;
	ULONG IoControlCode;

	irp = WdfRequestWdmGetIrp(Request);
	MajorFunction = irp->Tail.Overlay.CurrentStackLocation->MajorFunction;
		
	if (MajorFunction == IRP_MJ_DEVICE_CONTROL || MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)
	{
		IoControlCode = irp->Tail.Overlay.CurrentStackLocation->Parameters.DeviceIoControl.IoControlCode;

		// Looks like this is the real OutputBufferLength
		OutputBufferLength = CompletionParams->IoStatus.Information;

		DbgPrint("HCI!%s!%08X!Complete IoControlCode=0x%X OutputBufferLength=%u Status=0x%X", completionContext->Name, completionContext->uid, IoControlCode, OutputBufferLength, CompletionParams->IoStatus.Status);

		if (OutputBufferLength > 0)
		{
			status = WdfRequestRetrieveOutputBuffer(Request, OutputBufferLength, &buffer, &bufSize );
			if (!NT_SUCCESS(status)) {
				DbgPrint("HCI!%s!%08X!WdfRequestRetrieveOutputBuffer failed: 0x%x", completionContext->Name, completionContext->uid, status);
				goto exit;
			}
			printBufferContent(buffer, OutputBufferLength, completionContext->Name, completionContext->uid);
		}
	}
	
exit:
    ExFreePoolWithTag(completionContext, 'wp81');
    WdfRequestComplete(Request, CompletionParams->IoStatus.Status);

    return;
}


VOID
FilterForwardRequestWithCompletionRoutine(
    IN WDFREQUEST Request,
    IN WDFIOTARGET Target,
	IN PDEVICEFILTER_CONTEXT deviceContext,
    ULONG uid
    )
{
    BOOLEAN ret;
    NTSTATUS status;
    PCOMPLETION_CONTEXT completionContext;

    //
    // The following function essentially copies the content of
    // current stack location of the underlying IRP to the next one. 
    //
    WdfRequestFormatRequestUsingCurrentType(Request);

	// For logging purpose:
	// store the logging prefix of the device in the context passed to the completion routine.
    completionContext = ExAllocatePoolWithTag(PagedPool, sizeof(COMPLETION_CONTEXT), 'wp81');
    RtlCopyMemory(completionContext->Name, deviceContext->Name, 32);
    completionContext->uid = uid;

    WdfRequestSetCompletionRoutine(Request,
                                FilterRequestCompletionRoutine,
                                completionContext);

    // LARGE_INTEGER timeout;
	// timeout.QuadPart = RELATIVE(MILLISECONDS(500));
	// KeDelayExecutionThread(KernelMode, FALSE, &timeout);

    ret = WdfRequestSend(Request,
                         Target,
                         WDF_NO_SEND_OPTIONS);

    if (ret == FALSE) {
        status = WdfRequestGetStatus (Request);
        DbgPrint("HCI!%s!%08X!WdfRequestSend failed: 0x%x",deviceContext->Name, uid, status);
        WdfRequestComplete(Request, status);
    }

    return;
}

VOID
FilterEvtIoDeviceControl(
    IN WDFQUEUE      Queue,
    IN WDFREQUEST    Request,
    IN size_t        OutputBufferLength,
    IN size_t        InputBufferLength,
    IN ULONG         IoControlCode
    )
{
    WDFDEVICE device;
    NTSTATUS status;
    PVOID  buffer = NULL;
	size_t  bufSize = 0;
    ULONG seed = 1;
    ULONG uid;
    PIRP irp;
    LARGE_INTEGER Interval;

	// For logging purpose:
	// generate and UID to identify the current request
    uid = RtlRandomEx(&seed);

    //DbgPrint("HCI!Begin FilterEvtIoDeviceControl");

    device = WdfIoQueueGetDevice(Queue);
	PDEVICEFILTER_CONTEXT deviceContext = GetFilterContext(device);
			
	DbgPrint("HCI!%s!%08X!Receive IoControlCode=0x%X InputBufferLength=%u OutputBufferLength=%u StopBTHX=%s",deviceContext->Name, uid, IoControlCode, InputBufferLength, OutputBufferLength, StopBTHX?"true":"false");
	
    if (StopBTHX == TRUE && (IoControlCode == 0x41040F || IoControlCode == 0x410413)) {
		// Intercept the IOCTL_BTHX_WRITE_HCI (0x41040F) and the IOCTL_BTHX_READ_HCI (0x410413) sent by the official Bluetooth stack
		// And cancel them.
		// (wait a second to simulate a real processing of the IOCTL and avoid a fast retry by the official Bluetooth stack)
	
        Interval.QuadPart = RELATIVE(MILLISECONDS(1000)); 
        status = KeDelayExecutionThread(KernelMode, FALSE, &Interval);
        if (!NT_SUCCESS(status)) {
            DbgPrint("HCI!%s!%08X!KeDelayExecutionThread failed: 0x%x", deviceContext->Name, uid, status);
            WdfRequestComplete(Request, status);
            return;
        }        
        DbgPrint("HCI!%s!%08X!Cancel IoControlCode 0x%X", deviceContext->Name, uid, IoControlCode);        
        WdfRequestComplete(Request, STATUS_CANCELLED);
        return;
    }
	

    if (IoControlCode == 0x80002000) {
		// Convert the IOCTL sent by the control driver
		// 0x80002000 -> IOCTL_BTHX_WRITE_HCI (0x41040F)
        DbgPrint("HCI!%s!%08X!Update IoControlCode 0x80002003 -> 0x41040F", deviceContext->Name, uid);
        irp = WdfRequestWdmGetIrp(Request);
        irp->Tail.Overlay.CurrentStackLocation->Parameters.DeviceIoControl.IoControlCode = 0x41040F;
    }

    if (IoControlCode == 0x80002004) {
		// Convert the IOCTL sent by the control driver
		// 0x80002003 -> IOCTL_BTHX_READ_HCI (0x410413)
        DbgPrint("HCI!%s!%08X!Update IoControlCode 0x80002007 -> 0x410413", deviceContext->Name, uid);
        irp = WdfRequestWdmGetIrp(Request);
        irp->Tail.Overlay.CurrentStackLocation->Parameters.DeviceIoControl.IoControlCode = 0x410413;
    }

    if (InputBufferLength > 0) {
        status = WdfRequestRetrieveInputBuffer(Request, InputBufferLength, &buffer, &bufSize);
        if (!NT_SUCCESS(status)) {
            DbgPrint("HCI!%s!%08X!WdfRequestRetrieveInputBuffer failed: 0x%x", deviceContext->Name, uid, status);
            WdfRequestComplete(Request, status);
            return;
        }
        printBufferContent(buffer, bufSize, deviceContext->Name, uid);
        
        if (IoControlCode == 0x80002008) {
			// IOCTL sent by the control driver to block or passthrough the IOCTL sent by the official Bluetooh stack
			
            unsigned char *p = (unsigned char*)buffer;
            if (bufSize > 0 && p[0] == 1) {
                DbgPrint("HCI!%s!%08X!Stop IOCTL_BTHX messages.", deviceContext->Name, uid);
                StopBTHX = TRUE;
            } else if (bufSize > 0 && p[0] == 0) {
                DbgPrint("HCI!%s!%08X!Allow IOCTL_BTHX messages.", deviceContext->Name, uid);
                StopBTHX = FALSE;                
            }
            WdfRequestComplete(Request, STATUS_SUCCESS);
            return;
        }
    }

    FilterForwardRequestWithCompletionRoutine(Request, WdfDeviceGetIoTarget(device), deviceContext, uid);

	//DbgPrint("HCI!%s!End FilterEvtIoDeviceControl",deviceContext->Name);

    return;
}

void EvtDeviceCleanup(WDFOBJECT Object) 
{
    WDFDEVICE device = (WDFDEVICE) Object;
    NTSTATUS  status;
	
	DbgPrint("HCI!Begin EvtDeviceCleanup");

    // Clean the registry.
    WDFKEY hKey = NULL;
    UNICODE_STRING valueName;
    status = WdfDeviceOpenRegistryKey(device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      &hKey);
    if (NT_SUCCESS (status)) {
        RtlInitUnicodeString(&valueName, L"wp81DeviceObjectPointer");
        status = WdfRegistryRemoveValue(hKey, &valueName);
        if (!NT_SUCCESS(status)) {
            DbgPrint("HCI!WdfRegistryRemoveValue failed 0x%x", status);
        }
        WdfRegistryClose(hKey);
    }
    else {
        DbgPrint("HCI!WdfDeviceOpenRegistryKey failed 0x%x", status);
    }


	DbgPrint("HCI!End EvtDeviceCleanup");
}

NTSTATUS EvtDriverDeviceAdd(WDFDRIVER  Driver, PWDFDEVICE_INIT  DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);
	NTSTATUS                        status;
    WDFDEVICE                       device;    
    WDF_OBJECT_ATTRIBUTES           deviceAttributes;
	WDF_IO_QUEUE_CONFIG     		ioQueueConfig;
    
	DbgPrint("HCI!Begin EvtDriverDeviceAdd");

    //
    // Tell the framework that you are filter driver. Framework
    // takes care of inherting all the device flags & characterstics
    // from the lower device you are attaching to.
    //
    WdfFdoInitSetFilter(DeviceInit);

    //
    // Set device attributes
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICEFILTER_CONTEXT);
    deviceAttributes.EvtCleanupCallback = EvtDeviceCleanup;

	//
    // Create a framework device object.  This call will in turn create
    // a WDM deviceobject, attach to the lower stack and set the
    // appropriate flags and attributes.
    //
    status = WdfDeviceCreate(
        &DeviceInit,
        &deviceAttributes,
        &device
        );
    if (!NT_SUCCESS(status))
    {
        DbgPrint("HCI!WdfDeviceCreate failed with Status code 0x%x", status);
        goto exit;
    }
	
	PDRIVER_OBJECT pWdmDriver = WdfDriverWdmGetDriverObject(Driver);
	PDEVICE_OBJECT pWdmPDO = WdfDeviceWdmGetPhysicalDevice(device);
	PDEVICE_OBJECT pWdmFDO = WdfDeviceWdmGetDeviceObject(device);
	PDEVICE_OBJECT pWdmLowerDO = WdfDeviceWdmGetAttachedDevice(device);
	
	DbgPrint("HCI!Driver 0x%p, FDO 0x%p, PDO 0x%p, Lower 0x%p", pWdmDriver, pWdmFDO, pWdmPDO, pWdmLowerDO);
	
	DbgPrint("HCI!FDO Type=%d (3=Device), Size=%d, Driver=0x%p, NextDevice=0x%p, AttachedDevice=0x%p",pWdmFDO->Type, pWdmFDO->Size, pWdmFDO->DriverObject, pWdmFDO->NextDevice, pWdmFDO->AttachedDevice);
	DbgPrint("HCI!PDO Type=%d (3=Device), Size=%d, Driver=0x%p, NextDevice=0x%p, AttachedDevice=0x%p",pWdmPDO->Type, pWdmPDO->Size, pWdmPDO->DriverObject, pWdmPDO->NextDevice, pWdmPDO->AttachedDevice);
	DbgPrint("HCI!PDO2 Type=%d (3=Device), Size=%d, Driver=0x%p, NextDevice=0x%p, AttachedDevice=0x%p",pWdmPDO->NextDevice->Type, pWdmPDO->NextDevice->Size, pWdmPDO->NextDevice->DriverObject, pWdmPDO->NextDevice->NextDevice, pWdmPDO->NextDevice->AttachedDevice);
	DbgPrint("HCI!LowerDO Type=%d (3=Device), Size=%d, Driver=0x%p, NextDevice=0x%p, AttachedDevice=0x%p",pWdmLowerDO->Type, pWdmLowerDO->Size, pWdmLowerDO->DriverObject, pWdmLowerDO->NextDevice, pWdmLowerDO->AttachedDevice);
	
	PDRIVER_OBJECT pWdmDriver2 = pWdmFDO->DriverObject;
	DbgPrint("HCI!FDO Driver Type=%d (4=Driver), Device=0x%p, DriverName=%wZ, HardwareDatabase=%wZ",pWdmDriver2->Type, pWdmDriver2->DeviceObject, &(pWdmDriver2->DriverName), pWdmDriver2->HardwareDatabase);
	
	pWdmDriver2 = pWdmPDO->DriverObject;
	DbgPrint("HCI!PDO Driver Type=%d (4=Driver), Device=0x%p, DriverName=%wZ, HardwareDatabase=%wZ",pWdmDriver2->Type, pWdmDriver2->DeviceObject, &(pWdmDriver2->DriverName), pWdmDriver2->HardwareDatabase);

	pWdmDriver2 = pWdmLowerDO->DriverObject;
	DbgPrint("HCI!LowerDO Driver Type=%d (4=Driver), Device=0x%p, DriverName=%wZ, HardwareDatabase=%wZ",pWdmDriver2->Type, pWdmDriver2->DeviceObject, &(pWdmDriver2->DriverName), pWdmDriver2->HardwareDatabase);
	
				
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);	
	ioQueueConfig.EvtIoDeviceControl = FilterEvtIoDeviceControl;
	
	status = WdfIoQueueCreate(device,
                            &ioQueueConfig,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            WDF_NO_HANDLE // pointer to default queue
                            );
    if (!NT_SUCCESS(status)) {
        DbgPrint("HCI!WdfIoQueueCreate failed 0x%x", status);
        goto exit;
    }   
	
    // Create a prefix name for the logs of this device.
	PDEVICEFILTER_CONTEXT deviceContext = GetFilterContext(device);
	CHAR fullDriverName[32] = {0};
	RtlStringCbPrintfA(fullDriverName, 32-1, "%wZ", &(pWdmLowerDO->DriverObject->DriverName));
	CHAR *shortDriverName = fullDriverName;
	if (RtlCompareMemory(fullDriverName, "\\Driver\\", 8) == 8)
	{
		shortDriverName = fullDriverName + 8;
	}
	CHAR buffer[32];
	RtlZeroMemory(buffer, 32);
	RtlStringCbPrintfA(buffer, 32-1, "%p-%s", pWdmLowerDO->DriverObject->DeviceObject, shortDriverName);
	RtlCopyMemory(deviceContext->Name, buffer, 32);
	
    // Store this device object pointer in registry (for the control device).
    WDFKEY hKey = NULL;
    UNICODE_STRING valueName;
    status = WdfDeviceOpenRegistryKey(device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      &hKey);
    if (NT_SUCCESS (status)) {
        RtlInitUnicodeString(&valueName, L"wp81DeviceObjectPointer");
        status = WdfRegistryAssignULong (hKey,
                                  &valueName,
                                  (ULONG)pWdmFDO
                                );
        if (!NT_SUCCESS(status)) {
            DbgPrint("HCI!WdfRegistryAssignULong failed 0x%x", status);
        }
        WdfRegistryClose(hKey);
    }
    else {
        DbgPrint("HCI!WdfDeviceOpenRegistryKey failed 0x%x", status);
    }

			
exit:    
	DbgPrint("HCI!End EvtDriverDeviceAdd");
    return status;
}

void EvtDriverCleanup(WDFOBJECT DriverObject) 
{
    UNREFERENCED_PARAMETER(DriverObject);
	
	DbgPrint("HCI!Begin EvtDriverCleanup");
	DbgPrint("HCI!End EvtDriverCleanup");
}

// DriverEntry
NTSTATUS DriverEntry(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING  RegistryPath)
{
	DbgPrint("HCI!Begin DriverEntry");
	
    NTSTATUS status;
    WDFDRIVER driver;
    WDF_OBJECT_ATTRIBUTES attributes;
        
    WDF_DRIVER_CONFIG DriverConfig;
    WDF_DRIVER_CONFIG_INIT(
                           &DriverConfig,
                           EvtDriverDeviceAdd
                           );

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = EvtDriverCleanup;

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &attributes,
        &DriverConfig,
        &driver
        );

	DbgPrint("HCI!Driver registryPath= %S", RegistryPath->Buffer);

	DbgPrint("HCI!End DriverEntry");
    return status;
}