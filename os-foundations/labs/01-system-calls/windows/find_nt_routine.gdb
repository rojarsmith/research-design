# find_nt_routine.gdb - stop in the ntdll routines that WriteFile can reach,
# print who called them, and show the instructions up to the syscall.
#
#   gdb -q -batch -x find_nt_routine.gdb win32_write.exe
#
# ntdll.dll is not loaded until the program starts, so stop at main first,
# then set breakpoints on the exported Nt* names. MinGW gdb reads these
# names from the DLL export table; there are no Microsoft PDB symbols, so
# frames inside kernelbase.dll may show only the nearest exported name.

set pagination off
set confirm off
set disassembly-flavor intel
break main
run
break NtWriteFile
break NtDeviceIoControlFile
continue
echo \n=== first stop: backtrace ===\n
bt 6
echo \n=== instructions at the stop ===\n
x/8i $pc
echo \n=== rax, r10, rcx before the syscall (r10 gets rcx, eax gets the service number) ===\n
stepi 2
info registers rax r10 rcx
continue
echo \n=== second stop: backtrace ===\n
bt 6
continue
