/*
 * futex_mutex.c - Stage 05 companion for Linux: a mutex built on futex(2),
 * compared with pthread_mutex under contention. The Linux counterpart of
 * windows/lockbench.c, with the same workload.
 *
 * Build:  gcc -O2 -Wall -pthread -o futex_mutex futex_mutex.c
 * Run:    ./futex_mutex                    (every lock, 0.25 s per cell)
 *         ./futex_mutex 1.0                (1 s per cell)
 *         ./futex_mutex 0.5 futex 8        (one lock, one thread count)
 *         strace -f -c -e trace=futex ./futex_mutex 0.5 pthread 8
 *
 * Every thread loops: take the lock, run a short critical section on
 * shared data, release, then run a longer stretch of private work. The
 * critical section is about a fifth of each iteration.
 *
 * Locks compared:
 *   pthread        pthread_mutex_t with default attributes
 *   adaptive       PTHREAD_MUTEX_ADAPTIVE_NP: glibc spins a little first
 *   futex          the three-state mutex from Ulrich Drepper's paper
 *                  "Futexes Are Tricky": 0 free, 1 held, 2 held and maybe
 *                  waiters. FUTEX_WAIT sleeps only if the word is still 2.
 *   futex + spin   the same, retrying for a while before it waits
 *   spinlock       test-and-test-and-set, never sleeps
 *
 * For each cell it prints the throughput, the spread between the busiest
 * and the least busy thread, the process's system and user CPU time and
 * its voluntary and involuntary context switches during the cell
 * (getrusage), and for the futex locks how many FUTEX_WAIT and FUTEX_WAKE
 * calls they made.
 */
#define _GNU_SOURCE
#include <linux/futex.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define MAXT 64
#define CACHE 64

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static double tv_ms(struct timeval tv) { return tv.tv_sec * 1e3 + tv.tv_usec / 1e3; }

static inline void cpu_relax(void)
{
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#endif
}

enum kind { K_PTHREAD, K_ADAPTIVE, K_FUTEX, K_FUTEXSPIN, K_SPIN, K_COUNT };
static const char *kind_name[K_COUNT] = {"pthread", "adaptive", "futex", "futex + spin",
                                         "spinlock"};

struct shard {
    union {
        pthread_mutex_t mu;
        int word; /* futex mutex and spinlock */
    } u;
    char pad1[CACHE];
    volatile uint64_t counter;
    volatile uint64_t state;
    char pad2[CACHE];
};

struct worker {
    struct shard *sh;
    enum kind kind;
    long long ops, waits, wakes;
    char pad[CACHE];
};

static volatile int g_go, g_stop;
static int g_cswork = 40, g_outwork = 160;

static long futex(int *uaddr, int op, int val)
{
    return syscall(SYS_futex, uaddr, op, val, NULL, NULL, 0);
}

