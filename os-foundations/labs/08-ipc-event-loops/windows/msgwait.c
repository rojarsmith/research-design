/*
 * msgwait.c - Stage 08, exercise G: one GUI thread that waits on kernel
 * handles and window messages together with MsgWaitForMultipleObjectsEx.
 *
 * Build:  gcc -O2 -Wall -o msgwait.exe msgwait.c -luser32
 * Run:    msgwait.exe          the whole exercise, unattended: starts
 *                              "msgwait.exe loop" in a new process group,
 *                              talks to it over its pipe, then ends it with
 *                              Ctrl+Break
 *         msgwait.exe loop     the GUI thread alone; Ctrl+Break or Ctrl+C
 *                              ends it (it prints its pipe name)
 *
 * The loop process owns a message-only window (no visible window at all)
 * and one thread that waits, in one call, on
 *   [0] the event of an overlapped named-pipe operation  (cf. the socket)
 *   [1] a high-resolution periodic waitable timer, 16 ms (cf. timerfd)
 *   [2] an event set by the console control handler      (cf. signalfd)
 *   [3] an event set by a worker thread                  (cf. eventfd)
 *   and the thread's message queue: WM_APP messages posted by the worker,
 *   WM_TIMER from SetTimer, and WM_QUIT.
 * It waits alertably, so APCs that the worker queues run on this thread
 * inside the wait (WAIT_IO_COMPLETION).
 *
 * Part 1 shows, without any other thread, how the four kinds of wakeup
 * count: an auto-reset event, a semaphore, posted messages, and the
 * "new messages only" rule that MWMO_INPUTAVAILABLE changes.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WM_JOB (WM_APP + 1)

static double qpf;
static LONGLONG t0;
static double ms(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)(t.QuadPart - t0) * 1e3 / qpf;
}
#define LOG(...)                                                                                   \
    do {                                                                                           \
        printf("[%7.1f ms] ", ms());                                                               \
        printf(__VA_ARGS__);                                                                       \
        printf("\n");                                                                              \
    } while (0)

static void die(const char *what)
{
    printf("%s failed: error %lu\n", what, GetLastError());
    exit(1);
}

static const char *wait_name(DWORD r, DWORD n)
{
    static char buf[48];
    if (r == WAIT_TIMEOUT)
        return "WAIT_TIMEOUT";
    if (r == WAIT_IO_COMPLETION)
        return "WAIT_IO_COMPLETION";
    if (r == WAIT_FAILED)
        return "WAIT_FAILED";
    if (r == WAIT_OBJECT_0 + n)
        return "WAIT_OBJECT_0 + n (input in the queue)";
    snprintf(buf, sizeof buf, "WAIT_OBJECT_0 + %lu", r - WAIT_OBJECT_0);
    return buf;
}

/* ---- the window --------------------------------------------------------- */

static int jobs_seen, wm_timers;

static LRESULT CALLBACK wndproc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    switch (m) {
    case WM_JOB:
        jobs_seen++;
        LOG("message: WM_APP+1 from the worker, job %d", (int)wp);
        return 0;
    case WM_TIMER:
        wm_timers++;
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(w, m, wp, lp);
}

static HWND make_window(void)
{
    WNDCLASSW wc = {.lpfnWndProc = wndproc, .hInstance = GetModuleHandleW(NULL),
                    .lpszClassName = L"lab08-msgwait"};
    RegisterClassW(&wc);
    /* HWND_MESSAGE: a message-only window. It is never shown, receives
     * posted and sent messages and timers, but no broadcasts. */
    HWND w = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL,
                             wc.hInstance, NULL);
    if (!w)
        die("CreateWindowEx");
    return w;
}

/* ---- part 1: how wakeups count ------------------------------------------ */

