# Windows-SSDT
Looping the SSDT on windows to retrieve the addresses of all native system calls on the system.

The kernelmode part of this project uses relative offsets from ntoskrnl's base address
to retrieve the KeServiceDescriptorTable and KeServiceDescriptorTableShadow structures
these may have to be changed and or updated to support older / newer version of windows
however everything is setup in the project to allow for pattern scanning aswell as the
patterns to these structures being included in the project.

![ssdt-test](https://github.com/Aston539/Windows-SSDT/assets/140078446/be9e3d4a-c61d-4fe2-a129-cce07268b0d0)
