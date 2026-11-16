/*
 * lockbench.c - Stage 05 companion: the same contention test with five
 * Windows locks, then the same test with the lock split per thread.
 *
 * Build:  gcc -O2 -Wall -o lockbench.exe lockbench.c -lsynchronization
 * Run:    lockbench.exe              (0.25 s per cell)
 *         lockbench.exe 1.0          (1 s per cell)
 *
 * Every thread loops: take the lock, run a short critical section on
 * shared data, release, then run a longer stretch of private work. The
 * critical section is about a fifth of each iteration, so from about five
 * threads on the lock is busy all the time and threads must wait.
 *
 * Locks compared:
 *   SRWLOCK            AcquireSRWLockExclusive / ReleaseSRWLockExclusive
 *   CRITICAL_SECTION   InitializeCriticalSection (default spin count)
 *   CS spin 4000       InitializeCriticalSectionAndSpinCount(cs, 4000)
 *   WaitOnAddress      a three-state mutex (0 free, 1 held, 2 held and
 *                      maybe waiters) built on WaitOnAddress and
 *                      WakeByAddressSingle, the same design as a futex
 *                      mutex on Linux (linux/futex_mutex.c)
 *   WOA + spin         the same, but it retries for a while before waiting
 *   spinlock           test-and-test-and-set with _mm_pause, never sleeps
 *
 * For each cell the program prints the throughput, the spread between the
 * busiest and the least busy thread, the process's kernel and user CPU
 * time during the cell (GetProcessTimes), and for the WaitOnAddress locks
 * how many WaitOnAddress and WakeByAddressSingle calls they made.
 *
 * WaitOnAddress needs Windows 8 or later and links against
 * API-MS-Win-Core-Synch-l1-2-0.dll; with MinGW-w64 that is
 * -lsynchronization.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXT 64
#define CACHE 64

static double g_freq;
static double now_s(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / g_freq;
}

static uint64_t ft64(FILETIME f) { return ((uint64_t)f.dwHighDateTime << 32) | f.dwLowDateTime; }

static void proc_times(uint64_t *k, uint64_t *u)
{
    FILETIME c, e, kt, ut;
    GetProcessTimes(GetCurrentProcess(), &c, &e, &kt, &ut);
    *k = ft64(kt);
    *u = ft64(ut);
}

/* ---- the locks ------------------------------------------------------ */

enum kind { K_SRW, K_CS, K_CS4000, K_WOA, K_WOASPIN, K_SPIN, K_COUNT };
static const char *kind_name[K_COUNT] = {
    "SRWLOCK", "CRITICAL_SECTION", "CS spin 4000", "WaitOnAddress", "WOA + spin", "spinlock",
};

/* One lock plus the data it protects, alone in its cache lines. */
struct shard {
    union {
        SRWLOCK srw;
        CRITICAL_SECTION cs;
        volatile LONG word; /* WaitOnAddress mutex and spinlock */
    } u;
    char pad1[CACHE];
    volatile uint64_t counter; /* incremented under the lock */
    volatile uint64_t state;   /* the critical section's work */
    char pad2[CACHE];
};

struct worker {
    struct shard *sh;
    enum kind kind;
    long long ops;
    long long waits, wakes; /* WaitOnAddress, WakeByAddressSingle calls */
    char pad[CACHE];
};

static volatile LONG g_go, g_stop;
static int g_cswork = 40, g_outwork = 160;

static void woa_lock(struct worker *w, volatile LONG *m, int spin)
{
    LONG c = InterlockedCompareExchange(m, 1, 0);
    if (c == 0)
        return;
    for (int i = 0; i < spin; i++) { /* optional: retry before sleeping */
        _mm_pause();
        if (*m == 0 && (c = InterlockedCompareExchange(m, 1, 0)) == 0)
            return;
    }
    if (c != 2)
        c = InterlockedExchange(m, 2);
    while (c != 0) {
        LONG two = 2;
        /* Sleeps only if *m is still 2 when the kernel looks. */
        WaitOnAddress(m, &two, sizeof *m, INFINITE);
        w->waits++;
        c = InterlockedExchange(m, 2);
    }
}

