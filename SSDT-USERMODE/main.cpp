#include "includes.h"

/*
    All syscalls should relate to this pattern of bytes
    ?? being a wildcard so things like the syscall index
    or bytes that wont be the same for every one

    4C 8B D1 B8 ?? ?? ?? ?? F6 04 25 ?? ?? ?? ?? 01 75 03 0F 05 C3

    \x4C\x8B\xD1\xB8\x00\x00\x00\x00\xF6\x04\x25\x00\x00\x00\x00\x01\x75\x03\x0F\x05\xC3
    xxxx????xxx????xxxxxx
*/

/*
    Global variable for easily sending
    requests without routines having
    to be given the handle to our kernel
    device
*/
HANDLE g_hDevice = INVALID_HANDLE_VALUE;

/*
	Function for searching through a given segment of memory byte by byte to find a given section
*/
UINT64 Search(_In_ const char* pattern, _In_ const char* mask, _In_ UINT64 from, _In_ UINT64 to, _In_opt_ ULONG offset)
{
/*
	Make sure parameters are valid
*/
	if (!pattern || !mask || !from || !to)
	{
		return 0;
	}

/*
	Loop starting from beginning of segment to end having our
	iterator = to the current address we are at
*/
	for (auto curr = from; curr < (from + to); curr++)
	{
/*
		Make sure the address is valid could be done better
		by using VirtualQuery to get a MEMORY_BASIC_INFORMATION
		structure about this given region of memory so that we
		can check page protection for things like PAGE_GUARD
*/
		if ( !curr )
		{
			continue;
		}

		auto is_found = false;

		for (int i = 0; i < strlen(mask); i++)
		{
/*
			Make sure we arent going to cross our boundary
*/
			if ((curr + i) > (from + to))
			{
				is_found = false;

				break;
			}

/*
			Read a byte from the current address + our mask index to compare
			with our pattern
*/
			auto byte = *(BYTE*)(curr + i);

/*
			Check wehther if our current byte doesent match our pattern and we arent
			at a wildcard
*/
			if (mask[i] != '?' && byte != pattern[i])
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
    This function takes in the address of
    a routine that performs a system call
    with this address it then copies the 
    system call index and returns it for
    later use.
*/
ULONG GetSyscallIndex( PVOID routine )
{
	auto index_addr = Search(
		"\x4C\x8B\xD1\xB8\x00\x00\x00\x00\xF6\x04\x25\x00\x00\x00\x00\x01\x75\x03\x0F\x05\xC3",
		"xxxx????xxx????xxxxxx",
		( UINT_PTR )routine,
		( UINT_PTR )routine + 0x40,
		4 
	);

	if ( index_addr )
	{
		return *( ULONG* )( index_addr );
	}

	return 0;
}

/*
    Returns a std::pair including the number of native
    and gui syscalls in the format of { native, gui }
*/
std::pair<ULONG, ULONG> GetSystemServiceCount( )
{
    ssdt_info_t info_req = { };

/*
    Send request to our kernel component to retrieve
    the amount of system service routines registered
    for this version of windows.
*/
    if ( !DeviceIoControl( g_hDevice, IOCTL_SSDT_GET, &info_req, sizeof( ssdt_info_t ), &info_req, sizeof( ssdt_info_t ), NULL, NULL ) )
    {
        printf("device io control failed with: 0x%X\n", GetLastError());
    }

    return { info_req.number_of_native, info_req.number_of_gui };
}

/*
    Returns a given system service calls
    address inside of kernel
*/
ssdt_routine_t GetSystemServiceRoutine( ULONG index )
{
	ssdt_routine_t routine_req = { };

	routine_req.syscall_index = index;

	if ( !DeviceIoControl( g_hDevice, IOCTL_SSDT_GET_ROUTINE, &routine_req, sizeof( ssdt_routine_t ), &routine_req, sizeof( ssdt_routine_t ), NULL, NULL ) )
	{
		printf("device io control failed with: 0x%X\n", GetLastError());
	}

	return routine_req;
}

int main(int argv, char** argc)
{
    g_hDevice = CreateFileW( L"\\\\.\\SSDT-KERNEL-SYMBOLIC", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL );

    if (g_hDevice == INVALID_HANDLE_VALUE || !g_hDevice)
    {
        printf("failed to open device handle!\n");

        system("pause");

        return 1;
    }

	auto amt_routines = GetSystemServiceCount( );

	for ( UINT_PTR i = 0x0000; i < amt_routines.first; i++ )
	{
		auto routine = GetSystemServiceRoutine( i );

		printf("[0x%llX] 0x%llX - %i\n", i, routine.syscall_address, routine.gui_syscall);
	}
}