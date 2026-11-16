/*
 * nt_timer_res.c - Stage 04, exercise F: timers on NT, observed from user
 * mode without administrator rights.
 *
 * Build:  gcc -O2 -Wall -o nt_timer_res.exe nt_timer_res.c -lwinmm
 * Run:    nt_timer_res.exe
 *
 * 1. What the documented APIs report: timeGetDevCaps and
 *    GetSystemTimeAdjustment.
 * 2. How long Sleep(1) really takes, before and while this process holds
 *    a 1 ms timeBeginPeriod request.
 * 3. A periodic waitable timer with a 10 ms period and one thread blocked
 *    in WaitForSingleObject: the user-mode counterpart of tickdev's
 *    blocked reader. Once with a normal timer, once with
 *    CREATE_WAITABLE_TIMER_HIGH_RESOLUTION.
 *
 * Every number is measured with QueryPerformanceCounter. The machine's
 * other processes can request a higher resolution too, so record which
 * programs were running (a browser playing video often does).
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

static double g_freq;

static double now_us(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1e6 / g_freq;
}

static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : x > y;
}

static void summary(const char *label, double *v, int n)
{
    qsort(v, n, sizeof v[0], cmp);
    printf("  %-44s min %8.1f  median %8.1f  max %8.1f us  (%d samples)\n", label,
           v[0], v[n / 2], v[n - 1], n);
}

static void sleep_test(const char *label)
{
    enum { N = 100 };
    double d[N];
    for (int i = 0; i < N; i++) {
        double t0 = now_us();
        Sleep(1);
        d[i] = now_us() - t0;
    }
    summary(label, d, N);
}

static void periodic_test(DWORD flags, const char *label)
{
    enum { N = 200, PERIOD_MS = 10 };
    double dt[N];
    HANDLE t = CreateWaitableTimerExW(NULL, NULL, flags, TIMER_ALL_ACCESS);
    if (!t) {
        printf("  %s: CreateWaitableTimerExW failed, error %lu\n", label, GetLastError());
        return;
    }
    LARGE_INTEGER due;
    due.QuadPart = -10000LL * PERIOD_MS; /* relative, in 100 ns units */
    double start = now_us(), prev = start;
    if (!SetWaitableTimer(t, &due, PERIOD_MS, NULL, NULL, FALSE)) {
        printf("  %s: SetWaitableTimer failed, error %lu\n", label, GetLastError());
        CloseHandle(t);
        return;
    }
    for (int i = 0; i < N; i++) {
        WaitForSingleObject(t, INFINITE);
        double now = now_us();
        dt[i] = now - prev;
        prev = now;
    }
    CancelWaitableTimer(t);
    CloseHandle(t);
    /* An auto-reset timer that expires again before the thread has woken
     * signals once: unlike tickdev's seq or timerfd's count, the waiter
     * cannot tell how many periods it missed. Count them from the clock. */
    printf("  %s, 10 ms period: %d wake-ups while %.0f periods passed\n", label, N,
           (prev - start) / (PERIOD_MS * 1000.0));
    summary("    time between wake-ups", dt, N);
}

int main(void)
{
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_freq = (double)f.QuadPart;
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("1. What the APIs report\n");
    TIMECAPS tc;
    if (timeGetDevCaps(&tc, sizeof tc) == MMSYSERR_NOERROR)
        printf("  timeGetDevCaps: wPeriodMin %u ms, wPeriodMax %u ms\n", tc.wPeriodMin,
               tc.wPeriodMax);
    DWORD adj, inc;
    BOOL disabled;
    if (GetSystemTimeAdjustment(&adj, &inc, &disabled))
        printf("  GetSystemTimeAdjustment: increment %lu x 100 ns (%.4f ms), adjustment "
               "%lu, adjustment disabled %s\n",
               inc, inc / 10000.0, adj, disabled ? "TRUE" : "FALSE");
    printf("  QueryPerformanceFrequency: %.0f Hz\n", g_freq);

    printf("\n2. Sleep(1), 100 times\n");
    sleep_test("without timeBeginPeriod");
    MMRESULT r = timeBeginPeriod(1);
    sleep_test(r == TIMERR_NOERROR ? "with timeBeginPeriod(1)" : "timeBeginPeriod(1) FAILED");
    if (r == TIMERR_NOERROR)
        timeEndPeriod(1);
    sleep_test("after timeEndPeriod(1)");

    printf("\n3. A thread blocked on a periodic waitable timer, 200 periods\n");
    periodic_test(0, "normal timer");
    periodic_test(CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, "high-resolution timer");
    return 0;
}