static void woa_unlock(struct worker *w, volatile LONG *m)
{
    if (InterlockedExchange(m, 0) == 2) {
        WakeByAddressSingle((PVOID)m);
        w->wakes++;
    }
}

static void spin_lock(volatile LONG *m)
{
    for (;;) {
        if (InterlockedExchange(m, 1) == 0)
            return;
        while (*m != 0)
            _mm_pause();
    }
}

static void spin_unlock(volatile LONG *m) { InterlockedExchange(m, 0); }

static void lock(struct worker *w)
{
    struct shard *s = w->sh;
    switch (w->kind) {
    case K_SRW: AcquireSRWLockExclusive(&s->u.srw); break;
    case K_CS:
    case K_CS4000: EnterCriticalSection(&s->u.cs); break;
    case K_WOA: woa_lock(w, &s->u.word, 0); break;
    case K_WOASPIN: woa_lock(w, &s->u.word, 50); break;
    case K_SPIN: spin_lock(&s->u.word); break;
    default: break;
    }
}

static void unlock(struct worker *w)
{
    struct shard *s = w->sh;
    switch (w->kind) {
    case K_SRW: ReleaseSRWLockExclusive(&s->u.srw); break;
    case K_CS:
    case K_CS4000: LeaveCriticalSection(&s->u.cs); break;
    case K_WOA:
    case K_WOASPIN: woa_unlock(w, &s->u.word); break;
    case K_SPIN: spin_unlock(&s->u.word); break;
    default: break;
    }
}

static DWORD WINAPI worker_main(LPVOID arg)
{
    struct worker *w = arg;
    uint64_t mine = (uint64_t)(uintptr_t)w;
    while (!g_go)
        _mm_pause();
    while (!g_stop) {
        lock(w);
        uint64_t x = w->sh->state;
        for (int i = 0; i < g_cswork; i++) /* critical section */
            x = x * 6364136223846793005ull + 1442695040888963407ull;
        w->sh->state = x;
        w->sh->counter++;
        unlock(w);
        for (int i = 0; i < g_outwork; i++) /* private work */
            mine = mine * 6364136223846793005ull + 1;
        w->ops++;
    }
    return (DWORD)mine;
}

static void shard_init(struct shard *s, enum kind k)
{
    memset(s, 0, sizeof *s);
    if (k == K_SRW)
        InitializeSRWLock(&s->u.srw);
    else if (k == K_CS)
        InitializeCriticalSection(&s->u.cs);
    else if (k == K_CS4000)
        InitializeCriticalSectionAndSpinCount(&s->u.cs, 4000);
}

static void shard_fini(struct shard *s, enum kind k)
{
    if (k == K_CS || k == K_CS4000)
        DeleteCriticalSection(&s->u.cs);
}

struct result {
    double mops, ns_per_op, fair, kms, ums, waits_k, wakes_k;
    int ok;
};

/* Run one cell: nthreads threads, nshards locks, for `seconds`. */
static struct result run_cell(enum kind k, int nthreads, int nshards, double seconds)
{
    static struct shard shards[MAXT];
    static struct worker workers[MAXT];
    HANDLE th[MAXT];
    struct result r = {0};

    for (int i = 0; i < nshards; i++)
        shard_init(&shards[i], k);
    for (int i = 0; i < nthreads; i++) {
        memset(&workers[i], 0, sizeof workers[i]);
        workers[i].sh = &shards[i % nshards];
        workers[i].kind = k;
    }
    g_go = 0;
    g_stop = 0;
    for (int i = 0; i < nthreads; i++)
        th[i] = CreateThread(NULL, 0, worker_main, &workers[i], 0, NULL);

    uint64_t k0, u0, k1, u1;
    Sleep(20); /* let every thread reach the start line */
    proc_times(&k0, &u0);
    double t0 = now_s();
    InterlockedExchange(&g_go, 1);
    Sleep((DWORD)(seconds * 1000));
    InterlockedExchange(&g_stop, 1);
    double t1 = now_s();
    WaitForMultipleObjects((DWORD)nthreads, th, TRUE, INFINITE);
    proc_times(&k1, &u1);
    for (int i = 0; i < nthreads; i++)
        CloseHandle(th[i]);

