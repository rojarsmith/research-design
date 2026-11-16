/*
 * sealtest.c - Stage 08, exercise B: memfd buffers with MFD_ALLOW_SEALING,
 * and a receiver that refuses any buffer that could still shrink.
 *
 * Build:  gcc -O2 -Wall -o sealtest sealtest.c sealbuf.c fdpass.c
 * Run:    ./sealtest           (forks its own sender and receivers)
 *
 * 1. The check. A sender process offers ten fds over a socket; the
 *    receiver accepts one only if sealbuf_check() says it can never shrink
 *    and is as large as the sender claims.
 * 2. A sealed buffer cannot be shrunk by its sender. After the receiver has
 *    mapped it, the sender tries every way it has to make it smaller, then
 *    changes its contents, which the seals allow.
 * 3. What happens with an unsealed one: a receiver that checks only the
 *    size, maps the buffer, and is then shrunk under, once without and once
 *    with a SIGBUS handler.
 * 4. Freezing the contents too: F_SEAL_WRITE, and F_SEAL_FUTURE_WRITE,
 *    which lets the writer that already has a mapping keep writing.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "fdpass.h"
#include "sealbuf.h"

#define PAGE 4096

struct offer {
    uint32_t claimed; /* bytes the sender says the receiver may read */
    char label[60];
};

static void die(const char *what)
{
    perror(what);
    exit(1);
}

static const char *err(int r)
{
    static char buf[64];
    if (r >= 0)
        snprintf(buf, sizeof buf, "ok");
    else
        snprintf(buf, sizeof buf, "%s (%s)", strerrorname_np(errno), strerror(errno));
    return buf;
}

static void sync_send(int s)
{
    if (fdpass_send(s, "s", 1, NULL, 0) < 0)
        die("send");
}

static void sync_wait(int s)
{
    char c;
    if (fdpass_recv(s, &c, 1, NULL, NULL, NULL) != 1)
        die("recv");
}

/* ---- SIGBUS reporting --------------------------------------------------- */

static sigjmp_buf jb;
static void *volatile bus_addr;
static volatile int bus_code;

static void on_sigbus(int sig, siginfo_t *si, void *uc)
{
    (void)sig;
    (void)uc;
    bus_addr = si->si_addr;
    bus_code = si->si_code;
    siglongjmp(jb, 1);
}

static void catch_sigbus(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_sigbus;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGBUS, &sa, NULL);
}

/* ---- part 1: the receiver's check --------------------------------------- */

static int make_memfd(unsigned flags, size_t size)
{
    int fd = memfd_create("lab08", flags | MFD_CLOEXEC);
    if (fd < 0 || ftruncate(fd, (off_t)size) < 0)
        die("memfd_create");
    return fd;
}

static void offer(int s, int fd, uint32_t claimed, const char *label)
{
    struct offer o = {.claimed = claimed};
    snprintf(o.label, sizeof o.label, "%s", label);
    if (fdpass_send(s, &o, sizeof o, &fd, 1) < 0)
        die("sendmsg");
    close(fd);
}