static void part1(HWND w)
{
    printf("1. How each kind of wakeup counts (one thread, nothing else running)\n");
    HANDLE ev = CreateEventW(NULL, FALSE, FALSE, NULL); /* auto-reset */
    HANDLE sem = CreateSemaphoreW(NULL, 0, 100, NULL);
    for (int i = 0; i < 3; i++)
        SetEvent(ev);
    int n = 0;
    while (WaitForSingleObject(ev, 0) == WAIT_OBJECT_0)
        n++;
    printf("  SetEvent x3 on an auto-reset event, then wait until it times out: %d wakeup(s)\n",
           n);
    ReleaseSemaphore(sem, 3, NULL);
    n = 0;
    while (WaitForSingleObject(sem, 0) == WAIT_OBJECT_0)
        n++;
    printf("  ReleaseSemaphore(3), then wait until it times out:                %d wakeup(s)\n",
           n);
    for (int i = 0; i < 3; i++)
        PostMessageW(w, WM_NULL, 0, 0);
    MSG m;
    n = 0;
    while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE))
        n++;
    printf("  PostMessage x3, then PeekMessage until empty:                     %d message(s)\n",
           n);

    PostMessageW(w, WM_NULL, 0, 0);
    PostMessageW(w, WM_NULL, 0, 0);
    PeekMessageW(&m, NULL, 0, 0, PM_NOREMOVE); /* look, take nothing */
    /* GetQueueStatus: the low word holds the kinds of input that are new
     * since the queue was last looked at, the high word the kinds present. */
    DWORD st = GetQueueStatus(QS_ALLINPUT);
    printf("  two messages posted, then one PeekMessage(PM_NOREMOVE); GetQueueStatus: "
           "new 0x%04lx, present 0x%04lx (QS_POSTMESSAGE is 0x%04x)\n",
           (unsigned long)LOWORD(st), (unsigned long)HIWORD(st), QS_POSTMESSAGE);
    double a = ms();
    DWORD r = MsgWaitForMultipleObjectsEx(0, NULL, 200, QS_ALLINPUT, 0);
    printf("    MsgWaitForMultipleObjectsEx(no handles, 200 ms, QS_ALLINPUT, 0)                    "
           "-> %s after %.0f ms\n",
           wait_name(r, 0), ms() - a);
    a = ms();
    r = MsgWaitForMultipleObjectsEx(0, NULL, 200, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    printf("    MsgWaitForMultipleObjectsEx(no handles, 200 ms, QS_ALLINPUT, MWMO_INPUTAVAILABLE) "
           "-> %s after %.0f ms\n",
           wait_name(r, 0), ms() - a);
    while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE))
        ;
    CloseHandle(ev);
    CloseHandle(sem);
}

/* ---- part 2: the loop --------------------------------------------------- */

static HANDLE ctrl_ev, worker_ev;
static volatile DWORD ctrl_thread, ctrl_type;
static HWND g_win;
static HANDLE gui_thread;
static int apcs_seen;

static BOOL WINAPI on_ctrl(DWORD type)
{
    /* The system runs this on a new thread, not on the GUI thread. */
    ctrl_type = type;
    ctrl_thread = GetCurrentThreadId();
    SetEvent(ctrl_ev);
    return TRUE;
}

static void CALLBACK on_apc(ULONG_PTR arg)
{
    apcs_seen++;
    LOG("APC: job %d's callback ran on thread %lu inside the alertable wait", (int)arg,
        GetCurrentThreadId());
}

static DWORD WINAPI worker(LPVOID arg)
{
    (void)arg;
    for (int i = 1; i <= 3; i++) {
        Sleep(120);
        SetEvent(worker_ev);
        PostMessageW(g_win, WM_JOB, (WPARAM)i, 0);
        QueueUserAPC(on_apc, gui_thread, (ULONG_PTR)i);
    }
    return 0;
}

enum { PIPE_CONNECTING, PIPE_READING };

