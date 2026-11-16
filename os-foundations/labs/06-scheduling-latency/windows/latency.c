/*
 * latency.c - Stage 06 companion: a thread that wakes 60 times a second
 * from a waitable timer, idle and while other threads keep every logical
 * processor busy, at several thread priorities. It prints how late each
 * wakeup was as a text histogram. The Windows counterpart of
 * linux/latency.c, with the same buckets.
 *
 * Build:  gcc -O2 -Wall -o latency.exe latency.c
 * Run:    latency.exe                 every row below, 4 s each
 *         latency.exe 10              10 s each
 *         latency.exe 5 7             only row 7, for 5 s (for a WPR trace)
 *
 * Rows:
 *   1. idle, THREAD_PRIORITY_NORMAL, high-resolution timer
 *   2. idle, THREAD_PRIORITY_NORMAL, ordinary waitable timer
 *   3. load, THREAD_PRIORITY_NORMAL
 *   4. load, THREAD_PRIORITY_NORMAL, priority boosts disabled
 *   5. load, THREAD_PRIORITY_ABOVE_NORMAL
 *   6. load, THREAD_PRIORITY_HIGHEST
 *   7. load, THREAD_PRIORITY_TIME_CRITICAL
 * "load" is one spinning thread per logical processor, at
 * THREAD_PRIORITY_NORMAL, in this same process (NORMAL_PRIORITY_CLASS).
 *
 * The high-resolution timer comes from CreateWaitableTimerExW with
 * CREATE_WAITABLE_TIMER_HIGH_RESOLUTION (Windows 10 1803 and later). The
 * timer is re-armed every period with the time left until the next due
 * time, so the due times stay on a fixed 1/60 s grid measured with
 * QueryPerformanceCounter. Lateness is the time between the due time and
 * the moment the thread runs again after WaitForSingleObject.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

static double g_freq;
static volatile LONG stop;

static int64_t qpc(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

static DWORD WINAPI load(LPVOID arg)
{
    volatile uint64_t x = (uint64_t)(uintptr_t)arg;
    while (!stop)
        x = x * 6364136223846793005ull + 1;
    return 0;
}

static int cmpd(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* The same buckets as linux/latency.c, in microseconds. */
static const double edge_us[] = {50, 100, 200, 500, 1000, 2000, 4000, 8000, 16667};
#define NB (int)(sizeof edge_us / sizeof edge_us[0] + 1)

static void histogram(const double *us, int n)
{
    static const char *label[NB] = {
        "       < 50 us", "  50 - 100 us", " 100 - 200 us", " 200 - 500 us", "0.5 - 1 ms   ",
        "  1 - 2 ms   ", "  2 - 4 ms   ", "  4 - 8 ms   ", "8 - 16.7 ms  ", "    > 16.7 ms",
    };
    int count[NB] = {0};
    for (int i = 0; i < n; i++) {
        int b = 0;
        while (b < NB - 1 && us[i] >= edge_us[b])
            b++;
        count[b]++;
    }
    int max = 1;
    for (int b = 0; b < NB; b++)
        if (count[b] > max)
            max = count[b];
    for (int b = 0; b < NB; b++) {
        printf("  %s %6d  ", label[b], count[b]);
        int bar = (count[b] * 50 + max - 1) / max;
        for (int i = 0; i < bar; i++)
            putchar('#');
        putchar('\n');
    }
}

struct row {
    const char *name;
    int load, prio, noboost, hires;
};

static const struct row rows[] = {
    {"idle, NORMAL, high-resolution timer", 0, THREAD_PRIORITY_NORMAL, 0, 1},
    {"idle, NORMAL, ordinary waitable timer", 0, THREAD_PRIORITY_NORMAL, 0, 0},
    {"load, NORMAL", 1, THREAD_PRIORITY_NORMAL, 0, 1},
    {"load, NORMAL, priority boosts disabled", 1, THREAD_PRIORITY_NORMAL, 1, 1},
    {"load, ABOVE_NORMAL", 1, THREAD_PRIORITY_ABOVE_NORMAL, 0, 1},
    {"load, HIGHEST", 1, THREAD_PRIORITY_HIGHEST, 0, 1},
    {"load, TIME_CRITICAL", 1, THREAD_PRIORITY_TIME_CRITICAL, 0, 1},
};
#define NROWS (int)(sizeof rows / sizeof rows[0])

