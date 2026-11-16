/*
 * crossproc.c - Stage 05 companion: why a lock shared by two processes on
 * Windows needs something other than WaitOnAddress.
 *
 * Build:  gcc -O2 -Wall -o crossproc.exe crossproc.c -lsynchronization
 * Run:    crossproc.exe          (starts copies of itself as the second process)
 *
 * Both processes map the same page-file-backed section (a named file
 * mapping), so they see the same physical page. Atomic instructions on that
 * page work across processes. The question is only how a thread that has to
 * wait gets woken.
 *
 * Part 1: one waiter, one waker. The waiter waits for a word in shared
 *         memory to change, the waker changes it and wakes it.
 *           a. both in one process, WaitOnAddress / WakeByAddressAll
 *           b. in two processes, WaitOnAddress / WakeByAddressAll
 *           c. in two processes, a named auto-reset event
 *         Learn's WaitOnAddress page says the wake must come from "another
 *         thread in the same process". Case b shows what happens otherwise.
 *
 * Part 2: two processes increment one shared counter under one lock, for
 *         a fixed time each:
 *           a. a WaitOnAddress mutex in the shared page (each wait has a
 *              20 ms timeout, so a lost wake costs 20 ms instead of forever)
 *           b. a named kernel mutex (CreateMutexW)
 *           c. a "benaphore": an interlocked count in the shared page as
 *              the fast path, and a named semaphore only for waiting
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct shared {
    volatile LONG word;          /* part 1: the value the waiter watches */
    volatile LONG ready;         /* part 1: the waiter is about to wait */
    volatile LONG64 wake_qpc;    /* part 1: when the waker changed the word */
    volatile LONG go, stop;      /* part 2: start and stop flags */
    volatile LONG lockword;      /* part 2a: WaitOnAddress mutex */
    volatile LONG benacount;     /* part 2c: benaphore count */
    volatile LONG64 counter;     /* part 2: protected by the lock */
    volatile LONG64 ops[2];      /* part 2: increments per process */
    volatile LONG64 waits[2];    /* part 2: times each process had to wait */
    volatile LONG64 timeouts[2]; /* part 2a: waits that ended by timeout */
};

static double g_freq;
static LONG64 qpc(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}
static double ms_since(LONG64 t0) { return (double)(qpc() - t0) * 1e3 / g_freq; }

static struct shared *map_shared(const wchar_t *name, int create)
{
    HANDLE h = create ? CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                           4096, name)
                      : OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, name);
    if (!h) {
        fprintf(stderr, "file mapping %ls failed: %lu\n", name, GetLastError());
        exit(1);
    }
    struct shared *s = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 4096);
    if (!s) {
        fprintf(stderr, "MapViewOfFile failed: %lu\n", GetLastError());
        exit(1);
    }
    return s; /* the handle stays open for the life of the process */
}

/* ---- part 1: the waiter ---------------------------------------------- */

/* Wait up to 2 s for s->word to become non-zero. Returns how many ms after
 * the waker's change the waiter noticed it, or -1 if it never did. */
static double wait_woa(struct shared *s, int *returns)
{
    LONG zero = 0;
    LONG64 t0 = qpc();
    *returns = 0;
    s->ready = 1;
    while (s->word == 0) {
        double left = 2000.0 - ms_since(t0);
        if (left <= 0)
            break;
        WaitOnAddress(&s->word, &zero, sizeof s->word, (DWORD)left + 1);
        (*returns)++;
    }
    if (s->word == 0)
        return -1;
    return (double)(qpc() - s->wake_qpc) * 1e3 / g_freq;
}

static DWORD WINAPI waiter_thread(LPVOID arg)
{
    struct shared *s = arg;
    int returns;
    double ms = wait_woa(s, &returns);
    printf("    waiter thread: noticed the change %.3f ms after the wake "
           "(WaitOnAddress returned %d time(s))\n",
           ms, returns);
    return 0;
}

/* ---- part 2: three cross-process locks -------------------------------- */

enum lk { L_WOA, L_MUTEX, L_BENA };

struct lockctx {
    struct shared *s;
    HANDLE mutex, sem;
    int me;
};

