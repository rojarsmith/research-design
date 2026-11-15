/*
 * boundary.c - Stage 01 companion for Linux (WSL 2 or a Linux VM, x86-64).
 *
 * NOT compiled or run on the machine where this lab was written: it had no
 * Linux distribution or Linux toolchain. Treat every expectation in the lab
 * README as a prediction until you have run this yourself.
 *
 * Build:  gcc -O2 -Wall -o boundary boundary.c
 * Run:    ./boundary
 *         strace -o boundary.trace ./boundary     (then read boundary.trace)
 *         strace -c ./boundary > /dev/null        (count system calls)
 *
 * 1. The same write(2) three ways: the libc wrapper, libc's generic
 *    syscall(2), and a raw `syscall` instruction.
 * 2. write(2) with a kernel-half address and with NULL.
 * 3. Cost per call: a plain function, getpid(2) through syscall(2), and
 *    clock_gettime(CLOCK_MONOTONIC), which the vDSO serves without
 *    entering the kernel on most x86-64 systems.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

static volatile long sink;

__attribute__((noinline)) static long plain_function(long x)
{
    __asm__ volatile("" ::: "memory");
    return x + 1;
}

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static void time_it(const char *name, int which, long n)
{
    struct timespec ts;
    double t0 = now_ns();
    for (long i = 0; i < n; i++) {
        switch (which) {
        case 0: sink = plain_function(sink); break;
        case 1: sink += syscall(SYS_getpid); break;
        case 2: clock_gettime(CLOCK_MONOTONIC, &ts); sink += ts.tv_nsec; break;
        }
    }
    double t1 = now_ns();
    printf("  %-34s %8.1f ns/call  (%ld calls)\n", name, (t1 - t0) / (double)n, n);
}

int main(int argc, char **argv)
{
    long n = argc > 1 ? atol(argv[1]) : 1000000;
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("1. One write, three ways\n");
    static const char m1[] = "  write() through the libc wrapper\n";
    static const char m2[] = "  syscall(SYS_write, ...) through libc\n";
    if (write(1, m1, sizeof m1 - 1) < 0)
        perror("write");
    if (syscall(SYS_write, 1, m2, sizeof m2 - 1) < 0)
        perror("syscall");
#if defined(__x86_64__)
    static const char m3[] = "  a raw syscall instruction\n";
    long ret;
    /* x86-64 Linux: number in rax, arguments in rdi, rsi, rdx;
     * the instruction itself overwrites rcx and r11. */
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"((long)SYS_write), "D"(1L), "S"(m3), "d"(sizeof m3 - 1)
                     : "rcx", "r11", "memory");
    (void)ret;
#else
    printf("  (raw syscall example is x86-64 only)\n");
#endif

    printf("\n2. Bad pointers\n");
    void *kernel_ptr = (void *)(uintptr_t)0xffff800000000000ull;
    errno = 0;
    ssize_t r = write(1, kernel_ptr, 8);
    printf("  write(1, kernel address, 8) = %zd, errno %d (%s)\n", r, errno, strerror(errno));
    errno = 0;
    r = write(1, NULL, 8);
    printf("  write(1, NULL, 8)           = %zd, errno %d (%s)\n", r, errno, strerror(errno));

    printf("\n3. Cost per call\n");
    time_it("plain function call", 0, n);
    time_it("syscall(SYS_getpid)", 1, n);
    time_it("clock_gettime(CLOCK_MONOTONIC)", 2, n);
    return 0;
}
