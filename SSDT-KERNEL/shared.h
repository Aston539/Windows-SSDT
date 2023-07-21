#pragma once

#define _CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define IOCTL_SSDT_GET_ROUTINE _CTL_CODE( 0x8000, 0x800, 3, 0 )
#define IOCTL_SSDT_GET         _CTL_CODE( 0x8000, 0x801, 3, 0 )

struct ssdt_routine_t
{
/*
    values provided by usermode
*/
    ULONG syscall_index;

/*
    values sent by kernelmode
*/
    UINT64 syscall_address;
    BOOLEAN gui_syscall;
};

struct ssdt_info_t
{
/*
    values sent by kernelmode
*/
    ULONG_PTR number_of_native;
    ULONG_PTR number_of_gui;
};