static void futex_lock(struct worker *w, int *m, int spin)
{
    int c = 0;
    if (__atomic_compare_exchange_n(m, &c, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return;
    for (int i = 0; i < spin; i++) {
        cpu_relax();
        c = 0;
        if (__atomic_load_n(m, __ATOMIC_RELAXED) == 0 &&
            __atomic_compare_exchange_n(m, &c, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
            return;
    }
    if (c != 2)
        c = __atomic_exchange_n(m, 2, __ATOMIC_ACQUIRE);
    while (c != 0) {
        /* The kernel compares *m with 2 under its own lock before sleeping. */
        futex(m, FUTEX_WAIT_PRIVATE, 2);
        w->waits++;
        c = __atomic_exchange_n(m, 2, __ATOMIC_ACQUIRE);
    }
}

static void futex_unlock(struct worker *w, int *m)
{
    if (__atomic_exchange_n(m, 0, __ATOMIC_RELEASE) == 2) {
        futex(m, FUTEX_WAKE_PRIVATE, 1);
        w->wakes++;
    }
}

static void spin_lock(int *m)
{
    for (;;) {
        if (__atomic_exchange_n(m, 1, __ATOMIC_ACQUIRE) == 0)
            return;
        while (__atomic_load_n(m, __ATOMIC_RELAXED) != 0)
            cpu_relax();
    }
}

static void lock(struct worker *w)
{
    struct shard *s = w->sh;
    switch (w->kind) {
    case K_PTHREAD:
    case K_ADAPTIVE: pthread_mutex_lock(&s->u.mu); break;
    case K_FUTEX: futex_lock(w, &s->u.word, 0); break;
    case K_FUTEXSPIN: futex_lock(w, &s->u.word, 50); break;
    case K_SPIN: spin_lock(&s->u.word); break;
    default: break;
    }
}

static void unlock(struct worker *w)
{
    struct shard *s = w->sh;
    switch (w->kind) {
    case K_PTHREAD:
    case K_ADAPTIVE: pthread_mutex_unlock(&s->u.mu); break;
    case K_FUTEX:
    case K_FUTEXSPIN: futex_unlock(w, &s->u.word); break;
    case K_SPIN: __atomic_store_n(&s->u.word, 0, __ATOMIC_RELEASE); break;
    default: break;
    }
}

static void *worker_main(void *arg)
{
    struct worker *w = arg;
    uint64_t mine = (uint64_t)(uintptr_t)w;
    while (!__atomic_load_n(&g_go, __ATOMIC_ACQUIRE))
        cpu_relax();
    while (!__atomic_load_n(&g_stop, __ATOMIC_RELAXED)) {
        lock(w);
        uint64_t x = w->sh->state;
        for (int i = 0; i < g_cswork; i++)
            x = x * 6364136223846793005ull + 1442695040888963407ull;
        w->sh->state = x;
        w->sh->counter++;
        unlock(w);
        for (int i = 0; i < g_outwork; i++)
            mine = mine * 6364136223846793005ull + 1;
        w->ops++;
    }
    return (void *)(uintptr_t)mine;
}

static void shard_init(struct shard *s, enum kind k)
{
    memset(s, 0, sizeof *s);
    if (k == K_PTHREAD) {
        pthread_mutex_init(&s->u.mu, NULL);
    } else if (k == K_ADAPTIVE) {
        pthread_mutexattr_t a;
        pthread_mutexattr_init(&a);
        pthread_mutexattr_settype(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
        pthread_mutex_init(&s->u.mu, &a);
        pthread_mutexattr_destroy(&a);
    }
}

struct result {
    double mops, ns_per_op, fair, sys_ms, user_ms, waits_k, wakes_k;
    long vcsw, ivcsw;
    int ok;
};

static struct result run_cell(enum kind k, int nthreads, int nshards, double seconds)
{
    static struct shard shards[MAXT];
    static struct worker workers[MAXT];
    pthread_t th[MAXT];
    struct result r = {0};
    struct rusage ru0, ru1;

    for (int i = 0; i < nshards; i++)
        shard_init(&shards[i], k);
    for (int i = 0; i < nthreads; i++) {
        memset(&workers[i], 0, sizeof workers[i]);
        workers[i].sh = &shards[i % nshards];
        workers[i].kind = k;
    }
    g_go = 0;
    g_stop = 0;
    for (int i = 0; i < nthreads; i++)
        pthread_create(&th[i], NULL, worker_main, &workers[i]);
    usleep(20000);
    getrusage(RUSAGE_SELF, &ru0);
    double t0 = now_s();
    __atomic_store_n(&g_go, 1, __ATOMIC_RELEASE);
    usleep((useconds_t)(seconds * 1e6));
    __atomic_store_n(&g_stop, 1, __ATOMIC_RELAXED);
    double t1 = now_s();
    for (int i = 0; i < nthreads; i++)
        pthread_join(th[i], NULL);
    getrusage(RUSAGE_SELF, &ru1);

    long long total = 0, mn = -1, mx = 0, waits = 0, wakes = 0;
    for (int i = 0; i < nthreads; i++) {
        long long o = workers[i].ops;
        total += o;
        if (mn < 0 || o < mn)
            mn = o;
        if (o > mx)
            mx = o;
        waits += workers[i].waits;
        wakes += workers[i].wakes;
    }
    uint64_t counted = 0;
    for (int i = 0; i < nshards; i++) {
        counted += shards[i].counter;
        if (k == K_PTHREAD || k == K_ADAPTIVE)
            pthread_mutex_destroy(&shards[i].u.mu);
    }
    double el = t1 - t0;
    r.mops = (double)total / el / 1e6;
    r.ns_per_op = total ? el * 1e9 / (double)total : 0;
    r.fair = mx ? (double)mn / (double)mx : 0;
    r.sys_ms = tv_ms(ru1.ru_stime) - tv_ms(ru0.ru_stime);
    r.user_ms = tv_ms(ru1.ru_utime) - tv_ms(ru0.ru_utime);
    r.vcsw = ru1.ru_nvcsw - ru0.ru_nvcsw;
    r.ivcsw = ru1.ru_nivcsw - ru0.ru_nivcsw;
    r.waits_k = total ? 1000.0 * (double)waits / (double)total : 0;
    r.wakes_k = total ? 1000.0 * (double)wakes / (double)total : 0;
    r.ok = counted == (uint64_t)total;
    return r;
}

static void print_header(void)
{
    printf("  %-13s %7s %8s %8s %7s %8s %8s %9s %8s %9s %9s\n", "lock", "threads", "Mops/s",
           "ns/op", "min/max", "sys ms", "user ms", "vol csw", "invol", "waits/1k", "wakes/1k");
}

static void print_row(enum kind k, int n, struct result r)
{
    printf("  %-13s %7d %8.2f %8.1f %7.2f %8.0f %8.0f %9ld %8ld", kind_name[k], n, r.mops,
           r.ns_per_op, r.fair, r.sys_ms, r.user_ms, r.vcsw, r.ivcsw);
    if (k == K_FUTEX || k == K_FUTEXSPIN)
        printf(" %9.1f %9.1f", r.waits_k, r.wakes_k);
    else
        printf(" %9s %9s", "-", "-");
    printf("%s\n", r.ok ? "" : "  COUNT MISMATCH");
}

int main(int argc, char **argv)
{
    double seconds = argc > 1 ? atof(argv[1]) : 0.25;
    if (seconds < 0.05)
        seconds = 0.05;
    setvbuf(stdout, NULL, _IONBF, 0);
    int ncpu = (int)sysconf(_SC_NPROCESSORS_ONLN);

    if (argc > 3) { /* one lock, one thread count: for strace -c */
        for (int k = 0; k < K_COUNT; k++)
            if (strcmp(argv[2], kind_name[k]) == 0 ||
                (strcmp(argv[2], "futex+spin") == 0 && k == K_FUTEXSPIN)) {
                int n = atoi(argv[3]);
                if (n < 1 || n > MAXT)
                    n = 1;
                print_header();
                print_row((enum kind)k, n, run_cell((enum kind)k, n, 1, seconds));
                return 0;
            }
        fprintf(stderr, "unknown lock %s\n", argv[2]);
        return 1;
    }

    printf("Online CPUs: %d. %.2f s per cell. Critical section %d LCG steps, private work "
           "%d steps.\n\n",
           ncpu, seconds, g_cswork, g_outwork);
    {
        uint64_t x = 1;
        double t0 = now_s();
        for (int j = 0; j < 1000000; j++) {
            for (int i = 0; i < g_cswork + g_outwork; i++)
                x = x * 6364136223846793005ull + 1;
            __asm__ volatile("" : "+r"(x));
        }
        printf("One iteration without a lock: %.1f ns (critical section about %.0f%% of it)\n\n",
               (now_s() - t0) * 1e9 / 1e6, 100.0 * g_cswork / (g_cswork + g_outwork));
    }

    int counts[] = {1, 2, 4, 8, ncpu, 2 * ncpu};
    int ncounts = sizeof counts / sizeof counts[0];
    printf("1. One lock shared by every thread\n");
    print_header();
    for (int k = 0; k < K_COUNT; k++) {
        for (int c = 0; c < ncounts; c++) {
            int n = counts[c];
            if (n > MAXT || (c > 0 && n <= counts[c - 1]))
                continue;
            print_row((enum kind)k, n, run_cell((enum kind)k, n, 1, seconds));
        }
        printf("\n");
    }

    printf("2. Splitting the lock: one lock per thread instead of one for all\n");
    print_header();
    int n = ncpu > MAXT ? MAXT : ncpu;
    enum kind split[] = {K_PTHREAD, K_FUTEX};
    for (int i = 0; i < 2; i++) {
        struct result one = run_cell(split[i], n, 1, seconds);
        struct result per = run_cell(split[i], n, n, seconds);
        printf("  one lock:\n");
        print_row(split[i], n, one);
        printf("  one lock per thread:\n");
        print_row(split[i], n, per);
    }
    return 0;
}