static int loop_main(void)
{
    printf("[loop process: pid %lu, GUI thread %lu]\n", GetCurrentProcessId(),
           GetCurrentThreadId());
    g_win = make_window();
    part1(g_win);

    printf("\n2. One thread waits on four handles and its message queue\n");
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &gui_thread,
                    THREAD_SET_CONTEXT, FALSE, 0);
    ctrl_ev = CreateEventW(NULL, FALSE, FALSE, NULL);
    worker_ev = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!SetConsoleCtrlHandler(on_ctrl, TRUE))
        die("SetConsoleCtrlHandler");

    char pipename[64];
    snprintf(pipename, sizeof pipename, "\\\\.\\pipe\\lab08-msgwait-%lu", GetCurrentProcessId());
    HANDLE pipe = CreateNamedPipeA(pipename,
                                   PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED |
                                       FILE_FLAG_FIRST_PIPE_INSTANCE,
                                   PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE |
                                       PIPE_REJECT_REMOTE_CLIENTS,
                                   1, 4096, 4096, 0, NULL);
    if (pipe == INVALID_HANDLE_VALUE)
        die("CreateNamedPipe");
    OVERLAPPED ov = {.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL)};
    int pstate = PIPE_CONNECTING;
    char buf[256];
    if (ConnectNamedPipe(pipe, &ov) || GetLastError() == ERROR_PIPE_CONNECTED)
        SetEvent(ov.hEvent);

    HANDLE timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                          TIMER_ALL_ACCESS);
    LARGE_INTEGER due = {.QuadPart = -160000}; /* 16 ms, relative, in 100 ns units */
    if (!timer || !SetWaitableTimer(timer, &due, 16, NULL, NULL, FALSE))
        die("waitable timer");
    SetTimer(g_win, 1, 10, NULL); /* WM_TIMER every 10 ms, as asked for */

    HANDLE h[4] = {ov.hEvent, timer, ctrl_ev, worker_ev};
    LOG("loop: listening on %s; waiting on 4 handles and the message queue", pipename);
    fflush(stdout);
    HANDLE wt = CreateThread(NULL, 0, worker, NULL, 0, NULL);

    int ticks = 0, worker_wakeups = 0, waits = 0, quit = 0;
    double start = ms();
    while (!quit) {
        DWORD r = MsgWaitForMultipleObjectsEx(4, h, INFINITE, QS_ALLINPUT,
                                              MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
        waits++;
        if (r == WAIT_OBJECT_0 + 0) {
            DWORD n = 0;
            BOOL ok = GetOverlappedResult(pipe, &ov, &n, FALSE);
            DWORD e = ok ? 0 : GetLastError();
            ResetEvent(ov.hEvent);
            if (pstate == PIPE_CONNECTING) {
                ULONG cpid = 0;
                GetNamedPipeClientProcessId(pipe, &cpid);
                LOG("pipe: a client connected (pid %lu)", cpid);
                pstate = PIPE_READING;
            } else if (ok || e == ERROR_MORE_DATA) {
                buf[n] = 0;
                /* The data is already in buf: the event meant "the read is
                 * done", not "you may read now". A completion. */
                LOG("pipe: overlapped ReadFile completed, %lu bytes already in my buffer: \"%s\"",
                    n, buf);
            } else {
                LOG("pipe: the client went away (error %lu); waiting for the next one", e);
                DisconnectNamedPipe(pipe);
                pstate = PIPE_CONNECTING;
                if (ConnectNamedPipe(pipe, &ov) || GetLastError() == ERROR_PIPE_CONNECTED)
                    SetEvent(ov.hEvent);
                continue;
            }
            /* start the next read; it completes later, or right now */
            if (!ReadFile(pipe, buf, sizeof buf - 1, NULL, &ov) &&
                GetLastError() != ERROR_IO_PENDING && GetLastError() != ERROR_MORE_DATA)
                SetEvent(ov.hEvent); /* let the handler above see the error */
        } else if (r == WAIT_OBJECT_0 + 1) {
            ticks++;
        } else if (r == WAIT_OBJECT_0 + 2) {
            LOG("console control: %s, delivered by the handler on thread %lu; this is thread %lu",
                ctrl_type == CTRL_BREAK_EVENT ? "CTRL_BREAK_EVENT"
                : ctrl_type == CTRL_C_EVENT   ? "CTRL_C_EVENT"
                                              : "another event",
                ctrl_thread, GetCurrentThreadId());
            DestroyWindow(g_win); /* WM_DESTROY -> PostQuitMessage -> WM_QUIT */
        } else if (r == WAIT_OBJECT_0 + 3) {
            worker_wakeups++;
            LOG("event: the worker's event is set");
        } else if (r == WAIT_OBJECT_0 + 4) {
            MSG m;
            while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE)) {
                if (m.message == WM_QUIT) {
                    LOG("message: WM_QUIT; leaving the loop");
                    quit = 1;
                    break;
                }
                TranslateMessage(&m);
                DispatchMessageW(&m);
            }
        } else if (r == WAIT_IO_COMPLETION) {
            /* an APC ran; nothing else to do */
        } else {
            die("MsgWaitForMultipleObjectsEx");
        }
    }
    double secs = (ms() - start) / 1e3;
    WaitForSingleObject(wt, 1000);
    printf("\n  in %.2f s: %lu waits; waitable-timer wakeups %d (%.0f expected at 16 ms); WM_TIMER "
           "%d (%.0f asked for at 10 ms); worker: %d event wakeup(s), %d message(s), %d APC(s)\n",
           secs, (unsigned long)waits, ticks, secs * 1000 / 16, wm_timers, secs * 1000 / 10,
           worker_wakeups, jobs_seen, apcs_seen);
    CloseHandle(pipe);
    return 0;
}