static void run_row(int r, double seconds, int ncpu)
{
    const struct row *w = &rows[r];
    int n = (int)(seconds * 60.0);
    double *us = calloc((size_t)n, sizeof *us);
    HANDLE th[256];
    int nload = w->load ? (ncpu > 256 ? 256 : ncpu) : 0;

    printf("%d. %s: %d load thread(s), %d wakeups at 60 Hz\n", r + 1, w->name, nload, n);
    stop = 0;
    for (int i = 0; i < nload; i++)
        th[i] = CreateThread(NULL, 0, load, (LPVOID)(uintptr_t)(i + 1), 0, NULL);

    HANDLE self = GetCurrentThread();
    SetThreadPriority(self, w->prio);
    SetThreadPriorityBoost(self, w->noboost ? TRUE : FALSE); /* TRUE disables boosts */
    HANDLE timer = CreateWaitableTimerExW(NULL, NULL,
                                          w->hires ? CREATE_WAITABLE_TIMER_HIGH_RESOLUTION : 0,
                                          TIMER_ALL_ACCESS);
    if (!timer) {
        printf("  CreateWaitableTimerExW failed: %lu\n", GetLastError());
        exit(1);
    }
    printf("  measuring thread: tid %lu, priority %d\n", GetCurrentThreadId(),
           GetThreadPriority(self));
    Sleep(100); /* let the load threads start */

    const double period = g_freq / 60.0; /* in QPC ticks */
    int64_t start = qpc() + (int64_t)period;
    long long k = 0, missed = 0;
    for (int i = 0; i < n; i++) {
        int64_t due = start + (int64_t)(k * period);
        int64_t now = qpc();
        LARGE_INTEGER rel;
        /* negative = relative, in 100 ns units; at least 1 unit */
        double left100ns = (double)(due - now) * 1e7 / g_freq;
        rel.QuadPart = -(LONGLONG)(left100ns > 1 ? left100ns : 1);
        SetWaitableTimerEx(timer, &rel, 0, NULL, NULL, NULL, 0);
        WaitForSingleObject(timer, INFINITE);
        int64_t t = qpc();
        us[i] = (double)(t - due) * 1e6 / g_freq;
        k++;
        /* If we woke after the next due time too, those periods are gone. */
        while (start + (int64_t)(k * period) < t) {
            k++;
            missed++;
        }
    }
    stop = 1;
    if (nload)
        WaitForMultipleObjects((DWORD)nload, th, TRUE, INFINITE);
    for (int i = 0; i < nload; i++)
        CloseHandle(th[i]);
    CloseHandle(timer);
    SetThreadPriority(self, THREAD_PRIORITY_NORMAL);
    SetThreadPriorityBoost(self, FALSE);

    histogram(us, n);
    qsort(us, (size_t)n, sizeof *us, cmpd);
    double sum = 0;
    int over[4] = {0};
    const double lim[4] = {1000, 2000, 4000, 8000};
    for (int i = 0; i < n; i++) {
        sum += us[i];
        for (int j = 0; j < 4; j++)
            if (us[i] > lim[j])
                over[j]++;
    }
    printf("  min %.1f us, median %.1f us, mean %.1f us, p99 %.1f us, max %.1f us\n", us[0],
           us[n / 2], sum / n, us[(n * 99) / 100], us[n - 1]);
    printf("  later than 1 ms: %d, 2 ms: %d, 4 ms: %d, 8 ms: %d; whole periods missed: %lld\n\n",
           over[0], over[1], over[2], over[3], missed);
    free(us);
}

int main(int argc, char **argv)
{
    double seconds = argc > 1 ? atof(argv[1]) : 4.0;
    int only = argc > 2 ? atoi(argv[2]) : 0;
    if (seconds < 0.5)
        seconds = 0.5;
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_freq = (double)f.QuadPart;
    setvbuf(stdout, NULL, _IONBF, 0);

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    printf("pid %lu, %lu logical processors, QPC %.0f Hz, process priority class 0x%lX\n\n",
           GetCurrentProcessId(), si.dwNumberOfProcessors, g_freq,
           GetPriorityClass(GetCurrentProcess()));
    for (int r = 0; r < NROWS; r++)
        if (only == 0 || only == r + 1)
            run_row(r, seconds, (int)si.dwNumberOfProcessors);
    return 0;
}
