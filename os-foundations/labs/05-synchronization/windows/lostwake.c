/*
 * lostwake.c - Stage 05 companion: lost wakeups on Windows, caught in the
 * act, and two designs that cannot lose one.
 *
 * Build:  gcc -O2 -Wall -o lostwake.exe lostwake.c -lsynchronization
 * Run:    lostwake.exe              (20000 rounds per mode, 5 us window)
 *         lostwake.exe 50000 20     (50000 rounds, 20 us window)
 *
 * Two threads play ping-pong on one shared variable, `turn`. Each waits
 * until turn names it, then hands the turn to the other thread and
 * notifies it. A wait that misses its notification would sleep forever,
 * so every wait here has a 50 ms timeout. A wait that times out while
 * turn already names the waiter is a lost wakeup: the other thread
 * published the change and notified, and the waiter did not hear it.
 * After 20 lost wakeups a mode stops early.
 *
 * Between "I have decided to sleep" and "I am asleep", each waiter spins
 * for a short window, so that the race shows up often enough to count.
 *
 * Modes:
 *   1. CONDITION_VARIABLE, BROKEN: the waiter tests turn before it takes
 *      the lock, then takes the lock and sleeps.
 *   2. CONDITION_VARIABLE, correct: the waiter tests turn while it holds
 *      the lock, and SleepConditionVariableSRW releases the lock only once
 *      the thread is queued on the condition variable.
 *   3. WaitOnAddress, correct: the waiter passes the value it saw, and
 *      WaitOnAddress returns at once if turn no longer holds that value.
 *   4. WaitOnAddress, BROKEN: the notifier wakes first and publishes the
 *      new turn afterwards.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define TIMEOUT_MS 50
#define MAX_LOST 20

static volatile LONG turn;
static SRWLOCK lk = SRWLOCK_INIT;
static CONDITION_VARIABLE cv = CONDITION_VARIABLE_INIT;
static volatile LONG lost, timeouts, stop;
static int mode, rounds;
static double g_freq, window_us;

static double now_us(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1e6 / g_freq;
}

static void race_window(void)
{
    double t0 = now_us();
    while (now_us() - t0 < window_us)
        YieldProcessor();
}

/* A timed-out wait: lost if the turn was already ours. */
static void timed_out(int me)
{
    InterlockedIncrement(&timeouts);
    if (turn == me && InterlockedIncrement(&lost) >= MAX_LOST)
        stop = 1;
}

static void wait_turn(int me)
{
    switch (mode) {
    case 1: /* BROKEN: test outside the lock, sleep inside it */
        while (turn != me && !stop) {
            race_window();
            AcquireSRWLockExclusive(&lk);
            if (!SleepConditionVariableSRW(&cv, &lk, TIMEOUT_MS, 0) &&
                GetLastError() == ERROR_TIMEOUT)
                timed_out(me);
            ReleaseSRWLockExclusive(&lk);
        }
        break;
    case 2: /* correct: test and sleep under the same lock */
        AcquireSRWLockExclusive(&lk);
        while (turn != me && !stop) {
            race_window(); /* harmless: the notifier needs the lock to change turn */
            if (!SleepConditionVariableSRW(&cv, &lk, TIMEOUT_MS, 0) &&
                GetLastError() == ERROR_TIMEOUT)
                timed_out(me);
        }
        ReleaseSRWLockExclusive(&lk);
        break;
    case 3:
    case 4: { /* WaitOnAddress with the value we saw */
        LONG seen;
        while ((seen = turn) != me && !stop) {
            race_window();
            if (!WaitOnAddress(&turn, &seen, sizeof turn, TIMEOUT_MS) &&
                GetLastError() == ERROR_TIMEOUT)
                timed_out(me);
        }
        break;
    }
    }
}

static void give_turn(int other)
{
    switch (mode) {
    case 1:
    case 2:
        AcquireSRWLockExclusive(&lk);
        turn = other;
        ReleaseSRWLockExclusive(&lk);
        WakeAllConditionVariable(&cv);
        break;
    case 3:
        InterlockedExchange(&turn, other);
        WakeByAddressAll((PVOID)&turn);
        break;
    case 4: /* BROKEN: wake first, publish later */
        WakeByAddressAll((PVOID)&turn);
        race_window();
        InterlockedExchange(&turn, other);
        break;
    }
}

static volatile LONG done_rounds[2];

static DWORD WINAPI player(LPVOID arg)
{
    int me = (int)(INT_PTR)arg;
    for (int r = 0; r < rounds && !stop; r++) {
        wait_turn(me);
        if (stop)
            break;
        give_turn(1 - me);
        done_rounds[me] = r + 1;
    }
    stop = 1; /* let the other player out of its last wait */
    return 0;
}

int main(int argc, char **argv)
{
    rounds = argc > 1 ? atoi(argv[1]) : 20000;
    window_us = argc > 2 ? atof(argv[2]) : 5.0;
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_freq = (double)f.QuadPart;
    setvbuf(stdout, NULL, _IONBF, 0);

    static const char *names[] = {
        "",
        "1. CONDITION_VARIABLE, test outside the lock (BROKEN)",
        "2. CONDITION_VARIABLE, test under the lock",
        "3. WaitOnAddress with the value seen",
        "4. WaitOnAddress, wake before publish (BROKEN)",
    };
    printf("%d rounds per mode, %.1f us race window, %d ms wait timeout, "
           "stop after %d lost wakeups\n\n",
           rounds, window_us, TIMEOUT_MS, MAX_LOST);
    printf("  %-55s %8s %9s %8s %10s\n", "mode", "rounds", "timeouts", "lost", "elapsed");
    for (mode = 1; mode <= 4; mode++) {
        turn = 0;
        lost = timeouts = stop = 0;
        done_rounds[0] = done_rounds[1] = 0;
        double t0 = now_us();
        HANDLE a = CreateThread(NULL, 0, player, (LPVOID)0, 0, NULL);
        HANDLE b = CreateThread(NULL, 0, player, (LPVOID)1, 0, NULL);
        WaitForSingleObject(a, INFINITE);
        WaitForSingleObject(b, INFINITE);
        CloseHandle(a);
        CloseHandle(b);
        double t1 = now_us();
        printf("  %-55s %8ld %9ld %8ld %8.0f ms%s\n", names[mode], done_rounds[0], timeouts,
               lost, (t1 - t0) / 1000.0, lost >= MAX_LOST ? "  (stopped early)" : "");
    }
    return 0;
}
