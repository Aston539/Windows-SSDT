#pragma once

#pragma warning (disable : 6101)
#pragma warning (disable : 4996)

#ifndef printf
#define printf(...) DbgPrintEx(0, 0, __VA_ARGS__)
#endif

typedef unsigned char BYTE;
typedef BYTE* PBYTE;

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemBasicInformation,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,
	SystemModuleInformation = 0x0B

} SYSTEM_INFORMATION_CLASS, * PSYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_MODULE_INFORMATION
{
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR  FullPathName[256];

} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

typedef struct _SYSTEM_MODULES
{
	ULONG NumberOfModules;
	SYSTEM_MODULE_INFORMATION Modules[1];

} SYSTEM_MODULES, * PSYSTEM_MODULES;

typedef struct tagSYSTEM_SERVICE_TABLE
{
	PLONG		ServiceTable;
	PULONG_PTR	CounterTable;
	ULONG_PTR	ServiceLimit;
	PBYTE		ArgumentTable;

} SYSTEM_SERVICE_TABLE;

typedef struct tagSERVICE_DESCRIPTOR_TABLE
{
	SYSTEM_SERVICE_TABLE sst1;
	SYSTEM_SERVICE_TABLE sst2;
	SYSTEM_SERVICE_TABLE sst3;
	SYSTEM_SERVICE_TABLE sst4;

} SERVICE_DESCRIPTOR_TABLE;

extern "C" NTSTATUS ZwQuerySystemInformation(
	SYSTEM_INFORMATION_CLASS InfoClass,
	PVOID Buffer,
	ULONG Length,
	PULONG ReturnLength
);

extern "C" NTSTATUS NTAPI IoCreateDriver(
	PUNICODE_STRING Uni,
	PDRIVER_INITIALIZE DriverEntry
);