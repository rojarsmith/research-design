/*
 * latency.c - Stage 06 companion for Linux: a thread that wakes 60 times a
 * second from a timerfd, while other threads keep every CPU busy. It prints
 * how late each wakeup was as a text histogram.
 *
 * Build:  gcc -O2 -Wall -pthread -o latency latency.c
 * Run:    ./latency other 0          default policy, idle machine
 *         ./latency other 16         default policy, 16 CPU-bound threads
 *         ./latency fifo 16          SCHED_FIFO priority 50, same load
 *         ./latency fifo 16 10 80    10 s, SCHED_FIFO priority 80
 *
 * Arguments: policy (other or fifo), load threads (default: none),
 * seconds (default 5), FIFO priority (default 50). The load threads run at
 * the default policy and spin until the measurement ends.
 *
 * SCHED_FIFO needs CAP_SYS_NICE or a high enough RLIMIT_RTPRIO
 * (sched(7)). Without either, sched_setscheduler fails with EPERM, the
 * program says so and exits with status 2.
 *
 * How the lateness is measured: the timer is armed with an absolute first
 * expiry and a period of 1/60 s on CLOCK_MONOTONIC. Expiry k is due at
 * start + k * period. After each read() the thread takes the time and
 * subtracts the due time of the latest expiry. read() returns how many
 * expirations happened since the last read; more than one means whole
 * periods were missed.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#define PERIOD_NS 16666667LL /* 1/60 s */

static volatile int stop;

static int64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void *load(void *arg)
{
    volatile uint64_t x = (uint64_t)(uintptr_t)arg;
    while (!stop)
        x = x * 6364136223846793005ull + 1;
    return NULL;
}

static int cmp64(const void *a, const void *b)
{
    int64_t x = *(const int64_t *)a, y = *(const int64_t *)b;
    return (x > y) - (x < y);
}

/* The same buckets as windows/latency.c, in microseconds. */
static const double edge_us[] = {50, 100, 200, 500, 1000, 2000, 4000, 8000, 16667};
#define NB (int)(sizeof edge_us / sizeof edge_us[0] + 1)

static void histogram(int64_t *lat, int n)
{
    static const char *label[NB] = {
        "       < 50 us", "  50 - 100 us", " 100 - 200 us", " 200 - 500 us", "0.5 - 1 ms   ",
        "  1 - 2 ms   ", "  2 - 4 ms   ", "  4 - 8 ms   ", "8 - 16.7 ms  ", "    > 16.7 ms",
    };
    int count[NB] = {0};
    for (int i = 0; i < n; i++) {
        double us = (double)lat[i] / 1e3;
        int b = 0;
        while (b < NB - 1 && us >= edge_us[b])
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

int main(int argc, char **argv)
{
    const char *policy = argc > 1 ? argv[1] : "other";
    int nload = argc > 2 ? atoi(argv[2]) : 0;
    double seconds = argc > 3 ? atof(argv[3]) : 5.0;
    int fifo_prio = argc > 4 ? atoi(argv[4]) : 50;
    int n = (int)(seconds * 60.0);
    if (n < 1)
        n = 1;
    setvbuf(stdout, NULL, _IOLBF, 0);

    if (strcmp(policy, "fifo") == 0) {
        struct sched_param sp = {.sched_priority = fifo_prio};
        if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) {
            printf("SCHED_FIFO priority %d refused: %s\n", fifo_prio, strerror(errno));
            return 2;
        }
    } else if (strcmp(policy, "other") != 0) {
        fprintf(stderr, "policy must be other or fifo\n");
        return 1;
    }
    /* sched_setscheduler(0, ...) set the policy of this thread only, so the
     * load threads created below keep the default policy: new threads
     * inherit the creator's policy by default, so set them explicitly. */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
    struct sched_param zero = {.sched_priority = 0};
    pthread_attr_setschedparam(&attr, &zero);
    pthread_t *th = calloc((size_t)(nload > 0 ? nload : 1), sizeof *th);
    for (int i = 0; i < nload; i++)
        if (pthread_create(&th[i], &attr, load, (void *)(uintptr_t)(i + 1)) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            return 1;
        }

    int cur = sched_getscheduler(0);
    char pol[32];
    if (cur == SCHED_FIFO)
        snprintf(pol, sizeof pol, "SCHED_FIFO %d", fifo_prio);
    else
        snprintf(pol, sizeof pol, "%s", cur == SCHED_OTHER ? "SCHED_OTHER" : "?");
    printf("policy %s, %d load thread(s), %d wakeups at 60 Hz, %ld online CPUs\n", pol, nload,
           n, sysconf(_SC_NPROCESSORS_ONLN));

    int fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd < 0) {
        perror("timerfd_create");
        return 1;
    }
    usleep(100000); /* let the load threads start */
    int64_t start = now_ns() + PERIOD_NS;
    struct itimerspec its = {
        .it_interval = {0, PERIOD_NS},
        .it_value = {start / 1000000000LL, start % 1000000000LL},
    };
    if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &its, NULL) != 0) {
        perror("timerfd_settime");
        return 1;
    }

    int64_t *lat = calloc((size_t)n, sizeof *lat);
    uint64_t k = 0, missed = 0;
    for (int i = 0; i < n; i++) {
        uint64_t exp;
        if (read(fd, &exp, sizeof exp) != sizeof exp) {
            perror("read timerfd");
            return 1;
        }
        int64_t t = now_ns();
        k += exp;
        missed += exp - 1;
        lat[i] = t - (start + (int64_t)(k - 1) * PERIOD_NS);
    }
    stop = 1;
    for (int i = 0; i < nload; i++)
        pthread_join(th[i], NULL);

    histogram(lat, n);
    qsort(lat, (size_t)n, sizeof *lat, cmp64);
    double sum = 0;
    for (int i = 0; i < n; i++)
        sum += (double)lat[i];
    printf("  min %.1f us, median %.1f us, mean %.1f us, p99 %.1f us, max %.1f us\n",
           lat[0] / 1e3, lat[n / 2] / 1e3, sum / n / 1e3, lat[(n * 99) / 100] / 1e3,
           lat[n - 1] / 1e3);
    int over[4] = {0};
    const double lim_ms[4] = {1, 2, 4, 8};
    for (int i = 0; i < n; i++)
        for (int j = 0; j < 4; j++)
            if (lat[i] / 1e6 > lim_ms[j])
                over[j]++;
    printf("  later than 1 ms: %d, 2 ms: %d, 4 ms: %d, 8 ms: %d; whole periods missed: %llu\n",
           over[0], over[1], over[2], over[3], (unsigned long long)missed);
    return 0;
}
