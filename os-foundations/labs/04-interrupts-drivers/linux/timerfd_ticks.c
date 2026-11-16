/*
 * timerfd_ticks.c - Stage 04, exercise E: the user-space counterpart of
 * tickdev. A periodic timerfd wakes a blocked read() on every tick, and
 * read() returns how many ticks passed since the last read.
 *
 * Build:  gcc -O2 -Wall -pthread -o timerfd_ticks timerfd_ticks.c
 * Run:    ./timerfd_ticks [-n count] [-s ms] [-m ms]
 *   -n count  ticks to read (default 10)
 *   -s ms     sleep this long after each read, like tickread -s
 *   -m ms     period (default 100)
 *
 * It prints the same columns as tickread, plus the softirq counts of the
 * whole machine before and after (/proc/softirqs), and, while the main
 * thread is blocked in read(), that thread's state and wait channel as a
 * second thread sees them in /proc.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

static pid_t main_tid;

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

/* Sum of one row of /proc/softirqs over all CPUs, or -1. */
static long long softirq_total(const char *name)
{
    char line[4096];
    long long total = -1;
    FILE *f = fopen("/proc/softirqs", "r");
    if (!f)
        return -1;
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ')
            p++;
        size_t n = strlen(name);
        if (strncmp(p, name, n) == 0 && p[n] == ':') {
            total = 0;
            for (p += n + 1; *p;) {
                char *end;
                long long v = strtoll(p, &end, 10);
                if (end == p)
                    break;
                total += v;
                p = end;
            }
        }
    }
    fclose(f);
    return total;
}

static void read_file(const char *path, char *buf, size_t len)
{
    FILE *f = fopen(path, "r");
    buf[0] = 0;
    if (!f) {
        snprintf(buf, len, "(%s)", strerror(errno));
        return;
    }
    if (!fgets(buf, (int)len, f))
        snprintf(buf, len, "(empty)");
    buf[strcspn(buf, "\n")] = 0;
    fclose(f);
}

/* Look at the main thread from outside while it waits for a tick. */
static void *observer(void *arg)
{
    char path[64], stat[512], wchan[128];
    usleep(*(int *)arg * 1000 / 2); /* halfway into the first wait */
    snprintf(path, sizeof path, "/proc/self/task/%d/stat", main_tid);
    read_file(path, stat, sizeof stat);
    snprintf(path, sizeof path, "/proc/self/task/%d/wchan", main_tid);
    read_file(path, wchan, sizeof wchan);
    char *state = strrchr(stat, ')');
    printf("  [observer] main thread %d during read(): state '%c', wchan '%s'\n",
           main_tid, state && state[1] ? state[2] : '?', wchan);
    return NULL;
}

int main(int argc, char **argv)
{
    int count = 10, sleep_ms = 0, period_ms = 100, opt;
    while ((opt = getopt(argc, argv, "n:s:m:")) != -1) {
        switch (opt) {
        case 'n': count = atoi(optarg); break;
        case 's': sleep_ms = atoi(optarg); break;
        case 'm': period_ms = atoi(optarg); break;
        default:
            fprintf(stderr, "usage: %s [-n count] [-s ms] [-m ms]\n", argv[0]);
            return 2;
        }
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    main_tid = (pid_t)syscall(SYS_gettid);

    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (fd < 0) {
        perror("timerfd_create");
        return 1;
    }
    struct itimerspec its = {
        .it_interval = {period_ms / 1000, (period_ms % 1000) * 1000000L},
        .it_value = {period_ms / 1000, (period_ms % 1000) * 1000000L},
    };
    long long timer0 = softirq_total("TIMER"), hrtimer0 = softirq_total("HRTIMER");
    uint64_t start = now_ns();
    if (timerfd_settime(fd, 0, &its, NULL) < 0) {
        perror("timerfd_settime");
        return 1;
    }
    pthread_t th;
    pthread_create(&th, NULL, observer, &period_ms);

    printf("period %d ms, %d ticks, reader sleeps %d ms after each read\n", period_ms,
           count, sleep_ms);
    printf("%8s %16s %10s %7s %12s\n", "seq", "wake ns", "dt us", "missed",
           "latency us");
    uint64_t seq = 0, prev = 0;
    for (int i = 0; i < count; i++) {
        uint64_t expirations;
        if (read(fd, &expirations, sizeof expirations) != sizeof expirations) {
            perror("read");
            return 1;
        }
        uint64_t t = now_ns();
        seq += expirations;
        /* the deadline of the newest tick this read reported */
        uint64_t deadline = start + seq * (uint64_t)period_ms * 1000000u;
        if (i == 0)
            printf("%8llu %16llu %10s %7llu %12.1f\n", (unsigned long long)seq,
                   (unsigned long long)t, "-", (unsigned long long)(expirations - 1),
                   ((double)t - (double)deadline) / 1e3);
        else
            printf("%8llu %16llu %10.1f %7llu %12.1f\n", (unsigned long long)seq,
                   (unsigned long long)t, ((double)t - (double)prev) / 1e3,
                   (unsigned long long)(expirations - 1),
                   ((double)t - (double)deadline) / 1e3);
        prev = t;
        if (sleep_ms)
            usleep(sleep_ms * 1000);
    }
    pthread_join(th, NULL);
    long long timer1 = softirq_total("TIMER"), hrtimer1 = softirq_total("HRTIMER");
    printf("softirqs on all CPUs during the run: TIMER +%lld, HRTIMER +%lld\n",
           timer1 - timer0, hrtimer1 - hrtimer0);
    close(fd);
    return 0;
}
