/*
 * procthread.c - Stage 02 companion for Linux (WSL 2 or a Linux VM), the
 * counterpart of windows/nt_procthread.c: what a process costs, what a
 * thread costs, and what a switch costs when the kernel does it
 * (semaphores between two threads) and when user code does it
 * (swapcontext).
 *
 * NOT compiled or run on the machine where this lab was written: it had no
 * Linux distribution or Linux toolchain. Every number in the lab README
 * for Linux is a prediction for you to replace.
 *
 * Build:  gcc -O2 -Wall -pthread -o procthread procthread.c
 * Run:    ./procthread              (all parts)
 *         ./procthread identity     (part 1 only; use this one under strace -f)
 *         ./procthread switch       (parts 3 and 4 only)
 *         strace -f -e trace=clone,clone3,execve,wait4 ./procthread identity
 *         strace -c -f ./procthread switch > /dev/null
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

extern char **environ;

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static long tid(void) { return syscall(SYS_gettid); }

/* ---- 1. identity ----------------------------------------------------- */

static void *identity_thread(void *arg)
{
    (void)arg;
    printf("  created thread: pid %d, tid %ld\n", (int)getpid(), tid());
    return NULL;
}

static void part_identity(void)
{
    printf("1. Identity\n");
    printf("  main thread:    pid %d, tid %ld\n", (int)getpid(), tid());
    pthread_t t;
    pthread_create(&t, NULL, identity_thread, NULL);
    pthread_join(t, NULL);
    pid_t pid = fork();
    if (pid == 0) {
        printf("  child process:  pid %d, tid %ld, parent %d\n", (int)getpid(), tid(),
               (int)getppid());
        _exit(0);
    }
    waitpid(pid, NULL, 0);
    printf("\n");
}

/* ---- 2. creation cost ------------------------------------------------ */

static void *empty_thread(void *arg) { return arg; }

static void part_creation(int nproc, int nthread)
{
    char *argv_true[] = {"/bin/true", NULL};
    double t0, t1;
    printf("2. Creation cost (create, wait for exit)\n");

    t0 = now_s();
    for (int i = 0; i < nproc; i++) {
        pid_t p = fork();
        if (p == 0)
            _exit(0);
        waitpid(p, NULL, 0);
    }
    t1 = now_s();
    printf("  fork + _exit + wait:            %8.1f us each  (%d runs)\n",
           (t1 - t0) * 1e6 / nproc, nproc);

    t0 = now_s();
    for (int i = 0; i < nproc; i++) {
        pid_t p = fork();
        if (p == 0) {
            execv(argv_true[0], argv_true);
            _exit(127);
        }
        waitpid(p, NULL, 0);
    }
    t1 = now_s();
    printf("  fork + exec /bin/true + wait:   %8.1f us each  (%d runs)\n",
           (t1 - t0) * 1e6 / nproc, nproc);

    t0 = now_s();
    for (int i = 0; i < nproc; i++) {
        pid_t p;
        if (posix_spawn(&p, argv_true[0], NULL, NULL, argv_true, environ) != 0) {
            printf("  posix_spawn failed\n");
            break;
        }
        waitpid(p, NULL, 0);
    }
    t1 = now_s();
    printf("  posix_spawn /bin/true + wait:   %8.1f us each  (%d runs)\n",
           (t1 - t0) * 1e6 / nproc, nproc);

    t0 = now_s();
    for (int i = 0; i < nthread; i++) {
        pthread_t t;
        pthread_create(&t, NULL, empty_thread, NULL);
        pthread_join(t, NULL);
    }
    t1 = now_s();
    printf("  pthread_create + join:          %8.1f us each  (%d runs)\n\n",
           (t1 - t0) * 1e6 / nthread, nthread);
}

/* ---- 3. kernel-scheduled switch: two threads, two semaphores ---------- */

static sem_t s_ping, s_pong;
static long g_rounds;

static void *pong_thread(void *arg)
{
    (void)arg;
    for (long i = 0; i < g_rounds; i++) {
        sem_wait(&s_ping);
        sem_post(&s_pong);
    }
    return NULL;
}

/* cpu >= 0 pins both threads to that CPU; cpu < 0 leaves them free. */
static double ping_pong(long rounds, int cpu)
{
    cpu_set_t old, one;
    pthread_attr_t attr;
    pthread_t t;

    sem_init(&s_ping, 0, 0);
    sem_init(&s_pong, 0, 0);
    g_rounds = rounds;
    pthread_attr_init(&attr);
    sched_getaffinity(0, sizeof old, &old);
    if (cpu >= 0) {
        CPU_ZERO(&one);
        CPU_SET(cpu, &one);
        pthread_attr_setaffinity_np(&attr, sizeof one, &one);
        sched_setaffinity(0, sizeof one, &one);
    }
    pthread_create(&t, &attr, pong_thread, NULL);

    double t0 = now_s();
    for (long i = 0; i < rounds; i++) {
        sem_post(&s_ping);
        sem_wait(&s_pong);
    }
    double t1 = now_s();

    pthread_join(t, NULL);
    sched_setaffinity(0, sizeof old, &old);
    pthread_attr_destroy(&attr);
    sem_destroy(&s_ping);
    sem_destroy(&s_pong);
    return (t1 - t0) * 1e9 / (2.0 * (double)rounds); /* one round = two hand-offs */
}

/* ---- 4. user-scheduled switch: swapcontext --------------------------- */

static ucontext_t uc_main, uc_other;
static long g_n;
static volatile long g_switches;

static void other_context(void)
{
    for (long i = 0; i < g_n; i++) {
        g_switches++;
        swapcontext(&uc_other, &uc_main);
    }
}

static double ucontext_ping_pong(long n)
{
    static char stack[64 * 1024];
    g_n = n;
    g_switches = 0;
    getcontext(&uc_other);
    uc_other.uc_stack.ss_sp = stack;
    uc_other.uc_stack.ss_size = sizeof stack;
    uc_other.uc_link = &uc_main;
    makecontext(&uc_other, other_context, 0);

    double t0 = now_s();
    for (long i = 0; i < n; i++) {
        g_switches++;
        swapcontext(&uc_main, &uc_other);
    }
    double t1 = now_s();
    return (t1 - t0) * 1e9 / (double)g_switches;
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "all";
    setvbuf(stdout, NULL, _IONBF, 0); /* no buffered output duplicated by fork */
    printf("Online CPUs: %ld\n\n", sysconf(_SC_NPROCESSORS_ONLN));

    if (strcmp(mode, "all") == 0 || strcmp(mode, "identity") == 0)
        part_identity();
    if (strcmp(mode, "all") == 0)
        part_creation(200, 5000);
    if (strcmp(mode, "all") == 0 || strcmp(mode, "switch") == 0) {
        printf("3. Switch between two kernel threads (semaphore ping-pong, ns per hand-off)\n");
        long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
        printf("  same CPU:                       %8.0f ns\n", ping_pong(200000, ncpu > 2 ? 2 : 0));
        printf("  unpinned:                       %8.0f ns\n\n", ping_pong(200000, -1));
        printf("4. Switch between two contexts on one thread (swapcontext)\n");
        printf("  swapcontext:                    %8.1f ns\n", ucontext_ping_pong(2000000));
    }
    return 0;
}
