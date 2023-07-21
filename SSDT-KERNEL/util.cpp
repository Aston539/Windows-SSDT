#include "includes.h"

/*
	Function for finding the appropriate SYSTEM_MODULE_INFORMATION structure
	for a given loaded system module in memory by name
*/
NTSTATUS util::sys::GetModule( _In_ LPCSTR name, _Out_ SYSTEM_MODULE_INFORMATION* system_module )
{
	ULONG modules_size	= { };
	NTSTATUS status		= STATUS_SUCCESS;

/*
	We use this call to ZwQuerySystemInformation to get the amount of
	memory we need to allocate to hold all current loaded system modules
	this is returned via the ReturnLength parameter.
*/
	status = ZwQuerySystemInformation( SystemModuleInformation, NULL, NULL, &modules_size );

/*
	we dont check if status succeeded as this call will always return an
	invalid status as we dont give it a buffer to write into but if our
	returned allocation length was 0 then something has gone wrong and the
	return value of the function may indicate what went wrong.
*/
	if ( !modules_size )
	{
		return status;
	}

/*
	Allocate memory to hold a list of all system modules currently loaded
*/
	auto modules = ( PSYSTEM_MODULES )( ExAllocatePool( NonPagedPool, modules_size ) );

	if ( !modules )
	{
		return STATUS_NO_MEMORY;
	}

/*
	 Fill our allocated memory with the list of currently loaded modules
	 using the ZwQuerySystemInformation API
*/
	status = ZwQuerySystemInformation( SystemModuleInformation, modules, modules_size, NULL );

/*
	Make sure that we our call succeeded
*/
	if ( !NT_SUCCESS( status ) )
	{
/*
		Make sure to free our memory
*/
		ExFreePool(modules);

		return status;
	}

	for ( unsigned int i = 0; i < modules->NumberOfModules; i++ )
	{
/*
		Get the current module at our index
*/
		const auto current_module = modules->Modules[ i ];

/*
		Make sure it is a valid module
*/
		if ( !current_module.ImageBase || !current_module.FullPathName )
		{
			continue;
		}

/*
		Get the base name of the module by adding the offset
		from its path to its base name
*/
		const auto module_name = ( LPCSTR )( current_module.FullPathName + current_module.OffsetToFileName );

/*
		Make sure the name we recieved is valid if not could
		cause a bsod on _stricmp call
*/
		if ( !module_name )
		{
			continue;
		}

/*
		We use case insensitive string compare so the caller
		doesent have to worry about getting the cases wrong
*/
		if ( _stricmp( module_name, name ) == 0 )
		{
/*
			Free are allocated memory
*/
			ExFreePool( modules );

/*
			Make sure the caller passed a valid address to change if not then
			we can just return that the module is loaded as the caller may not
			want the module and just want to know that its loaded in memory
*/
			if ( system_module )
			{
				*system_module = current_module;
			}

			return STATUS_SUCCESS;
		}
	}

/*
	Make sure to free our memory
*/
	ExFreePool(modules);

/*
	This would mean iether the name was wrong passed
	in or the module isnt loaded
*/
	return STATUS_INVALID_PARAMETER;
}

/*
	Function for searching through a given segment of memory byte by byte to find a given section
*/
UINT64 util::pattern::Search( _In_ const char* pattern, _In_ const char* mask, _In_ UINT64 from, _In_ UINT64 to, _In_opt_ ULONG offset )
{
/*
	Make sure parameters are valid
*/
	if ( !pattern || !mask || !from || !to )
	{
		return 0;
	}

/*
	Loop starting from beginning of segment to end having our 
	iterator = to the current address we are at
*/
	for ( auto curr = from; curr < ( from + to ); curr++ )
	{
/*
		Make sure the pte is valid for the current memory address
		as if it wasnt this would cause a page fault which in kernel
		( NonPageable Memory ) cannot happen and would cause a bsod
*/
		if ( MmIsAddressValid( ( PVOID )curr ) == FALSE )
		{
			continue;
		}

		auto is_found = false;

		/*
		
		*/
		for ( int i = 0; i < strlen( mask ); i++ )
		{
/*
			If the byte is at an invalid PTE then we want to say we didnt find it
*/
			if ( MmIsAddressValid( ( PVOID )( curr + i ) ) == FALSE )
			{
				is_found = false;

				break;
			}

/*
			Make sure we arent going to cross our boundary
*/
			if ( ( curr + i ) > ( from + to ) )
			{
				is_found = false;

				break;
			}

/*
			Read a byte from the current address + our mask index to compare
			with our pattern
*/
			auto byte = *( BYTE* )( curr + i );

/*
			Check wehther if our current byte doesent match our pattern and we arent
			at a wildcard
*/
			if ( mask[ i ] != '?' && byte != pattern[ i ] )
			{
				is_found = false;

				break;
			}
			
/*
			if every byte matched we found our pattern
*/
			is_found = true;
		}

		if ( is_found )
		{
			return curr + offset;
		}
	}

	return 0;
}

/*
	Resloves a relative address between an instructions address
	and its relative offset from a 64 bit variable
*/
UINT64 util::pattern::ResolveRelative( _In_ UINT64 instruction, _In_ ULONG offset, _In_ ULONG Instruction_size )
{
	if ( !instruction || !MmIsAddressValid( ( PVOID )( instruction + offset ) ) )
	{
		return 0;
	}

	auto rva = *( LONG* )( instruction + offset );

	return ( instruction + Instruction_size + rva );
}