static void lock(struct lockctx *c, enum lk k)
{
    struct shared *s = c->s;
    if (k == L_WOA) {
        volatile LONG *m = &s->lockword;
        LONG v = InterlockedCompareExchange(m, 1, 0);
        if (v == 0)
            return;
        if (v != 2)
            v = InterlockedExchange(m, 2);
        while (v != 0) {
            LONG two = 2;
            s->waits[c->me]++;
            if (!WaitOnAddress(m, &two, sizeof *m, 20) && GetLastError() == ERROR_TIMEOUT)
                s->timeouts[c->me]++;
            v = InterlockedExchange(m, 2);
        }
    } else if (k == L_MUTEX) {
        WaitForSingleObject(c->mutex, INFINITE);
    } else {
        if (InterlockedIncrement(&s->benacount) > 1) {
            s->waits[c->me]++;
            WaitForSingleObject(c->sem, INFINITE);
        }
    }
}

static void unlock(struct lockctx *c, enum lk k)
{
    struct shared *s = c->s;
    if (k == L_WOA) {
        if (InterlockedExchange(&s->lockword, 0) == 2)
            WakeByAddressSingle((PVOID)&s->lockword); /* reaches only this process */
    } else if (k == L_MUTEX) {
        ReleaseMutex(c->mutex);
    } else {
        if (InterlockedDecrement(&s->benacount) > 0)
            ReleaseSemaphore(c->sem, 1, NULL);
    }
}

static void hammer(struct lockctx *c, enum lk k)
{
    struct shared *s = c->s;
    uint64_t x = (uint64_t)c->me + 1;
    while (!s->go)
        YieldProcessor();
    while (!s->stop) {
        lock(c, k);
        s->counter++;
        for (int i = 0; i < 40; i++) /* hold the lock for a little while */
            x = x * 6364136223846793005ull + 1;
        unlock(c, k);
        for (int i = 0; i < 160; i++)
            x = x * 6364136223846793005ull + 1;
        s->ops[c->me]++;
    }
    if (x == 42)
        printf("%llu\n", (unsigned long long)x); /* keep x alive */
}

static void open_objects(struct lockctx *c, DWORD parent_pid, int create)
{
    wchar_t name[64];
    swprintf(name, 64, L"Local\\lab05-mutex-%lu", parent_pid);
    c->mutex = create ? CreateMutexW(NULL, FALSE, name) : OpenMutexW(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, name);
    swprintf(name, 64, L"Local\\lab05-sem-%lu", parent_pid);
    c->sem = create ? CreateSemaphoreW(NULL, 0, 1, name) : OpenSemaphoreW(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, name);
    if (!c->mutex || !c->sem) {
        fprintf(stderr, "named objects failed: %lu\n", GetLastError());
        exit(1);
    }
}

/* ---- the second process ---------------------------------------------- */

static int child_main(const char *what, DWORD parent_pid)
{
    wchar_t name[64];
    swprintf(name, 64, L"Local\\lab05-shared-%lu", parent_pid);
    struct shared *s = map_shared(name, 0);

    if (strcmp(what, "woa-wait") == 0) {
        int returns;
        double ms = wait_woa(s, &returns);
        if (ms < 0)
            printf("    waiter process: gave up after 2000 ms, the word is %ld "
                   "(WaitOnAddress returned %d time(s))\n",
                   s->word, returns);
        else
            printf("    waiter process: noticed the change %.3f ms after the wake "
                   "(WaitOnAddress returned %d time(s))\n",
                   ms, returns);
        return 0;
    }
    if (strcmp(what, "event-wait") == 0) {
        swprintf(name, 64, L"Local\\lab05-event-%lu", parent_pid);
        HANDLE ev = OpenEventW(SYNCHRONIZE, FALSE, name);
        s->ready = 1;
        DWORD r = WaitForSingleObject(ev, 2000);
        double ms = (double)(qpc() - s->wake_qpc) * 1e3 / g_freq;
        printf("    waiter process: %s %.3f ms after the SetEvent\n",
               r == WAIT_OBJECT_0 ? "woken" : "TIMED OUT", ms);
        return 0;
    }
    /* "lock-N": run part 2 with lock kind N as process 1 */
    struct lockctx c = {s, NULL, NULL, 1};
    open_objects(&c, parent_pid, 0);
    s->ready = 1;
    hammer(&c, (enum lk)atoi(what + 5));
    return 0;
}

static DWORD WINAPI stop_after_1s(LPVOID arg)
{
    struct shared *s = arg;
    Sleep(1000);
    InterlockedExchange(&s->stop, 1);
    return 0;
}