static void sender1(int s)
{
    int p[2];
    if (pipe2(p, O_CLOEXEC) < 0)
        die("pipe");
    offer(s, p[0], PAGE, "a pipe");
    close(p[1]);

    char path[] = "/tmp/lab08-sealtest-XXXXXX";
    int f = mkostemp(path, O_CLOEXEC);
    if (f < 0 || ftruncate(f, 4 * PAGE) < 0)
        die("mkstemp");
    unlink(path);
    offer(s, f, 4 * PAGE, "a regular file in /tmp");

    char shm[64];
    snprintf(shm, sizeof shm, "/lab08-sealtest-%d", (int)getpid());
    int sh = shm_open(shm, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (sh < 0 || ftruncate(sh, 4 * PAGE) < 0)
        die("shm_open");
    shm_unlink(shm);
    offer(s, sh, 4 * PAGE, "a POSIX shm_open() object (tmpfs)");

    offer(s, make_memfd(0, 4 * PAGE), 4 * PAGE, "memfd without MFD_ALLOW_SEALING");
    offer(s, make_memfd(MFD_ALLOW_SEALING, 4 * PAGE), 4 * PAGE, "memfd, sealing allowed, no seals");

    int m = make_memfd(MFD_ALLOW_SEALING, 4 * PAGE);
    sealbuf_seal(m, F_SEAL_GROW);
    offer(s, m, 4 * PAGE, "memfd sealed GROW only");

    m = make_memfd(MFD_ALLOW_SEALING, 4 * PAGE);
    sealbuf_seal(m, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL);
    offer(s, m, 16 * PAGE, "memfd sealed, but claims 16 pages");

    m = make_memfd(MFD_ALLOW_SEALING, 4 * PAGE);
    sealbuf_seal(m, F_SEAL_SHRINK);
    offer(s, m, 4 * PAGE, "memfd sealed SHRINK");

    m = make_memfd(MFD_ALLOW_SEALING, 4 * PAGE);
    sealbuf_seal(m, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL);
    offer(s, m, 4 * PAGE, "memfd sealed SHRINK|GROW|SEAL");

    m = make_memfd(MFD_ALLOW_SEALING, 4 * PAGE);
    sealbuf_seal(m, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_SEAL);
    offer(s, m, 4 * PAGE, "memfd sealed SHRINK|GROW|WRITE|SEAL");

    struct offer end = {.claimed = 0, .label = "end"};
    fdpass_send(s, &end, sizeof end, NULL, 0);
}

static void part1(void)
{
    printf("1. The receiver accepts a buffer only if it can never shrink\n");
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sv) < 0)
        die("socketpair");
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        close(sv[1]);
        sender1(sv[0]);
        _exit(0);
    }
    close(sv[0]);
    for (;;) {
        struct offer o;
        int fd, n = 1;
        if (fdpass_recv(sv[1], &o, sizeof o, &fd, &n, NULL) <= 0 || o.claimed == 0)
            break;
        char why[128];
        size_t size;
        int ok = sealbuf_check(fd, o.claimed, F_SEAL_SHRINK, &size, why, sizeof why) == 0;
        printf("  %-38s %s: %s\n", o.label, ok ? "ACCEPT" : "reject", why);
        close(fd);
    }
    waitpid(pid, NULL, 0);
    close(sv[1]);
}

/* ---- part 2: the sender cannot shrink a sealed buffer ------------------- */

