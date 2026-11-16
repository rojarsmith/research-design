/*
 * sigbus.c - Stage 03, exercise E: shrink a file under a live mapping.
 *
 * Build:  gcc -O2 -Wall -o sigbus sigbus.c
 * Run:    ./sigbus [file]      (default: ./sigbus.dat, created here)
 *
 * 1. Map three pages of a file, truncate the file to one page plus 100
 *    bytes, then read each page. Catch the SIGBUS and print what the
 *    kernel put in siginfo.
 * 2. The server case: a "client" process shrinks a file that a "server"
 *    process has mapped and is reading. Once without a SIGBUS handler,
 *    once with one.
 * 3. The fix that stage 08 builds on: a memfd sealed with F_SEAL_SHRINK
 *    cannot be shrunk at all.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define PAGE 4096

static const char *path = "sigbus.dat";
static sigjmp_buf jb;
static volatile sig_atomic_t got_code;
static void *volatile got_addr;

static void die(const char *what)
{
    perror(what);
    exit(1);
}

static void on_sigbus(int sig, siginfo_t *si, void *uc)
{
    (void)sig;
    (void)uc;
    got_addr = si->si_addr;
    got_code = si->si_code;
    siglongjmp(jb, 1);
}

static void catch_sigbus(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_sigbus;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGBUS, &sa, NULL) < 0)
        die("sigaction");
}

static const char *code_name(int code)
{
    switch (code) {
    case BUS_ADRALN: return "BUS_ADRALN";
    case BUS_ADRERR: return "BUS_ADRERR";
    case BUS_OBJERR: return "BUS_OBJERR";
    default: return "other";
    }
}

static int make_file(size_t pages)
{
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        die("open");
    char buf[PAGE];
    for (size_t i = 0; i < pages; i++) {
        memset(buf, 'a' + (int)i, sizeof buf);
        if (write(fd, buf, sizeof buf) != PAGE)
            die("write");
    }
    return fd;
}

/* Read one byte, or report the SIGBUS it caused. */
static void probe(const char *name, volatile char *p, const char *base)
{
    if (sigsetjmp(jb, 1) == 0) {
        char c = *p;
        printf("  read %-28s -> 0x%02x%s\n", name, (unsigned char)c,
               c == 0 ? "  (zero)" : "");
    } else {
        printf("  read %-28s -> SIGBUS, si_code %d (%s), si_addr = map + 0x%lx\n",
               name, (int)got_code, code_name(got_code),
               (unsigned long)((char *)got_addr - base));
    }
}

static void part1(void)
{
    printf("1. Truncate a mapped file from 3 pages to 1 page + 100 bytes\n");
    int fd = make_file(3);
    char *m = mmap(NULL, 3 * PAGE, PROT_READ, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED)
        die("mmap");
    printf("  before: page 0 '%c', page 1 '%c', page 2 '%c'\n", m[0], m[PAGE],
           m[2 * PAGE]);
    if (ftruncate(fd, PAGE + 100) < 0)
        die("ftruncate");
    catch_sigbus();
    probe("page 0, offset 0", m, m);
    probe("page 1, offset 50", m + PAGE + 50, m);
    probe("page 1, offset 200 (past EOF)", m + PAGE + 200, m);
    probe("page 2, offset 0", m + 2 * PAGE, m);
    munmap(m, 3 * PAGE);
    close(fd);
    signal(SIGBUS, SIG_DFL);
}

/* The server sums a buffer that the client gave it, page by page. */
static long server_sum(const volatile char *buf, size_t len, int client_fd,
                       int go_fd)
{
    long sum = 0;
    for (size_t i = 0; i < len; i++) {
        if (i == PAGE) {
            /* halfway through: let the client act, then wait for it */
            char c = 'x';
            if (write(client_fd, &c, 1) != 1 || read(go_fd, &c, 1) != 1)
                die("pipe");
        }
        sum += buf[i];
    }
    return sum;
}

static void run_server(int with_handler)
{
    int to_client[2], to_server[2];
    if (pipe(to_client) < 0 || pipe(to_server) < 0)
        die("pipe");
    int fd = make_file(4);

    pid_t server = fork();
    if (server < 0)
        die("fork");
    if (server == 0) {
        char *buf = mmap(NULL, 4 * PAGE, PROT_READ, MAP_SHARED, fd, 0);
        if (buf == MAP_FAILED)
            die("mmap");
        if (with_handler) {
            catch_sigbus();
            if (sigsetjmp(jb, 1) != 0) {
                printf("  server: SIGBUS at buffer + 0x%lx; rejecting this "
                       "client's buffer and carrying on\n",
                       (unsigned long)((char *)got_addr - buf));
                _exit(0);
            }
        }
        long s = server_sum(buf, 4 * PAGE, to_client[1], to_server[0]);
        printf("  server: sum %ld\n", s);
        _exit(0);
    }
    /* the client: wait until the server is halfway, then shrink the file */
    char c;
    if (read(to_client[0], &c, 1) != 1)
        die("pipe");
    if (ftruncate(fd, PAGE) < 0)
        die("ftruncate");
    printf("  client: truncated the shared file to one page\n");
    fflush(stdout);
    if (write(to_server[1], &c, 1) != 1)
        die("pipe");
    int status;
    waitpid(server, &status, 0);
    if (WIFSIGNALED(status))
        printf("  server was killed by signal %d (%s)\n", WTERMSIG(status),
               strsignal(WTERMSIG(status)));
    else
        printf("  server exited with status %d\n", WEXITSTATUS(status));
    close(fd);
    close(to_client[0]);
    close(to_client[1]);
    close(to_server[0]);
    close(to_server[1]);
}

static void part2(void)
{
    printf("\n2. A client shrinks a file that a server has mapped\n");
    printf("  a) server without a SIGBUS handler\n");
    fflush(stdout);
    run_server(0);
    printf("  b) server with a SIGBUS handler\n");
    fflush(stdout);
    run_server(1);
}

static void part3(void)
{
    printf("\n3. A sealed memfd cannot shrink (the fix stage 08 builds on)\n");
    int fd = memfd_create("lab03", MFD_ALLOW_SEALING);
    if (fd < 0) {
        perror("  memfd_create");
        return;
    }
    if (ftruncate(fd, 4 * PAGE) < 0)
        die("ftruncate");
    if (fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK) < 0) {
        perror("  F_ADD_SEALS");
        return;
    }
    int seals = fcntl(fd, F_GET_SEALS);
    printf("  seals 0x%x (F_SEAL_SHRINK is 0x%x)\n", seals, F_SEAL_SHRINK);
    errno = 0;
    int r = ftruncate(fd, PAGE);
    printf("  ftruncate to one page -> %d, errno %d (%s)\n", r, errno,
           strerror(errno));
    close(fd);
}

int main(int argc, char **argv)
{
    if (argc > 1)
        path = argv[1];
    setvbuf(stdout, NULL, _IOLBF, 0);
    part1();
    part2();
    part3();
    unlink(path);
    return 0;
}