static HANDLE spawn(const wchar_t *self, const char *what)
{
    wchar_t cmd[MAX_PATH + 64];
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi;
    si.cb = sizeof si;
    swprintf(cmd, sizeof cmd / sizeof cmd[0], L"\"%ls\" --child %hs %lu", self, what,
             GetCurrentProcessId());
    if (!CreateProcessW(self, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "CreateProcess failed: %lu\n", GetLastError());
        exit(1);
    }
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

/* Wake the waiter once it has said it is about to wait, plus 200 ms so
 * that it is certainly asleep inside the wait. */
static void wake_after_ready(struct shared *s, HANDLE ev)
{
    while (!s->ready)
        Sleep(1);
    Sleep(200);
    s->wake_qpc = qpc();
    if (ev) {
        SetEvent(ev);
    } else {
        InterlockedExchange(&s->word, 1);
        WakeByAddressAll((PVOID)&s->word);
    }
}

static void reset1(struct shared *s)
{
    s->word = 0;
    s->ready = 0;
    s->wake_qpc = 0;
}

int main(int argc, char **argv)
{
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_freq = (double)f.QuadPart;
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc == 4 && strcmp(argv[1], "--child") == 0)
        return child_main(argv[2], (DWORD)strtoul(argv[3], NULL, 10));

    wchar_t self[MAX_PATH], name[64];
    GetModuleFileNameW(NULL, self, MAX_PATH);
    DWORD pid = GetCurrentProcessId();
    swprintf(name, 64, L"Local\\lab05-shared-%lu", pid);
    struct shared *s = map_shared(name, 1);
    printf("Shared page mapped at %p in this process (pid %lu)\n\n", (void *)s, pid);

    printf("1. One waiter, one waker, a word in shared memory\n");
    printf("  a. same process: WaitOnAddress in a thread, WakeByAddressAll from main\n");
    reset1(s);
    HANDLE t = CreateThread(NULL, 0, waiter_thread, s, 0, NULL);
    wake_after_ready(s, NULL);
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);

    printf("  b. two processes: WaitOnAddress in the child, WakeByAddressAll here\n");
    reset1(s);
    HANDLE ch = spawn(self, "woa-wait");
    wake_after_ready(s, NULL);
    WaitForSingleObject(ch, INFINITE);
    CloseHandle(ch);

    printf("  c. two processes: a named auto-reset event\n");
    reset1(s);
    swprintf(name, 64, L"Local\\lab05-event-%lu", pid);
    HANDLE ev = CreateEventW(NULL, FALSE, FALSE, name);
    ch = spawn(self, "event-wait");
    wake_after_ready(s, ev);
    WaitForSingleObject(ch, INFINITE);
    CloseHandle(ch);
    CloseHandle(ev);

    printf("\n2. Two processes, one shared counter, one lock, 1 s each\n");
    printf("  %-34s %12s %12s %10s %10s %9s\n", "lock", "ops parent", "ops child", "waits",
           "timeouts", "counter");
    struct lockctx c = {s, NULL, NULL, 0};
    open_objects(&c, pid, 1);
    const char *names[] = {"WaitOnAddress mutex (20 ms timeout)", "named mutex (CreateMutexW)",
                           "benaphore (count + named semaphore)"};
    for (int k = 0; k < 3; k++) {
        s->go = 0;
        s->stop = 0;
        s->ready = 0;
        s->lockword = 0;
        s->benacount = 0;
        s->counter = 0;
        for (int i = 0; i < 2; i++)
            s->ops[i] = s->waits[i] = s->timeouts[i] = 0;
        char what[16];
        snprintf(what, sizeof what, "lock-%d", k);
        ch = spawn(self, what);
        while (!s->ready) /* the child has mapped the page and opened the objects */
            Sleep(1);
        HANDLE stopper = CreateThread(NULL, 0, stop_after_1s, s, 0, NULL);
        InterlockedExchange(&s->go, 1);
        hammer(&c, (enum lk)k); /* this process is process 0 */
        WaitForSingleObject(stopper, INFINITE);
        CloseHandle(stopper);
        WaitForSingleObject(ch, INFINITE);
        CloseHandle(ch);
        long long total = s->ops[0] + s->ops[1];
        char waits[24] = "-", timeouts[24] = "-"; /* not counted for the kernel mutex */
        if (k != L_MUTEX)
            snprintf(waits, sizeof waits, "%lld", (long long)(s->waits[0] + s->waits[1]));
        if (k == L_WOA)
            snprintf(timeouts, sizeof timeouts, "%lld",
                     (long long)(s->timeouts[0] + s->timeouts[1]));
        printf("  %-34s %12lld %12lld %10s %10s %9s\n", names[k], (long long)s->ops[0],
               (long long)s->ops[1], waits, timeouts, s->counter == total ? "ok" : "WRONG");
    }
    return 0;
}