static void part2(void)
{
    printf("\n2. A sealed buffer cannot be shrunk by its sender\n");
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sv) < 0)
        die("socketpair");
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        /* the receiver */
        close(sv[0]);
        struct offer o;
        int fd, n = 1;
        char why[128];
        size_t size;
        if (fdpass_recv(sv[1], &o, sizeof o, &fd, &n, NULL) <= 0 || n != 1)
            die("recvmsg");
        if (sealbuf_check(fd, o.claimed, F_SEAL_SHRINK, &size, why, sizeof why) < 0) {
            printf("  receiver: rejected: %s\n", why);
            _exit(1);
        }
        const volatile unsigned char *p = sealbuf_map_ro(fd, size);
        printf("  receiver: accepted (%s) and mapped %zu bytes\n", why, size);
        fflush(stdout);
        sync_send(sv[1]); /* mapped */
        sync_wait(sv[1]); /* the sender has done its worst */
        catch_sigbus();
        if (sigsetjmp(jb, 1) == 0) {
            printf("  receiver: first byte of each page:");
            for (size_t i = 0; i < size; i += PAGE)
                printf(" 0x%02x", p[i]);
            printf("  (no SIGBUS)\n");
        } else {
            printf("  receiver: SIGBUS at offset 0x%lx\n",
                   (unsigned long)((const char *)bus_addr - (const char *)p));
        }
        _exit(0);
    }
    close(sv[1]);
    int fd = sealbuf_create("lab08-sealed", 4 * PAGE);
    if (fd < 0)
        die("memfd_create");
    unsigned char *w = mmap(NULL, 4 * PAGE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (w == MAP_FAILED)
        die("mmap");
    for (int i = 0; i < 4; i++)
        memset(w + i * PAGE, 0x10 + i, PAGE);
    int r = sealbuf_seal(fd, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL);
    printf("  sender:   memfd of 4 pages filled with 0x10..0x13, F_ADD_SEALS SHRINK|GROW|SEAL -> "
           "%s\n",
           err(r));
    struct offer o = {.claimed = 4 * PAGE, .label = "sealed"};
    if (fdpass_send(sv[0], &o, sizeof o, &fd, 1) < 0)
        die("sendmsg");
    sync_wait(sv[0]);

    r = ftruncate(fd, PAGE);
    printf("  sender:   ftruncate to 1 page                 -> %s\n", err(r));
    r = ftruncate(fd, 0);
    printf("  sender:   ftruncate to 0                      -> %s\n", err(r));
    r = ftruncate(fd, 8 * PAGE);
    printf("  sender:   ftruncate to 8 pages                -> %s\n", err(r));
    char proc[64];
    snprintf(proc, sizeof proc, "/proc/self/fd/%d", fd);
    r = open(proc, O_RDWR | O_TRUNC | O_CLOEXEC);
    printf("  sender:   open(/proc/self/fd/%d, O_TRUNC)      -> %s\n", fd, err(r));
    if (r >= 0)
        close(r);
    r = fcntl(fd, F_ADD_SEALS, F_SEAL_WRITE);
    printf("  sender:   F_ADD_SEALS WRITE (after SEAL)      -> %s\n", err(r));
    r = fcntl(fd, F_GET_SEALS);
    char names[64];
    printf("  sender:   F_GET_SEALS                         -> %s; there is no call that removes a "
           "seal\n",
           sealbuf_seal_names(r, names, sizeof names));
    w[0] = 0xAA;
    printf("  sender:   writes 0xAA into page 0 through its mapping -> ok (contents may change)\n");
    r = fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, 2 * PAGE, PAGE);
    printf("  sender:   punches a hole in page 2            -> %s\n", err(r));
    fflush(stdout);
    sync_send(sv[0]);
    int st;
    waitpid(pid, &st, 0);
    munmap(w, 4 * PAGE);
    close(fd);
    close(sv[0]);
}

/* ---- part 3: an unsealed buffer and a receiver that only checks the size - */

static void part3_run(int with_handler)
{
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sv) < 0)
        die("socketpair");
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        close(sv[0]);
        struct offer o;
        int fd, n = 1;
        if (fdpass_recv(sv[1], &o, sizeof o, &fd, &n, NULL) <= 0 || n != 1)
            die("recvmsg");
        struct stat st;
        fstat(fd, &st);
        printf("  receiver: fstat says %lld bytes >= %u claimed: looks fine, mapping it\n",
               (long long)st.st_size, o.claimed);
        const volatile unsigned char *p = mmap(NULL, o.claimed, PROT_READ, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED)
            die("mmap");
        unsigned sum = p[0];
        fflush(stdout);
        sync_send(sv[1]);
        sync_wait(sv[1]);
        if (with_handler) {
            catch_sigbus();
            if (sigsetjmp(jb, 1) != 0) {
                printf("  receiver: SIGBUS (si_code %d, %s) at offset 0x%lx; dropping this "
                       "client's buffer and carrying on\n",
                       bus_code, bus_code == BUS_ADRERR ? "BUS_ADRERR" : "other",
                       (unsigned long)((const char *)bus_addr - (const char *)p));
                _exit(0);
            }
        }
        for (size_t i = 0; i < o.claimed; i += 64)
            sum += p[i];
        printf("  receiver: sum %u\n", sum);
        _exit(0);
    }
    close(sv[1]);
    int fd = make_memfd(MFD_ALLOW_SEALING, 4 * PAGE); /* allowed, but never sealed */
    struct offer o = {.claimed = 4 * PAGE, .label = "unsealed"};
    if (fdpass_send(sv[0], &o, sizeof o, &fd, 1) < 0)
        die("sendmsg");
    sync_wait(sv[0]);
    int r = ftruncate(fd, PAGE);
    printf("  sender:   ftruncate to 1 page -> %s\n", err(r));
    fflush(stdout);
    sync_send(sv[0]);
    int st;
    waitpid(pid, &st, 0);
    if (WIFSIGNALED(st))
        printf("  receiver was killed by signal %d (%s)\n", WTERMSIG(st), strsignal(WTERMSIG(st)));
    else
        printf("  receiver exited with status %d\n", WEXITSTATUS(st));
    close(fd);
    close(sv[0]);
}

