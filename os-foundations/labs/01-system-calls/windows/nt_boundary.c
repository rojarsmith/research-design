/*
 * nt_boundary.c - Stage 01 companion: which Win32 calls cross into the NT
 * kernel, what one crossing costs, and how the kernel treats a bad pointer
 * that comes from user mode.
 *
 * Build (WinLibs or any MinGW-w64 gcc, x86-64):
 *     gcc -O2 -Wall -o nt_boundary.exe nt_boundary.c
 * Run:
 *     nt_boundary.exe        (each timing row runs for at least 0.5 s)
 *     nt_boundary.exe 2      (at least 2 s per row)
 *
 * Part 1 times six calls. Three are predicted to stay in user mode, three
 * to end in a `syscall` instruction inside ntdll. For each row the program
 * also reads this thread's kernel time and user time before and after the
 * loop (GetThreadTimes). Those counters move in coarse steps, so each row
 * runs for a fixed wall time rather than a fixed number of calls.
 *
 * Part 2 dumps the first bytes of a few ntdll Nt* stubs and finds the
 * `mov eax, imm32` (system service number) and `syscall` (0F 05) in them.
 * Service numbers are internal and change between Windows builds.
 *
 * Part 3 passes a kernel-space address and a NULL address as buffers to
 * the native NtQueryInformationProcess and NtWriteFile and to the
 * documented Win32 WriteFile, and prints what comes back at each layer.
 *
 * NtQueryInformationProcess is called through GetProcAddress because Learn
 * documents it as internal and subject to change between releases. Do not
 * use it like this in production code.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef NTSTATUS(NTAPI *NtQueryInformationProcess_t)(HANDLE, PROCESSINFOCLASS,
                                                      PVOID, ULONG, PULONG);

static NtQueryInformationProcess_t pNtQueryInformationProcess;
static HANDLE g_event;
static volatile DWORD g_sink;

/* A plain function call that the optimizer may not remove or inline. */
__attribute__((noinline)) static DWORD plain_function(DWORD x)
{
    __asm__ volatile("" ::: "memory");
    return x + 1;
}

static void call_plain(void) { g_sink = plain_function(g_sink); }
static void call_getpid(void) { g_sink += GetCurrentProcessId(); }
static void call_tick(void) { g_sink += (DWORD)GetTickCount64(); }

static void call_handlecount(void)
{
    DWORD n = 0;
    GetProcessHandleCount(GetCurrentProcess(), &n);
    g_sink += n;
}

static void call_ntqip(void)
{
    PROCESS_BASIC_INFORMATION pbi;
    ULONG len = 0;
    pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation,
                               &pbi, sizeof pbi, &len);
    g_sink += len;
}

static void call_wait(void) { g_sink += WaitForSingleObject(g_event, 0); }

struct row {
    const char *name;
    const char *expect; /* what the reader should predict */
    void (*fn)(void);
};

static const struct row rows[] = {
    {"plain C function call", "user only", call_plain},
    {"GetCurrentProcessId()", "user only (reads the TEB)", call_getpid},
    {"GetTickCount64()", "user only (shared page)", call_tick},
    {"GetProcessHandleCount()", "kernel (Win32 wrapper)", call_handlecount},
    {"NtQueryInformationProcess()", "kernel (native, direct)", call_ntqip},
    {"WaitForSingleObject(set, 0)", "kernel", call_wait},
};

static uint64_t ft64(FILETIME ft)
{
    return ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
}

static void thread_times(uint64_t *kernel100ns, uint64_t *user100ns)
{
    FILETIME c, e, k, u;
    GetThreadTimes(GetCurrentThread(), &c, &e, &k, &u);
    *kernel100ns = ft64(k);
    *user100ns = ft64(u);
}

/* Each row runs for at least `seconds` of wall time, in batches of `batch`
 * calls, so that even the 2 ns rows run long enough for the thread's
 * kernel and user times to move. */
