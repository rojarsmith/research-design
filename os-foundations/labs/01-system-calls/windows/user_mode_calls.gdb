# user_mode_calls.gdb - disassemble two Win32 calls that answer without a
# system call, and one that needs one.
#
#   gdb -q -batch -x user_mode_calls.gdb win32_write.exe
#
# kernel32.dll forwards many functions to kernelbase.dll, so the first
# instruction may be a jump; follow it with a second x/i at the target.

set pagination off
set confirm off
set disassembly-flavor intel
break main
run
echo \n=== GetCurrentProcessId ===\n
x/4i GetCurrentProcessId
echo \n=== GetTickCount64 ===\n
x/12i GetTickCount64
echo \n=== GetProcessHandleCount (first instructions only) ===\n
x/6i GetProcessHandleCount
kill
