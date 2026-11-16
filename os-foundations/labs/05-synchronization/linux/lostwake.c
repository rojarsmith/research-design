/*
 * lostwake.c - Stage 05 companion for Linux: lost wakeups with a condition
 * variable and with a raw futex, and the designs that cannot lose one.
 * The Linux counterpart of windows/lostwake.c, with the same four modes.
 *
 * Build:  gcc -O2 -Wall -pthread -o lostwake lostwake.c
 * Run:    ./lostwake              (20000 rounds per mode, 5 us window)
 *         ./lostwake 50000 0      (no extra window)
 *
 * Two threads play ping-pong on `turn`. Each waits until turn names it,
 * hands the turn over and notifies. Every wait has a 50 ms timeout; a wait
 * that times out while turn already names the waiter is a lost wakeup.
 * A mode stops after 20 of them.
 *
 * Modes:
 *   1. pthread_cond, BROKEN: test turn before taking the mutex.
 *   2. pthread_cond, correct: test turn while holding the mutex;
 *      pthread_cond_wait releases it only once the thread is waiting.
 *   3. futex, correct: FUTEX_WAIT with the value seen. The kernel checks
 *      that turn still holds that value before it puts the thread to sleep.
 *   4. futex, BROKEN: FUTEX_WAKE first, publish the new turn afterwards.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define TIMEOUT_MS 50
#define MAX_LOST 20

static int turn;
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv;
static int lost, timeouts, stop;
static int mode, rounds, done_rounds[2];
static double window_us;

static double now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

static void race_window(void)
{
    double t0 = now_us();
    while (now_us() - t0 < window_us)
        ;
}

static int load(int *p) { return __atomic_load_n(p, __ATOMIC_SEQ_CST); }

static void timed_out(int me)
{
    __atomic_fetch_add(&timeouts, 1, __ATOMIC_RELAXED);
    if (load(&turn) == me && __atomic_add_fetch(&lost, 1, __ATOMIC_RELAXED) >= MAX_LOST)
        __atomic_store_n(&stop, 1, __ATOMIC_SEQ_CST);
}

static int cond_wait_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); /* cv uses CLOCK_MONOTONIC, see main */
    ts.tv_nsec += TIMEOUT_MS * 1000000L;
    ts.tv_sec += ts.tv_nsec / 1000000000L;
    ts.tv_nsec %= 1000000000L;
    return pthread_cond_timedwait(&cv, &mu, &ts);
}

static void wait_turn(int me)
{
    switch (mode) {
    case 1: /* BROKEN: test outside the mutex */
        while (load(&turn) != me && !load(&stop)) {
            race_window();
            pthread_mutex_lock(&mu);
            if (cond_wait_ms() == ETIMEDOUT)
                timed_out(me);
            pthread_mutex_unlock(&mu);
        }
        break;
    case 2: /* correct: test and wait under the mutex */
        pthread_mutex_lock(&mu);
        while (load(&turn) != me && !load(&stop)) {
            race_window();
            if (cond_wait_ms() == ETIMEDOUT)
                timed_out(me);
        }
        pthread_mutex_unlock(&mu);
        break;
    case 3:
    case 4: {
        int seen;
        while ((seen = load(&turn)) != me && !load(&stop)) {
            race_window();
            struct timespec rel = {0, TIMEOUT_MS * 1000000L};
            /* Sleeps only if turn still equals seen; otherwise EAGAIN. */
            if (syscall(SYS_futex, &turn, FUTEX_WAIT_PRIVATE, seen, &rel, NULL, 0) == -1 &&
                errno == ETIMEDOUT)
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
        pthread_mutex_lock(&mu);
        __atomic_store_n(&turn, other, __ATOMIC_SEQ_CST);
        pthread_mutex_unlock(&mu);
        pthread_cond_broadcast(&cv);
        break;
    case 3:
        __atomic_store_n(&turn, other, __ATOMIC_SEQ_CST);
        syscall(SYS_futex, &turn, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
        break;
    case 4: /* BROKEN: wake first, publish later */
        syscall(SYS_futex, &turn, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
        race_window();
        __atomic_store_n(&turn, other, __ATOMIC_SEQ_CST);
        break;
    }
}

static void *player(void *arg)
{
    int me = (int)(long)arg;
    for (int r = 0; r < rounds && !load(&stop); r++) {
        wait_turn(me);
        if (load(&stop))
            break;
        give_turn(1 - me);
        done_rounds[me] = r + 1;
    }
    __atomic_store_n(&stop, 1, __ATOMIC_SEQ_CST);
    return NULL;
}

int main(int argc, char **argv)
{
    rounds = argc > 1 ? atoi(argv[1]) : 20000;
    window_us = argc > 2 ? atof(argv[2]) : 5.0;
    setvbuf(stdout, NULL, _IONBF, 0);

    pthread_condattr_t ca;
    pthread_condattr_init(&ca);
    pthread_condattr_setclock(&ca, CLOCK_MONOTONIC);
    pthread_cond_init(&cv, &ca);

    static const char *names[] = {
        "",
        "1. pthread_cond, test outside the mutex (BROKEN)",
        "2. pthread_cond, test under the mutex",
        "3. futex wait with the value seen",
        "4. futex, wake before publish (BROKEN)",
    };
    printf("%d rounds per mode, %.1f us race window, %d ms wait timeout, "
           "stop after %d lost wakeups\n\n",
           rounds, window_us, TIMEOUT_MS, MAX_LOST);
    printf("  %-50s %8s %9s %8s %10s\n", "mode", "rounds", "timeouts", "lost", "elapsed");
    for (mode = 1; mode <= 4; mode++) {
        turn = 0;
        lost = timeouts = stop = 0;
        done_rounds[0] = done_rounds[1] = 0;
        double t0 = now_us();
        pthread_t a, b;
        pthread_create(&a, NULL, player, (void *)0L);
        pthread_create(&b, NULL, player, (void *)1L);
        pthread_join(a, NULL);
        pthread_join(b, NULL);
        printf("  %-50s %8d %9d %8d %8.0f ms%s\n", names[mode], done_rounds[0], timeouts, lost,
               (now_us() - t0) / 1000.0, lost >= MAX_LOST ? "  (stopped early)" : "");
    }
    return 0;
}