static void part3(void)
{
    printf("\n3. An unsealed buffer, and a receiver that only checked its size\n");
    printf("  a) receiver without a SIGBUS handler\n");
    part3_run(0);
    printf("  b) receiver with a SIGBUS handler\n");
    part3_run(1);
}

/* ---- part 4: freezing the contents --------------------------------------- */

static void part4(void)
{
    printf("\n4. Freezing the contents: F_SEAL_WRITE and F_SEAL_FUTURE_WRITE\n");
    int fd = sealbuf_create("lab08-write", PAGE);
    char *w = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (w == MAP_FAILED)
        die("mmap");
    strcpy(w, "frame 1");
    int r = sealbuf_seal(fd, F_SEAL_WRITE);
    printf("  F_SEAL_WRITE while the writer still has a shared writable mapping -> %s\n", err(r));
    munmap(w, PAGE);
    r = sealbuf_seal(fd, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_SEAL);
    printf("  F_SEAL_WRITE after munmap                                    -> %s\n", err(r));
    r = (int)pwrite(fd, "x", 1, 0);
    printf("  pwrite                                                       -> %s\n", err(r));
    void *m = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("  mmap PROT_WRITE, MAP_SHARED                                  -> %s\n",
           err(m == MAP_FAILED ? -1 : 0));
    m = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    printf("  mmap PROT_WRITE, MAP_PRIVATE                                 -> %s%s\n",
           err(m == MAP_FAILED ? -1 : 0),
           m == MAP_FAILED ? "" : "  (writes go to private copies)");
    if (m != MAP_FAILED) {
        ((char *)m)[0] = 'X';
        char c = '?';
        if (pread(fd, &c, 1, 0) != 1)
            perror("pread");
        printf("    wrote 'X' through it; the file still starts with '%c'\n", c);
        munmap(m, PAGE);
    }
    char proc[64];
    snprintf(proc, sizeof proc, "/proc/self/fd/%d", fd);
    int rw = open(proc, O_RDWR | O_CLOEXEC);
    r = rw < 0 ? -1 : (int)write(rw, "x", 1);
    printf("  reopen /proc/self/fd/%d O_RDWR (%s), then write             -> %s\n", fd,
           rw < 0 ? "failed" : "ok", err(r));
    if (rw >= 0)
        close(rw);
    close(fd);

#ifdef F_SEAL_FUTURE_WRITE
    fd = sealbuf_create("lab08-future", PAGE);
    w = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    strcpy(w, "frame 1");
    r = sealbuf_seal(fd, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_FUTURE_WRITE | F_SEAL_SEAL);
    printf("  F_SEAL_FUTURE_WRITE with the writer's mapping still in place -> %s\n", err(r));
    const char *ro = mmap(NULL, PAGE, PROT_READ, MAP_SHARED, fd, 0);
    strcpy(w, "frame 2");
    printf("  the writer writes \"frame 2\" through its old mapping; a reader's mapping shows "
           "\"%s\"\n",
           ro);
    r = (int)pwrite(fd, "x", 1, 0);
    printf("  pwrite                                                       -> %s\n", err(r));
    m = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("  a new mmap PROT_WRITE, MAP_SHARED                            -> %s\n",
           err(m == MAP_FAILED ? -1 : 0));
    close(fd);
#endif
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    part1();
    part2();
    part3();
    part4();
    return 0;
}
