/*
 * iocp.c - Stage 08, exercise H (optional): the pipe server again, as one
 * thread on an I/O completion port.
 *
 * Build:  gcc -O2 -Wall -o iocp.exe iocp.c
 * Run:    iocp.exe               (starts three copies of itself as clients)
 *
 * The server opens three instances of one named pipe, overlapped, and
 * associates each with one completion port, using the instance number as
 * the completion key. It starts a read, a write or a connect on each, and
 * then only ever calls GetQueuedCompletionStatusEx. Every entry it gets
 * back is a finished operation: for a read, the bytes are already in the
 * buffer it passed when it started the read, and the entry says how many.
 *
 * Things that are not I/O reach the same port as packets the program
 * posts itself with PostQueuedCompletionStatus: a thread-pool timer posts
 * a "tick" every 16 ms, and the server posts "quit" to itself when the last
 * client has gone. One wait, one queue, everything a completion.
 */
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NCLIENTS 3
#define NMSG 4
#define KEY_TICK 100
#define KEY_QUIT 101

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

enum state { CONNECTING, READING, WRITING, DONE };

struct inst {
    OVERLAPPED ov; /* first member: the OVERLAPPED * in an entry is the instance */
    HANDLE pipe;
    enum state st;
    int id;
    ULONG pid;
    char buf[256];
    char out[256];
    int msgs;
};

static void start_read(struct inst *in)
{
    memset(&in->ov, 0, sizeof in->ov);
    in->st = READING;
    /* The result arrives as a completion packet even if ReadFile finishes
     * at once; FILE_SKIP_COMPLETION_PORT_ON_SUCCESS would change that. */
    if (!ReadFile(in->pipe, in->buf, sizeof in->buf - 1, NULL, &in->ov) &&
        GetLastError() != ERROR_IO_PENDING && GetLastError() != ERROR_MORE_DATA) {
        LOG("instance %d: ReadFile failed at once: error %lu", in->id, GetLastError());
        in->st = DONE;
    }
}

static void CALLBACK on_timer(PTP_CALLBACK_INSTANCE ci, PVOID port, PTP_TIMER t)
{
    (void)ci;
    (void)t;
    PostQueuedCompletionStatus((HANDLE)port, 0, KEY_TICK, NULL);
}

