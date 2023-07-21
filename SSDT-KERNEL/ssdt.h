#pragma once

namespace ssdt
{
/*
	I am using pattern scanning to find the KeServiceDescriptorTable and
	KeServiceDescriptorTableShadow as it is more consistent then relative offsets
	and easier then following a exported function call to the global variables.

	The way i found these variables was via the KiSystemServiceRepeat location
	inside of the KiSystemCall64 function inside of ntoskrnl this can be identified
	with static analysis and the symbols given by windows.
*/

	inline const char* ssdt_pattern = "\x4C\x8D\x15\x00\x00\x00\x00\x4C\x8D\x1D\x00\x00\x00\x00\xF7";
	inline const char* ssdt_mask = "xxx????xxx????x";

	inline const char* ssdts_pattern = "\x4C\x8D\x1D\x00\x00\x00\x00\xF7";
	inline const char* ssdts_mask = "xxx????x";

	SERVICE_DESCRIPTOR_TABLE GetKeServiceDescriptorTable( _In_ SYSTEM_MODULE_INFORMATION ntoskrnl );
	SERVICE_DESCRIPTOR_TABLE GetKeServiceDescriptorTableShadow( _In_ SYSTEM_MODULE_INFORMATION ntoskrnl );

	UINT64 GetSyscallAddress( _In_ ULONG syscall_index );

	BOOLEAN IsGuiSyscall( _In_ ULONG syscall_index);
}