/* ---- the launcher: client and Ctrl+Break sender ------------------------- */

static int launcher(void)
{
    char self[MAX_PATH], cmd[MAX_PATH + 16];
    GetModuleFileNameA(NULL, self, MAX_PATH);
    snprintf(cmd, sizeof cmd, "\"%s\" loop", self);
    STARTUPINFOA si = {.cb = sizeof si};
    PROCESS_INFORMATION pi;
    /* A new process group, so that Ctrl+Break can be sent to it alone. */
    if (!CreateProcessA(self, cmd, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si,
                        &pi))
        die("CreateProcess");
    char pipename[64];
    snprintf(pipename, sizeof pipename, "\\\\.\\pipe\\lab08-msgwait-%lu", pi.dwProcessId);
    HANDLE p = INVALID_HANDLE_VALUE;
    for (int i = 0; i < 200 && p == INVALID_HANDLE_VALUE; i++) {
        p = CreateFileA(pipename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (p == INVALID_HANDLE_VALUE)
            Sleep(10);
    }
    if (p == INVALID_HANDLE_VALUE)
        die("open the loop's pipe");
    Sleep(500);
    const char *msgs[] = {"first message", "second message", "third message"};
    for (int i = 0; i < 3; i++) {
        DWORD w;
        WriteFile(p, msgs[i], (DWORD)strlen(msgs[i]), &w, NULL);
        Sleep(60);
    }
    CloseHandle(p);
    Sleep(200);
    if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pi.dwProcessId)) {
        printf("[launcher] GenerateConsoleCtrlEvent failed: error %lu; terminating the loop\n",
               GetLastError());
        TerminateProcess(pi.hProcess, 99);
    }
    WaitForSingleObject(pi.hProcess, 10000);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    printf("[launcher] the loop process exited with code %lu\n", code);
    return 0;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    LARGE_INTEGER f, t;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t);
    qpf = (double)f.QuadPart;
    t0 = t.QuadPart;
    if (argc >= 2 && strcmp(argv[1], "loop") == 0)
        return loop_main();
    return launcher();
}