    long long total = 0, mn = -1, mx = 0, waits = 0, wakes = 0;
    for (int i = 0; i < nthreads; i++) {
        long long o = workers[i].ops;
        total += o;
        if (mn < 0 || o < mn)
            mn = o;
        if (o > mx)
            mx = o;
        waits += workers[i].waits;
        wakes += workers[i].wakes;
    }
    uint64_t counted = 0;
    for (int i = 0; i < nshards; i++) {
        counted += shards[i].counter;
        shard_fini(&shards[i], k);
    }
    double el = t1 - t0;
    r.mops = (double)total / el / 1e6;
    r.ns_per_op = total ? el * 1e9 / (double)total : 0;
    r.fair = mx ? (double)mn / (double)mx : 0;
    r.kms = (double)(k1 - k0) / 1e4;
    r.ums = (double)(u1 - u0) / 1e4;
    r.waits_k = total ? 1000.0 * (double)waits / (double)total : 0;
    r.wakes_k = total ? 1000.0 * (double)wakes / (double)total : 0;
    r.ok = counted == (uint64_t)total; /* every op went through a lock */
    return r;
}

static void print_header(void)
{
    printf("  %-17s %7s %9s %9s %8s %10s %9s %10s %10s\n", "lock", "threads", "Mops/s",
           "ns/op", "min/max", "kernel ms", "user ms", "waits/1k", "wakes/1k");
}

static void print_row(enum kind k, int n, struct result r)
{
    printf("  %-17s %7d %9.2f %9.1f %8.2f %10.0f %9.0f", kind_name[k], n, r.mops, r.ns_per_op,
           r.fair, r.kms, r.ums);
    if (k == K_WOA || k == K_WOASPIN)
        printf(" %10.1f %10.1f", r.waits_k, r.wakes_k);
    else
        printf(" %10s %10s", "-", "-");
    printf("%s\n", r.ok ? "" : "  COUNT MISMATCH");
}

int main(int argc, char **argv)
{
    double seconds = argc > 1 ? atof(argv[1]) : 0.25;
    if (seconds < 0.05)
        seconds = 0.05;
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_freq = (double)f.QuadPart;
    setvbuf(stdout, NULL, _IONBF, 0);

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int ncpu = (int)si.dwNumberOfProcessors;
    printf("Logical processors: %d. %.2f s per cell. Critical section %d LCG steps, "
           "private work %d steps.\n\n",
           ncpu, seconds, g_cswork, g_outwork);

    /* Calibrate: one iteration with no lock at all. */
    {
        uint64_t x = 1;
        double t0 = now_s();
        for (int j = 0; j < 1000000; j++) {
            for (int i = 0; i < g_cswork + g_outwork; i++)
                x = x * 6364136223846793005ull + 1;
            __asm__ volatile("" : "+r"(x));
        }
        double t1 = now_s();
        printf("One iteration without a lock: %.1f ns (critical section about %.0f%% of it)\n\n",
               (t1 - t0) * 1e9 / 1e6, 100.0 * g_cswork / (g_cswork + g_outwork));
    }

    int counts[] = {1, 2, 4, 8, ncpu, 2 * ncpu};
    int ncounts = sizeof counts / sizeof counts[0];

    printf("1. One lock shared by every thread\n");
    print_header();
    for (int k = 0; k < K_COUNT; k++) {
        for (int c = 0; c < ncounts; c++) {
            int n = counts[c];
            if (n > MAXT || (c > 0 && n == counts[c - 1]))
                continue;
            print_row((enum kind)k, n, run_cell((enum kind)k, n, 1, seconds));
        }
        printf("\n");
    }

    printf("2. Splitting the lock: one lock per thread instead of one for all\n");
    print_header();
    int n = ncpu > MAXT ? MAXT : ncpu;
    enum kind split[] = {K_SRW, K_WOA};
    for (int i = 0; i < 2; i++) {
        struct result one = run_cell(split[i], n, 1, seconds);
        struct result per = run_cell(split[i], n, n, seconds);
        printf("  one lock:\n");
        print_row(split[i], n, one);
        printf("  one lock per thread:\n");
        print_row(split[i], n, per);
    }
    return 0;
}
