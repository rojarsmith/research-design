/*
 * forkcow.c - Stage 03, exercise D: Linux's copy-on-write fork, seen
 * through the child's own page-fault counters.
 *
 * Build:  gcc -O2 -Wall -o forkcow forkcow.c
 * Run:    ./forkcow
 *
 * Part 1: a parent writes every page of a 64 MiB anonymous mapping, then
 * forks. The child reads every page, then writes 1000 of them, and
 * reports its minor faults (ru_minflt) after each step. fork(2) says the
 * child's resource usage starts at zero, and getrusage(2) counts page
 * faults serviced without I/O as minor faults.
 *
 * Part 2: how long fork + _exit + wait takes as the parent's memory grows.
 *
 * Transparent huge pages would let one fault cover 2 MiB, so the mapping
 * asks for MADV_NOHUGEPAGE.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define PAGE 4096L

static long minflt(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_minflt;
}

static double now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

static char *touched_region(long bytes)
{
    char *p = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    madvise(p, bytes, MADV_NOHUGEPAGE);
    for (long i = 0; i < bytes; i += PAGE)
        p[i] = 1;
    return p;
}

static void part1(void)
{
    const long bytes = 64L << 20, pages = bytes / PAGE;
    char *p = touched_region(bytes);
    printf("1. fork with %ld written pages (64 MiB) in the parent\n", pages);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        long f0 = minflt();
        volatile long sum = 0;
        for (long i = 0; i < bytes; i += PAGE)
            sum += p[i];
        long f1 = minflt();
        for (long i = 0; i < 1000; i++)
            p[i * PAGE] = 2;
        long f2 = minflt();
        printf("  child: minor faults %ld at start, +%ld after reading %ld pages, "
               "+%ld after writing 1000 pages\n",
               f0, f1 - f0, pages, f2 - f1);
        _exit(0);
    }
    waitpid(pid, NULL, 0);
    munmap(p, bytes);
}

static void part2(void)
{
    printf("\n2. fork + _exit + wait, by size of the parent's written memory\n");
    long sizes[] = {1L << 20, 64L << 20, 256L << 20};
    for (int s = 0; s < 3; s++) {
        char *p = touched_region(sizes[s]);
        double best = 1e30;
        for (int r = 0; r < 20; r++) {
            double t0 = now_us();
            pid_t pid = fork();
            if (pid == 0)
                _exit(0);
            waitpid(pid, NULL, 0);
            double t = now_us() - t0;
            if (t < best)
                best = t;
        }
        printf("  %4ld MiB: best of 20 runs %8.1f us\n", sizes[s] >> 20, best);
        munmap(p, sizes[s]);
    }
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    part1();
    part2();
    return 0;
}
