/*
 * uthread_win64.c - user-level threads on Windows x64 with a hand-written
 * context switch (uswitch_win64.S). It has the shape asked for in the xv6
 * part of stage 02: a thread table, a switch in assembly, and a yield.
 *
 * Build:
 *     gcc -O2 -Wall -o uthread_win64.exe uthread_win64.c uswitch_win64.S
 *     gcc -O2 -Wall -DSKIP_XMM -o uthread_noxmm.exe uthread_win64.c uswitch_win64.S
 * Run:
 *     uthread_win64.exe
 *
 * Every thread keeps two running sums across its yields: one integer and
 * one double. At -O2 the compiler is free to keep them in callee-saved
 * registers (rbx, rsi, rdi, r12-r15, xmm6-xmm15) across the call to
 * thread_yield(), so the sums check whether uswitch really preserves
 * those registers. Build with -DSKIP_XMM to drop xmm6-xmm15 from the
 * switch and see what happens.
 *
 * What this does NOT do, compared with SwitchToFiber: it never tells
 * Windows about the new stacks. The thread environment block still
 * describes the original stack, so an exception raised on a uthread
 * stack cannot be dispatched, and nothing here is safe to use with
 * C++ exceptions, structured exception handling or stack-hungry APIs.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct uctx {
    uint64_t rsp, rbx, rbp, rdi, rsi, r12, r13, r14, r15, pad;
    unsigned char xmm[10][16]; /* xmm6 .. xmm15 */
};
_Static_assert(sizeof(struct uctx) == 0xF0, "layout must match uswitch_win64.S");

void uswitch(struct uctx *old, struct uctx *new);

enum state { FREE, RUNNABLE, RUNNING };

#define MAX_THREAD 4
#define STACK_SIZE (256 * 1024)

struct uthread {
    struct uctx ctx;
    enum state state;
    const char *name;
    void (*fn)(void *);
    void *arg;
    char *stack; /* lowest address of the allocation, guard page included */
};

static struct uthread all[MAX_THREAD]; /* all[0] is main */
static struct uthread *current;
static uint64_t switches;

static void thread_schedule(void)
{
    struct uthread *next = NULL;
    int self = (int)(current - all);
    for (int k = 1; k <= MAX_THREAD; k++) {
        struct uthread *t = &all[(self + k) % MAX_THREAD];
        if (t->state == RUNNABLE) {
            next = t;
            break;
        }
    }
    if (next == NULL) {
        fprintf(stderr, "thread_schedule: no runnable threads\n");
        exit(1);
    }
    if (next == current) {
        current->state = RUNNING;
        return;
    }
    struct uthread *prev = current;
    current = next;
    next->state = RUNNING;
    switches++;
    uswitch(&prev->ctx, &next->ctx);
    /* we are back: some other thread chose prev again */
}

static void thread_yield(void)
{
    current->state = RUNNABLE;
    thread_schedule();
}

static void thread_returned(void)
{
    fprintf(stderr, "thread_entry returned; there is no caller to return to\n");
    abort();
}

/* Every new thread starts here, on its own stack, by uswitch's `ret`. */
static void thread_entry(void)
{
    current->fn(current->arg);
    current->state = FREE;
    thread_schedule(); /* a FREE thread is never picked again */
    thread_returned();
}