static int client(const char *name, int id)
{
    HANDLE p = INVALID_HANDLE_VALUE;
    for (int i = 0; i < 200 && p == INVALID_HANDLE_VALUE; i++) {
        p = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (p == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_PIPE_BUSY)
                WaitNamedPipeA(name, 1000);
            else
                Sleep(10);
        }
    }
    if (p == INVALID_HANDLE_VALUE)
        die("client CreateFile");
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(p, &mode, NULL, NULL);
    int ok = 0;
    for (int i = 1; i <= NMSG; i++) {
        char msg[64], reply[256];
        DWORD n;
        snprintf(msg, sizeof msg, "client %d, message %d", id, i);
        if (!WriteFile(p, msg, (DWORD)strlen(msg), &n, NULL))
            break;
        if (ReadFile(p, reply, sizeof reply - 1, &n, NULL))
            ok++;
        Sleep(20 + 15 * id);
    }
    CloseHandle(p);
    return ok == NMSG ? 0 : 1;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    LARGE_INTEGER f, t;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t);
    qpf = (double)f.QuadPart;
    t0 = t.QuadPart;
    if (argc >= 4 && strcmp(argv[1], "client") == 0)
        return client(argv[2], atoi(argv[3]));

    char name[64];
    snprintf(name, sizeof name, "\\\\.\\pipe\\lab08-iocp-%lu", GetCurrentProcessId());
    /* NumberOfConcurrentThreads 1: one thread serves the port. */
    HANDLE port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if (!port)
        die("CreateIoCompletionPort");
    struct inst in[NCLIENTS];
    for (int i = 0; i < NCLIENTS; i++) {
        memset(&in[i], 0, sizeof in[i]);
        in[i].id = i;
        in[i].pipe = CreateNamedPipeA(
            name, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | (i == 0 ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, NCLIENTS, 4096,
            4096, 0, NULL);
        if (in[i].pipe == INVALID_HANDLE_VALUE)
            die("CreateNamedPipe");
        if (!CreateIoCompletionPort(in[i].pipe, port, (ULONG_PTR)i, 0))
            die("associate with the port");
        in[i].st = CONNECTING;
        if (!ConnectNamedPipe(in[i].pipe, &in[i].ov)) {
            DWORD e = GetLastError();
            if (e == ERROR_PIPE_CONNECTED) /* no packet comes for this case */
                PostQueuedCompletionStatus(port, 0, (ULONG_PTR)i, &in[i].ov);
            else if (e != ERROR_IO_PENDING)
                die("ConnectNamedPipe");
        }
    }
    LOG("server: %d instances of %s on one completion port", NCLIENTS, name);

    double start = ms();
    PTP_TIMER tp = CreateThreadpoolTimer(on_timer, port, NULL);
    FILETIME due;
    LARGE_INTEGER d = {.QuadPart = -160000};
    due.dwLowDateTime = d.LowPart;
    due.dwHighDateTime = (DWORD)d.HighPart;
    SetThreadpoolTimer(tp, &due, 16, 0);

    char self[MAX_PATH];
    GetModuleFileNameA(NULL, self, MAX_PATH);
    HANDLE kids[NCLIENTS];
    for (int i = 0; i < NCLIENTS; i++) {
        char cmd[MAX_PATH + 96];
        snprintf(cmd, sizeof cmd, "\"%s\" client %s %d", self, name, i);
        STARTUPINFOA si = {.cb = sizeof si};
        PROCESS_INFORMATION pi;
        if (!CreateProcessA(self, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            die("CreateProcess");
        kids[i] = pi.hProcess;
        CloseHandle(pi.hThread);
    }

    int ticks = 0, calls = 0, entries = 0, maxbatch = 0, done = 0, quit = 0;
    while (!quit) {
        OVERLAPPED_ENTRY e[16];
        ULONG n = 0;
        if (!GetQueuedCompletionStatusEx(port, e, 16, &n, INFINITE, FALSE))
            die("GetQueuedCompletionStatusEx");
        calls++;
        entries += (int)n;
        if ((int)n > maxbatch)
            maxbatch = (int)n;
        for (ULONG k = 0; k < n; k++) {
            if (e[k].lpCompletionKey == KEY_TICK) {
                ticks++;
                continue;
            }
            if (e[k].lpCompletionKey == KEY_QUIT) {
                quit = 1;
                continue;
            }
            struct inst *s = (struct inst *)e[k].lpOverlapped;
            DWORD bytes = e[k].dwNumberOfBytesTransferred;
            DWORD got;
            BOOL ok = GetOverlappedResult(s->pipe, &s->ov, &got, FALSE);
            DWORD err = ok ? 0 : GetLastError();
            switch (s->st) {
            case CONNECTING:
                GetNamedPipeClientProcessId(s->pipe, &s->pid);
                LOG("instance %d: connect completed, client pid %lu", s->id, s->pid);
                start_read(s);
                break;
            case READING:
                if (!ok && err != ERROR_MORE_DATA) {
                    LOG("instance %d: read completed with error %lu: client pid %lu is gone "
                        "after %d messages",
                        s->id, err, s->pid, s->msgs);
                    DisconnectNamedPipe(s->pipe);
                    s->st = DONE;
                    if (++done == NCLIENTS)
                        PostQueuedCompletionStatus(port, 0, KEY_QUIT, NULL);
                    break;
                }
                s->buf[bytes] = 0;
                s->msgs++;
                LOG("instance %d: read completed, %lu bytes already in the buffer: \"%s\"", s->id,
                    bytes, s->buf);
                snprintf(s->out, sizeof s->out, "ack %d", s->msgs);
                memset(&s->ov, 0, sizeof s->ov);
                s->st = WRITING;
                if (!WriteFile(s->pipe, s->out, (DWORD)strlen(s->out), NULL, &s->ov) &&
                    GetLastError() != ERROR_IO_PENDING) {
                    LOG("instance %d: WriteFile failed: error %lu", s->id, GetLastError());
                    s->st = DONE;
                }
                break;
            case WRITING:
                start_read(s);
                break;
            case DONE:
                break;
            }
        }
    }
    double secs = (ms() - start) / 1e3;
    SetThreadpoolTimer(tp, NULL, 0, 0);
    WaitForThreadpoolTimerCallbacks(tp, TRUE);
    CloseThreadpoolTimer(tp);
    WaitForMultipleObjects(NCLIENTS, kids, TRUE, 5000);
    int failed = 0;
    for (int i = 0; i < NCLIENTS; i++) {
        DWORD code = 1;
        GetExitCodeProcess(kids[i], &code);
        failed += code != 0;
        CloseHandle(kids[i]);
        CloseHandle(in[i].pipe);
    }
    printf("\n  in %.2f s: %d GetQueuedCompletionStatusEx calls returned %d entries (at most %d "
           "at once); %d timer packets (%.0f expected at 16 ms); clients that got every reply: "
           "%d of %d\n",
           secs, calls, entries, maxbatch, ticks, secs * 1000 / 16, NCLIENTS - failed, NCLIENTS);
    CloseHandle(port);
    return 0;
}
