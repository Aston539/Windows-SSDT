#pragma once

namespace util
{
	namespace sys
	{
		NTSTATUS GetModule(_In_ LPCSTR name, _Out_ SYSTEM_MODULE_INFORMATION* system_module);
	}
	
	namespace pattern
	{
		UINT64 Search(_In_ const char* pattern, _In_ const char* mask, _In_ UINT64 from, _In_ UINT64 to, _In_opt_ ULONG offset = 0);
		UINT64 ResolveRelative( _In_ UINT64 instruction, _In_ ULONG Offset, _In_ ULONG InstructionSize);
	}
}