static void thread_create(const char *name, void (*fn)(void *), void *arg)
{
    struct uthread *t = NULL;
    for (int i = 1; i < MAX_THREAD; i++)
        if (all[i].state == FREE) {
            t = &all[i];
            break;
        }
    if (!t) {
        fprintf(stderr, "thread_create: table full\n");
        exit(1);
    }
    if (t->stack == NULL) {
        /* one extra page at the bottom, made inaccessible, so an overflow
         * faults instead of silently overwriting the next allocation */
        t->stack = VirtualAlloc(NULL, STACK_SIZE + 4096, MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);
        DWORD old;
        if (!t->stack || !VirtualProtect(t->stack, 4096, PAGE_NOACCESS, &old)) {
            fprintf(stderr, "thread_create: no stack\n");
            exit(1);
        }
    }
    memset(&t->ctx, 0, sizeof t->ctx);
    t->name = name;
    t->fn = fn;
    t->arg = arg;

    /* Build the stack so that uswitch's `ret` "returns" into thread_entry
     * with the alignment the ABI promises at a function entry: rsp is 8
     * below a 16-byte boundary, with a return address at [rsp] and 32
     * bytes of shadow space above it. */
    char *top = t->stack + 4096 + STACK_SIZE; /* 16-byte aligned */
    uint64_t *sp = (uint64_t *)(top - 48);
    sp[0] = (uint64_t)(uintptr_t)thread_entry;    /* popped by uswitch's ret */
    sp[1] = (uint64_t)(uintptr_t)thread_returned; /* thread_entry's return address */
    t->ctx.rsp = (uint64_t)(uintptr_t)sp;
    t->state = RUNNABLE;
}

static int live_threads(void)
{
    int n = 0;
    for (int i = 1; i < MAX_THREAD; i++)
        n += all[i].state != FREE;
    return n;
}

/* ---- the demo -------------------------------------------------------- */

struct work {
    int id;
    int rounds;
    int print;
    int64_t isum;
    double dsum;
};

static void worker(void *arg)
{
    struct work *w = arg;
    int64_t isum = 0;
    double dsum = 0.0;
    for (int i = 0; i < w->rounds; i++) {
        isum += (int64_t)i * (w->id + 1);
        dsum += (double)i * 0.25 + (double)w->id;
        if (w->print)
            printf("  %s round %d\n", current->name, i);
        thread_yield();
    }
    w->isum = isum;
    w->dsum = dsum;
}

static int check(const struct work *w)
{
    int64_t isum = 0;
    double dsum = 0.0;
    for (int i = 0; i < w->rounds; i++) {
        isum += (int64_t)i * (w->id + 1);
        dsum += (double)i * 0.25 + (double)w->id;
    }
    int ok = isum == w->isum && dsum == w->dsum;
    printf("  thread %d: integer sum %lld (expected %lld), double sum %.2f (expected %.2f) %s\n",
           w->id, (long long)w->isum, (long long)isum, w->dsum, dsum, ok ? "ok" : "WRONG");
    return ok;
}

static int run(const char *title, int rounds, int print, int nthreads, double *ns)
{
    static const char *names[] = {"thread_a", "thread_b", "thread_c"};
    struct work w[3] = {{0}};
    printf("%s\n", title);
    for (int i = 0; i < nthreads; i++) {
        w[i].id = i;
        w[i].rounds = rounds;
        w[i].print = print;
        thread_create(names[i], worker, &w[i]);
    }

    LARGE_INTEGER f, t0, t1;
    QueryPerformanceFrequency(&f);
    uint64_t s0 = switches;
    QueryPerformanceCounter(&t0);
    while (live_threads() > 0)
        thread_yield();
    QueryPerformanceCounter(&t1);
    uint64_t n = switches - s0;
    if (ns)
        *ns = (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)f.QuadPart / (double)n;

    int ok = 1;
    for (int i = 0; i < nthreads; i++)
        ok &= check(&w[i]);
    printf("  %llu switches\n\n", (unsigned long long)n);
    return ok;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    current = &all[0];
    all[0].state = RUNNING;
    all[0].name = "main";

#ifdef SKIP_XMM
    printf("Built with SKIP_XMM: uswitch does not save xmm6-xmm15.\n\n");
#endif
    int ok = run("Three threads, three rounds each:", 3, 1, 3, NULL);

    double ns = 0;
    ok &= run("Two threads, 5,000,000 rounds each (timed):", 5000000, 0, 2, &ns);
    printf("uswitch round robin: %.1f ns per switch, including the scheduler loop\n", ns);
    return ok ? 0 : 1;
}