static void part1_timing(long batch, double seconds)
{
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);

    printf("Part 1: cost per call, and this thread's kernel and user time during the loop\n");
    printf("        (batches of %ld calls, at least %.1f s per row)\n\n", batch, seconds);
    printf("  %-29s %-27s %11s %8s %9s %9s\n", "call", "prediction to check",
           "calls", "ns/call", "kernel ms", "user ms");

    for (size_t r = 0; r < sizeof rows / sizeof rows[0]; r++) {
        const struct row *w = &rows[r];
        for (long i = 0; i < 1000; i++) /* warm up caches and page in code */
            w->fn();

        uint64_t k0, u0, k1, u1;
        long long calls = 0;
        double elapsed = 0;
        thread_times(&k0, &u0);
        QueryPerformanceCounter(&t0);
        do {
            for (long i = 0; i < batch; i++)
                w->fn();
            calls += batch;
            QueryPerformanceCounter(&t1);
            elapsed = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;
        } while (elapsed < seconds);
        thread_times(&k1, &u1);

        printf("  %-29s %-27s %11lld %8.1f %9.0f %9.0f\n", w->name, w->expect,
               calls, elapsed * 1e9 / (double)calls, (double)(k1 - k0) / 1e4,
               (double)(u1 - u0) / 1e4);
    }
    printf("\n");
}

/* Print the first bytes of an exported ntdll function and look for
 * "mov eax, imm32" (B8 xx xx xx xx) and "syscall" (0F 05). */
static void dump_stub(HMODULE ntdll, const char *name)
{
    const unsigned char *p = (const unsigned char *)GetProcAddress(ntdll, name);
    if (!p) {
        printf("  %-26s not exported\n", name);
        return;
    }
    printf("  %-26s %p:", name, (const void *)p);
    for (int i = 0; i < 24; i++)
        printf(" %02X", p[i]);
    printf("\n");

    int ssn = -1, sys = -1;
    for (int i = 0; i < 24; i++) {
        if (ssn < 0 && p[i] == 0xB8 && i + 4 < 24)
            ssn = (int)(p[i + 1] | (p[i + 2] << 8) | (p[i + 3] << 16) |
                        ((unsigned)p[i + 4] << 24));
        if (sys < 0 && p[i] == 0x0F && i + 1 < 24 && p[i + 1] == 0x05)
            sys = i;
    }
    if (ssn >= 0)
        printf("  %-26s mov eax, 0x%X  (system service number on this build)\n", "", ssn);
    if (sys >= 0)
        printf("  %-26s syscall at +%d\n", "", sys);
    else
        printf("  %-26s no syscall instruction in the first 24 bytes\n", "");
}

static void part2_stubs(void)
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    printf("Part 2: the user-mode side of a system call, inside ntdll.dll\n\n");
    dump_stub(ntdll, "NtQueryInformationProcess");
    dump_stub(ntdll, "NtWaitForSingleObject");
    dump_stub(ntdll, "NtWriteFile");
    dump_stub(ntdll, "NtClose");
    printf("\n");
}

typedef NTSTATUS(NTAPI *NtWriteFile_t)(HANDLE, HANDLE, PVOID, PVOID,
                                        PIO_STATUS_BLOCK, PVOID, ULONG,
                                        PLARGE_INTEGER, PULONG);
typedef ULONG(NTAPI *RtlNtStatusToDosError_t)(NTSTATUS);

/* The system's own text for a Win32 error code. */
static const char *win32_text(DWORD err)
{
    static char buf[256];
    DWORD n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                             buf, sizeof buf, NULL);
    if (n == 0)
        n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL, err, 0, buf, sizeof buf, NULL);
    while (n > 0 && (buf[n - 1] == '\r' || buf[n - 1] == '\n' || buf[n - 1] == ' '))
        buf[--n] = '\0';
    return n ? buf : "(no message text)";
}

