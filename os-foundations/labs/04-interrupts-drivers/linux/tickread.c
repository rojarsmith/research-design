/*
 * tickread.c - Stage 04, exercise D: read tick events from /dev/tickdev.
 *
 * NOT run on the machine where this lab was written (the module could not
 * be loaded there). It was compiled in an Ubuntu 24.04 container to check
 * that it builds; see the lab README.
 *
 * Build:  make reader          (or: gcc -O2 -Wall -o tickread tickread.c)
 * Run:    ./tickread [-n count] [-s ms] [-N] [-p] [device]
 *   -n count  events to read (default 10)
 *   -s ms     sleep this long after each read: a slow reader misses ticks
 *   -N        open with O_NONBLOCK and show what read() does between ticks
 *   -p        wait in poll() before each read
 *
 * For each event it prints the sequence number, the kernel's timestamp,
 * the time since the previous tick, how many ticks this reader missed,
 * and how long after the tick this process got to run (wake-up latency).
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "tickdev.h"

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

int main(int argc, char **argv)
{
    int count = 10, sleep_ms = 0, nonblock = 0, use_poll = 0, opt;
    const char *dev = "/dev/tickdev";

    while ((opt = getopt(argc, argv, "n:s:Np")) != -1) {
        switch (opt) {
        case 'n': count = atoi(optarg); break;
        case 's': sleep_ms = atoi(optarg); break;
        case 'N': nonblock = 1; break;
        case 'p': use_poll = 1; break;
        default:
            fprintf(stderr, "usage: %s [-n count] [-s ms] [-N] [-p] [device]\n", argv[0]);
            return 2;
        }
    }
    if (optind < argc)
        dev = argv[optind];

    int fd = open(dev, O_RDONLY | (nonblock ? O_NONBLOCK : 0));
    if (fd < 0) {
        perror(dev);
        return 1;
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("pid %d reading %s%s%s\n", getpid(), dev, nonblock ? " (O_NONBLOCK)" : "",
           use_poll ? " with poll()" : "");
    printf("%8s %16s %10s %7s %12s\n", "seq", "timestamp ns", "dt us", "missed",
           "latency us");

    struct tick_event prev = {0, 0};
    int got = 0, eagain = 0;
    while (got < count) {
        if (use_poll) {
            struct pollfd p = {.fd = fd, .events = POLLIN};
            if (poll(&p, 1, -1) < 0) {
                perror("poll");
                return 1;
            }
        }
        struct tick_event ev;
        ssize_t n = read(fd, &ev, sizeof ev);
        uint64_t t = now_ns();
        if (n < 0 && errno == EAGAIN) {
            eagain++;
            usleep(1000);
            continue;
        }
        if (n != (ssize_t)sizeof ev) {
            fprintf(stderr, "read returned %zd: %s\n", n, n < 0 ? strerror(errno) : "short");
            return 1;
        }
        if (got == 0)
            printf("%8llu %16llu %10s %7s %12.1f\n", (unsigned long long)ev.seq,
                   (unsigned long long)ev.timestamp_ns, "-", "-",
                   (t - ev.timestamp_ns) / 1e3);
        else
            printf("%8llu %16llu %10.1f %7llu %12.1f\n", (unsigned long long)ev.seq,
                   (unsigned long long)ev.timestamp_ns,
                   (ev.timestamp_ns - prev.timestamp_ns) / 1e3,
                   (unsigned long long)(ev.seq - prev.seq - 1), (t - ev.timestamp_ns) / 1e3);
        prev = ev;
        got++;
        if (sleep_ms)
            usleep(sleep_ms * 1000);
    }
    if (nonblock)
        printf("read() returned EAGAIN %d times between ticks\n", eagain);
    close(fd);
    return 0;
}
