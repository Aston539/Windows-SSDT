#include "includes.h"

/*
    The GetKeServiceDescriptorTable and GetKeServiceDescriptorTableShadow routines
    could be made faster via only searching within the section they are inside which
    would be the .text section
*/
SERVICE_DESCRIPTOR_TABLE ssdt::GetKeServiceDescriptorTable( _In_ SYSTEM_MODULE_INFORMATION ntoskrnl )
{
    // auto ssdt_addr = util::pattern::ResolveRelative(
    //     util::pattern::Search(ssdt_pattern, ssdt_mask, ( UINT64 )ntoskrnl.ImageBase, ntoskrnl.ImageSize), 3, 7);

    auto ssdt_addr = (UINT64)ntoskrnl.ImageBase + 0x0E018C0;

    if ( !ssdt_addr )
    {
        return { };
    }

    return *( SERVICE_DESCRIPTOR_TABLE* )( ssdt_addr );
}

SERVICE_DESCRIPTOR_TABLE ssdt::GetKeServiceDescriptorTableShadow( _In_ SYSTEM_MODULE_INFORMATION ntoskrnl )
{
    // auto ssdts_addr = util::pattern::ResolveRelative(
    //     util::pattern::Search( ssdts_pattern, ssdts_mask, ( UINT64 )ntoskrnl.ImageBase, ntoskrnl.ImageSize ), 3, 7 );

    auto ssdts_addr = (UINT64)ntoskrnl.ImageBase + 0x0CFCA40;

    if ( !ssdts_addr )
    {
        return { };
    }

    return *( SERVICE_DESCRIPTOR_TABLE* )( ssdts_addr );
}

/*
    Returns a given syscalls address by using the syscall index
    to check which system service dispatch table to use then 
    indexes that table
*/
UINT64 ssdt::GetSyscallAddress( _In_ ULONG syscall_index )
{
    SYSTEM_MODULE_INFORMATION ntoskrnl = { };

    auto status = util::sys::GetModule( "ntoskrnl.exe", &ntoskrnl );

    if ( !NT_SUCCESS( status ) )
    {
        return 0;
    }

    if ( ssdt::IsGuiSyscall( syscall_index ) )
    {
/*
        If the syscall is a gui syscall this means that we need
        to use the shadow system service dispatch table
*/
        auto ssdt = ssdt::GetKeServiceDescriptorTableShadow( ntoskrnl );

        auto sdt = ssdt.sst1;

        if ( syscall_index > sdt.ServiceLimit )
        {
            return 0;
        }

/*
        Get the service table entry for this syscall
*/
        auto service_table_entry = sdt.ServiceTable[ syscall_index ];

        if ( !service_table_entry )
        {
            return 0;
        }

/*
        Get the relative address from the service table to the routine
        via a arithmetic right shift

        we have to do this because on x64 service table entry's hold the
        relative offset from the service table to the routine unlike on x86
        were it holds the absolute address
*/
        auto syscall_relative = service_table_entry >> 4;

        if ( !syscall_relative )
        {
            return 0;
        }

/*
        return the supposed syscall routine address
*/
        return ( UINT64 )sdt.ServiceTable + syscall_relative;
    }
    else
    {
 /*
        If the syscall is not a gui syscall this means that it
        is a native syscall and we should use the normal system
        service descriptor table
*/
        auto ssdt = ssdt::GetKeServiceDescriptorTable( ntoskrnl );

        auto sdt = ssdt.sst1;

        if ( syscall_index > sdt.ServiceLimit )
        {
            return 0;
        }

/*
        Get the service table entry for this syscall
*/
        auto service_table_entry = sdt.ServiceTable[ syscall_index ];

        if ( !service_table_entry )
        {
            return 0;
        }

/*
        Get the relative address from the service table to the routine
        via a arithmetic right shift

        we have to do this because on x64 service table entry's hold the
        relative offset from the service table to the routine unlike on x86
        were it holds the absolute address
*/
        auto syscall_relative = service_table_entry >> 4;

        if ( !syscall_relative )
        {
            return 0;
        }

/*
        return the supposed syscall routine address
*/
        return ( UINT64 )sdt.ServiceTable + syscall_relative;
    }
}

/*
    Returns whether a given syscall is a gui syscall
    via index range checking
*/
BOOLEAN ssdt::IsGuiSyscall( _In_ ULONG syscall_index )
{
/*
    On the windows operating system syscall between 0x1000 and 0x1FFF
    are considered gui syscall this means that they are system calls
    meant for use with drawing and the windows gui if a thread is not
    already marked as a gui thread and calls one of these syscalls it will
    be converted into a gui thread by setting the gui bit inside of its kthread
    structure
*/
    return syscall_index >= 0x1000 && syscall_index <= 0x1FFF;
}