static void part3_bad_pointers(void)
{
    /* On x64 Windows the user address space ends far below this value;
     * 0xFFFF8000'00000000 is the bottom of the kernel half. */
    void *kernel_ptr = (void *)(uintptr_t)0xFFFF800000000000ull;
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    NtWriteFile_t pNtWriteFile = (NtWriteFile_t)(void *)GetProcAddress(ntdll, "NtWriteFile");
    RtlNtStatusToDosError_t pToDos =
        (RtlNtStatusToDosError_t)(void *)GetProcAddress(ntdll, "RtlNtStatusToDosError");
    PROCESS_BASIC_INFORMATION pbi;
    ULONG len = 0;
    NTSTATUS st;

    printf("Part 3: bad pointers handed to the kernel from user mode\n\n");

    st = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation,
                                    &pbi, sizeof pbi, &len);
    printf("  NtQueryInformationProcess(good buffer)    -> NTSTATUS 0x%08lX, len %lu, pid %llu\n",
           (unsigned long)st, (unsigned long)len,
           (unsigned long long)(uintptr_t)pbi.UniqueProcessId);

    st = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation,
                                    kernel_ptr, sizeof pbi, NULL);
    printf("  NtQueryInformationProcess(kernel address) -> NTSTATUS 0x%08lX\n",
           (unsigned long)st);

    st = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation,
                                    NULL, sizeof pbi, NULL);
    printf("  NtQueryInformationProcess(NULL)           -> NTSTATUS 0x%08lX\n\n",
           (unsigned long)st);

    char path[MAX_PATH], dir[MAX_PATH];
    GetTempPathA(sizeof dir, dir);
    GetTempFileNameA(dir, "ntb", 0, path);
    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (f == INVALID_HANDLE_VALUE) {
        printf("  could not create a temporary file (error %lu)\n", GetLastError());
        return;
    }

    /* Win32 first: WriteFile returns BOOL and leaves a Win32 error code. */
    struct {
        const char *label;
        const void *buf;
        DWORD n;
    } cases[] = {
        {"good buffer", "hello\n", 6},
        {"kernel address", kernel_ptr, 16},
        {"NULL", NULL, 16},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        DWORD written = 12345;
        SetLastError(0);
        BOOL ok = WriteFile(f, cases[i].buf, cases[i].n, &written, NULL);
        DWORD err = GetLastError();
        printf("  WriteFile(file, %-14s, %2lu) -> %-5s written %lu",
               cases[i].label, (unsigned long)cases[i].n, ok ? "TRUE" : "FALSE",
               (unsigned long)written);
        if (!ok)
            printf(", error %lu: %s", (unsigned long)err, win32_text(err));
        printf("\n");
    }
    printf("\n");

    /* Then the native call underneath, which returns the NTSTATUS directly. */
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        IO_STATUS_BLOCK iosb;
        memset(&iosb, 0, sizeof iosb);
        st = pNtWriteFile(f, NULL, NULL, NULL, &iosb, (PVOID)cases[i].buf,
                          cases[i].n, NULL, NULL);
        printf("  NtWriteFile(file, %-14s, %2lu) -> NTSTATUS 0x%08lX",
               cases[i].label, (unsigned long)cases[i].n, (unsigned long)st);
        if (st != 0 && pToDos)
            printf(" -> Win32 %lu", (unsigned long)pToDos(st));
        printf("\n");
    }
    CloseHandle(f);
}

int main(int argc, char **argv)
{
    double seconds = argc > 1 ? atof(argv[1]) : 0.5;
    if (seconds < 0.05)
        seconds = 0.05;

    pNtQueryInformationProcess = (NtQueryInformationProcess_t)(void *)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
    g_event = CreateEventA(NULL, TRUE, TRUE, NULL); /* manual reset, already set */
    if (!pNtQueryInformationProcess || !g_event) {
        fprintf(stderr, "setup failed\n");
        return 1;
    }

    OSVERSIONINFOW v;
    memset(&v, 0, sizeof v);
    v.dwOSVersionInfoSize = sizeof v;
    typedef LONG(NTAPI * RtlGetVersion_t)(OSVERSIONINFOW *);
    RtlGetVersion_t pRtlGetVersion = (RtlGetVersion_t)(void *)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
    if (pRtlGetVersion && pRtlGetVersion(&v) == 0)
        printf("Windows %lu.%lu build %lu\n\n", v.dwMajorVersion, v.dwMinorVersion,
               v.dwBuildNumber);

    part1_timing(100000, seconds);
    part2_stubs();
    part3_bad_pointers();
    CloseHandle(g_event);
    return 0;
}
