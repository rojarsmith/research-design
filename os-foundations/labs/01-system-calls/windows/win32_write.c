/*
 * win32_write.c - the Win32 version of "write(1, buf, n)", small enough to
 * step through in a debugger and find the Nt* routine underneath.
 *
 * Build:  gcc -O0 -g -Wall -o win32_write.exe win32_write.c
 * Run:    win32_write.exe
 * Debug:  gdb -q -batch -x find_nt_routine.gdb win32_write.exe
 *
 * The first WriteFile goes to standard output, whatever that is: a console
 * window, a pipe, or a file if you redirect it. The second goes to a
 * temporary disk file. Compare the native routines that each one reaches.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int main(void)
{
    static const char msg[] = "hello from WriteFile\n";
    DWORD n = 0;

    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(out, msg, sizeof msg - 1, &n, NULL);

    HANDLE f = CreateFileA("win32_write.tmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (f != INVALID_HANDLE_VALUE) {
        WriteFile(f, msg, sizeof msg - 1, &n, NULL);
        CloseHandle(f);
    }
    return 0;
}
