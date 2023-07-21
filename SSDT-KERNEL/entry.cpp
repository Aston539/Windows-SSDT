#include "includes.h"

/*
	Routine that is automatically called when the driver
	has been requested to unload from memory the driver must
	support this to have the ability to be unloaded
*/
VOID UnloadRoutine( _In_ PDRIVER_OBJECT DriverObject )
{
	printf("Driver Unload Called!\n");

	UNICODE_STRING symbolic_name = RTL_CONSTANT_STRING( L"\\??\\SSDT-KERNEL-SYMBOLIC" );

	IoDeleteSymbolicLink( &symbolic_name );
	IoDeleteDevice( DriverObject->DeviceObject );
}

/*
	Create and Close routine for the drivers major functions
	this function will be called when a handle is created to
	the driver and when the handle is closed to the driver

	all this routine does is send request success
*/
NTSTATUS CreateCloseRoutine( _In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp )
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = NULL;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/*
	DeviceControl major function routine this is the routine that
	we will use to communicate with out usermode component it is
	called by the usermode DeviceIoControl syscall which will make its
	way to our function with a buffer specified by our usermode which
	will allow us to recieve and send data back and forth from kernel to usermode
*/
NTSTATUS DeviceControlRoutine( _In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp )
{
	UNREFERENCED_PARAMETER(DeviceObject);

	auto status = STATUS_SUCCESS;

/*
	Get our current IO_STACK_LOCATION structure this is used to
	retrieve data passed from usermode
*/
	const auto stack = IoGetCurrentIrpStackLocation(Irp);
	const auto req	 = stack->Parameters;

	switch ( req.DeviceIoControl.IoControlCode )
	{
		case IOCTL_SSDT_GET_ROUTINE: {

/*
			Make sure the buffers sent from usermode matches
			what the buffer sizes are supposed to be
*/
			if ( req.DeviceIoControl.InputBufferLength  != sizeof( ssdt_routine_t ) ||
				 req.DeviceIoControl.OutputBufferLength != sizeof( ssdt_routine_t ) )
			{
				status = STATUS_BUFFER_TOO_SMALL;

				break;
			}

			auto usermode_buffer = ( ssdt_routine_t* )( req.DeviceIoControl.Type3InputBuffer );

			if ( !usermode_buffer )
			{
				status = STATUS_INVALID_PARAMETER;

				break;
			}

			auto syscall_routine = ssdt::GetSyscallAddress( usermode_buffer->syscall_index );

			usermode_buffer->syscall_address	= syscall_routine;
			usermode_buffer->gui_syscall		= ssdt::IsGuiSyscall( usermode_buffer->syscall_index );

		} break;

		case IOCTL_SSDT_GET: {

			if ( req.DeviceIoControl.InputBufferLength  != sizeof( ssdt_info_t ) ||
				 req.DeviceIoControl.OutputBufferLength != sizeof( ssdt_info_t ) )
			{
				status = STATUS_BUFFER_TOO_SMALL;

				break;
			}

			auto usermode_buffer = ( ssdt_info_t* )( req.DeviceIoControl.Type3InputBuffer );

			if (!usermode_buffer)
			{
				status = STATUS_INVALID_PARAMETER;

				break;
			}

			SYSTEM_MODULE_INFORMATION ntoskrnl = { };

			status = util::sys::GetModule( "ntoskrnl.exe", &ntoskrnl );

			if ( !NT_SUCCESS( status ) )
			{
				break;
			}

			auto ssdt_native	= ssdt::GetKeServiceDescriptorTable( ntoskrnl ).sst1;
			auto ssdt_gui		= ssdt::GetKeServiceDescriptorTableShadow( ntoskrnl ).sst1;

			usermode_buffer->number_of_native	= ssdt_native.ServiceLimit;
			usermode_buffer->number_of_gui		= ssdt_gui.ServiceLimit;

		} break;

		default: {

			status = STATUS_INVALID_DEVICE_REQUEST;

		} break;
	}

	Irp->IoStatus.Status		= status;
	Irp->IoStatus.Information	= NULL;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

/*
	Our real driver entry point we have this to support
	manual mapping of this driver although it should not
	be used this way
*/
NTSTATUS EntryPoint( _In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath )
{
/*
	We are not going to use the RegistryPath parameter so to avoid
	warnings we are going to use the UNREFERENCED_PARAMETER macro to
	reference it
*/
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS status = STATUS_SUCCESS;

	printf("Hello from DriverEntry!\n");

/*
	Set a driver unload routine so our driver can actually
	unload from memory
*/
	DriverObject->DriverUnload = UnloadRoutine;

/*
	Register a new device object for use with this driver
*/
	UNICODE_STRING device_name		= RTL_CONSTANT_STRING( L"\\Device\\SSDT-KERNEL-DEVICE" );
	PDEVICE_OBJECT device_object	= nullptr;

	status = IoCreateDevice( DriverObject, NULL, &device_name, FILE_DEVICE_UNKNOWN, NULL, FALSE, &device_object );

	if ( !NT_SUCCESS( status ) )
	{
		printf("Failed to create device object!\n");

		return status;
	}

	DriverObject->DeviceObject		= device_object;

/*
	Register a symbolic link so that a usermode counterpart
	has the ability to open a handle to the driver and communicate
*/
	UNICODE_STRING symbolic_name	= RTL_CONSTANT_STRING( L"\\??\\SSDT-KERNEL-SYMBOLIC" );

	status = IoCreateSymbolicLink( &symbolic_name, &device_name );

	if ( !NT_SUCCESS( status ) )
	{
		printf("Failed to create symbolic link!\n");

		IoDeleteDevice( device_object );

		return status;
	}

/*
	Setup the necessary major function routines so our driver
	can successfully communicate with a usermode counterpart
	with the help of handles.

	We set our create and close routines to the same routine
	because for this drivers needs they arent required to be 
	more then a simple request success.
*/
	DriverObject->MajorFunction[ IRP_MJ_CREATE		   ]	= CreateCloseRoutine;   /* called on handle creation to driver				*/
	DriverObject->MajorFunction[ IRP_MJ_CLOSE		   ]	= CreateCloseRoutine;   /* called on handle deletion						*/
	DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ]	= DeviceControlRoutine; /* called on DeviceIoControl syscall from usermode	*/

	return status;
}

/*
	DriverEntry this is an exported routine which should be
	called by the PnpCallDriverEntry routine after windows is
	done loading it
*/
extern "C" 
NTSTATUS DriverEntry( _In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath )
{
/*
	If our driver object doesent have a valid address
	this will most likely mean that the driver was mapped
	into memory instead of being loaded properly to account
	for this we will call IoCreateDriver which will continue the
	routine like windows normally would.
*/
	if ( !DriverObject )
	{
		UNICODE_STRING driver_name = RTL_CONSTANT_STRING( L"SSDT-KERNEL" );

		return IoCreateDriver( &driver_name, EntryPoint );
	}

/*
	If the driver object was not null these probably means that the driver
	was loaded normally by the windows loader and we can just pass the parameters
	windows give us to our actual entry point function
*/
	return EntryPoint( DriverObject, RegistryPath );
}