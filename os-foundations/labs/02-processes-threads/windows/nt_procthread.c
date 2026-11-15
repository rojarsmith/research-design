/*
 * nt_procthread.c - Stage 02 companion: what a process costs on NT, what a
 * thread costs, and what a switch between two threads costs when the
 * kernel does it (events) and when user code does it (fibers).
 *
 * Build:  gcc -O2 -Wall -o nt_procthread.exe nt_procthread.c
 * Run:    nt_procthread.exe
 *
 * NT has no fork. CreateProcess builds a new process from an executable
 * file; here the program starts copies of itself with the argument
 * "--child", and each child exits at once.
 *
 * The event ping-pong runs twice: with both threads pinned to the same
 * logical CPU, so every hand-off is a context switch on that CPU, and
 * unpinned, so the scheduler may put the two threads on different CPUs.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double g_freq;

static double now_s(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / g_freq;
}

/* ---- 1. identity: one process, several threads ---------------------- */

static DWORD WINAPI identity_thread(LPVOID arg)
{
    (void)arg;
    printf("  created thread: pid %lu, tid %lu\n", GetCurrentProcessId(),
           GetCurrentThreadId());
    return 0;
}

static void part_identity(const wchar_t *self)
{
    printf("1. Identity\n");
    printf("  main thread:    pid %lu, tid %lu\n", GetCurrentProcessId(),
           GetCurrentThreadId());
    HANDLE t = CreateThread(NULL, 0, identity_thread, NULL, 0, NULL);
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);

    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi;
    wchar_t cmd[MAX_PATH + 32];
    si.cb = sizeof si;
    swprintf(cmd, sizeof cmd / sizeof cmd[0], L"\"%ls\" --child-print", self);
    if (CreateProcessW(self, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        printf("  CreateProcess returned: child pid %lu, its first thread tid %lu\n\n",
               pi.dwProcessId, pi.dwThreadId);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

/* ---- 2. creation cost ------------------------------------------------ */

static DWORD WINAPI empty_thread(LPVOID arg)
{
    (void)arg;
    return 0;
}

static void part_creation(const wchar_t *self, int nproc, int nthread)
{
    printf("2. Creation cost (create, wait for exit, close handles)\n");

    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi;
    wchar_t cmd[MAX_PATH + 32];
    si.cb = sizeof si;
    swprintf(cmd, sizeof cmd / sizeof cmd[0], L"\"%ls\" --child", self);

    double t0 = now_s();
    for (int i = 0; i < nproc; i++) {
        if (!CreateProcessW(self, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            printf("  CreateProcess failed: %lu\n", GetLastError());
            return;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    double t1 = now_s();
    printf("  CreateProcess + wait:  %8.1f us each  (%d runs)\n",
           (t1 - t0) * 1e6 / nproc, nproc);

    t0 = now_s();
    for (int i = 0; i < nthread; i++) {
        HANDLE t = CreateThread(NULL, 0, empty_thread, NULL, 0, NULL);
        WaitForSingleObject(t, INFINITE);
        CloseHandle(t);
    }
    t1 = now_s();
    printf("  CreateThread + wait:   %8.1f us each  (%d runs)\n\n",
           (t1 - t0) * 1e6 / nthread, nthread);
}

/* ---- 3. kernel-scheduled switch: two threads, two events ------------ */

static HANDLE ev_ping, ev_pong;
static volatile LONG g_rounds;

static DWORD WINAPI pong_thread(LPVOID arg)
{
    (void)arg;
    for (LONG i = 0; i < g_rounds; i++) {
        WaitForSingleObject(ev_ping, INFINITE);
        SetEvent(ev_pong);
    }
    return 0;
}

static double ping_pong(LONG rounds, DWORD_PTR mask)
{
    ev_ping = CreateEventW(NULL, FALSE, FALSE, NULL); /* auto-reset */
    ev_pong = CreateEventW(NULL, FALSE, FALSE, NULL);
    g_rounds = rounds;
    HANDLE t = CreateThread(NULL, 0, pong_thread, NULL, CREATE_SUSPENDED, NULL);
    DWORD_PTR old = 0;
    if (mask) {
        SetThreadAffinityMask(t, mask);
        old = SetThreadAffinityMask(GetCurrentThread(), mask);
    }
    ResumeThread(t);

    double t0 = now_s();
    for (LONG i = 0; i < rounds; i++) {
        SetEvent(ev_ping);
        WaitForSingleObject(ev_pong, INFINITE);
    }
    double t1 = now_s();

    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
    CloseHandle(ev_ping);
    CloseHandle(ev_pong);
    if (mask && old)
        SetThreadAffinityMask(GetCurrentThread(), old);
    /* one round = two hand-offs */
    return (t1 - t0) * 1e9 / (2.0 * rounds);
}

/* ---- 4. user-scheduled switch: two fibers --------------------------- */

static LPVOID fib_main, fib_other;
static volatile long g_fiber_switches;

static void WINAPI other_fiber(LPVOID arg)
{
    long n = (long)(INT_PTR)arg;
    for (long i = 0; i < n; i++) {
        g_fiber_switches++;
        SwitchToFiber(fib_main);
    }
    for (;;) /* a fiber function must not return: that would exit the thread */
        SwitchToFiber(fib_main);
}

static double fiber_ping_pong(long n)
{
    fib_main = ConvertThreadToFiber(NULL);
    fib_other = CreateFiber(64 * 1024, other_fiber, (LPVOID)(INT_PTR)n);
    g_fiber_switches = 0;
    double t0 = now_s();
    for (long i = 0; i < n; i++) {
        g_fiber_switches++;
        SwitchToFiber(fib_other);
    }
    double t1 = now_s();
    DeleteFiber(fib_other);
    ConvertFiberToThread();
    return (t1 - t0) * 1e9 / (double)g_fiber_switches;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--child") == 0)
        return 0;
    if (argc > 1 && strcmp(argv[1], "--child-print") == 0) {
        printf("  child process:  pid %lu, tid %lu\n", GetCurrentProcessId(),
               GetCurrentThreadId());
        return 0;
    }

    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_freq = (double)f.QuadPart;
    setvbuf(stdout, NULL, _IONBF, 0);

    wchar_t self[MAX_PATH];
    GetModuleFileNameW(NULL, self, MAX_PATH);

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    printf("Logical processors: %lu\n\n", si.dwNumberOfProcessors);

    part_identity(self);
    part_creation(self, 200, 5000);

    printf("3. Switch between two kernel threads (event ping-pong, ns per hand-off)\n");
    DWORD_PTR cpu = (DWORD_PTR)1 << (si.dwNumberOfProcessors > 2 ? 2 : 0);
    printf("  same logical CPU:      %8.0f ns\n", ping_pong(200000, cpu));
    printf("  unpinned:              %8.0f ns\n\n", ping_pong(200000, 0));

    printf("4. Switch between two fibers on one thread (SwitchToFiber)\n");
    printf("  fiber switch:          %8.1f ns\n", fiber_ping_pong(10000000));
    return 0;
}
