/*
 * readiness.c - Stage 08, exercise D: what a readiness model tells you,
 * and what it cannot.
 *
 * Build:  gcc -O2 -Wall -o readiness readiness.c
 * Run:    ./readiness
 *
 * 1. Level-triggered vs edge-triggered epoll on a pipe with unread bytes.
 * 2. Writability as back-pressure: how many messages fit into a Unix
 *    socket before send() says EAGAIN, and how many the peer must read
 *    before EPOLLOUT comes back.
 * 3. Regular files: poll() calls them always readable, and epoll refuses
 *    them. Readiness says nothing about a read that waits for a disk.
 * 4. A completion interface: try io_uring_setup(2). A container's seccomp
 *    profile may refuse it; the program reports what happened.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/io_uring.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

static void die(const char *what)
{
    perror(what);
    exit(1);
}

static int ready(int ep, uint32_t *events)
{
    struct epoll_event ev;
    int n = epoll_wait(ep, &ev, 1, 0); /* timeout 0: just ask */
    if (n < 0)
        die("epoll_wait");
    if (events)
        *events = n ? ev.events : 0;
    return n;
}

static int unread(int fd)
{
    int n = 0;
    ioctl(fd, FIONREAD, &n);
    return n;
}

static void trigger_mode(int et)
{
    int p[2];
    if (pipe2(p, O_NONBLOCK | O_CLOEXEC) < 0)
        die("pipe");
    int ep = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev = {.events = EPOLLIN | (et ? EPOLLET : 0)};
    epoll_ctl(ep, EPOLL_CTL_ADD, p[0], &ev);
    char buf[16];
    printf("  %s:\n", et ? "edge-triggered (EPOLLIN | EPOLLET)" : "level-triggered (EPOLLIN)");
    if (write(p[1], "0123456789", 10) != 10)
        die("write");
    printf("    wrote 10 bytes                 -> epoll_wait reports %d event(s)\n", ready(ep, NULL));
    if (read(p[0], buf, 4) != 4)
        die("read");
    printf("    read 4, %d left unread          -> epoll_wait reports %d event(s)\n", unread(p[0]),
           ready(ep, NULL));
    if (et) {
        if (write(p[1], "A", 1) != 1)
            die("write");
        printf("    wrote 1 more byte              -> epoll_wait reports %d event(s)\n",
               ready(ep, NULL));
    }
    int total = 0;
    ssize_t r;
    while ((r = read(p[0], buf, sizeof buf)) > 0)
        total += (int)r;
    printf("    read until EAGAIN (%d bytes)    -> epoll_wait reports %d event(s)\n", total,
           ready(ep, NULL));
    close(ep);
    close(p[0]);
    close(p[1]);
}

static void part1(void)
{
    printf("1. Level-triggered vs edge-triggered, on a pipe\n");
    trigger_mode(0);
    trigger_mode(1);
}

static void part2(void)
{
    printf("\n2. Writability is back-pressure (SOCK_SEQPACKET socketpair, 64-byte messages)\n");
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, sv) < 0)
        die("socketpair");
    int sndbuf = 0;
    socklen_t len = sizeof sndbuf;
    getsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);
    char msg[64] = "frame";
    int sent = 0;
    while (send(sv[0], msg, sizeof msg, MSG_NOSIGNAL) == (ssize_t)sizeof msg)
        sent++;
    printf("  SO_SNDBUF %d bytes; %d messages sent before send() failed with %s\n", sndbuf, sent,
           strerrorname_np(errno));
    int ep = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev = {.events = EPOLLOUT};
    epoll_ctl(ep, EPOLL_CTL_ADD, sv[0], &ev);
    printf("  the sender's EPOLLOUT now: %d event(s)\n", ready(ep, NULL));
    int drained = 0;
    while (ready(ep, NULL) == 0 && recv(sv[1], msg, sizeof msg, 0) > 0)
        drained++;
    printf("  the peer had to read %d of the %d messages before EPOLLOUT came back\n", drained,
           sent);
    close(ep);
    close(sv[0]);
    close(sv[1]);
}

static void part3(void)
{
    printf("\n3. Regular files are always \"ready\"\n");
    int fd = open("/proc/self/exe", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        die("open");
    struct pollfd p = {.fd = fd, .events = POLLIN};
    int n = poll(&p, 1, 0);
    printf("  poll() on a regular file: %d ready, revents 0x%x (POLLIN is 0x%x)\n", n, p.revents,
           POLLIN);
    int ep = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev = {.events = EPOLLIN};
    int r = epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev);
    printf("  epoll_ctl(EPOLL_CTL_ADD) on it: %d, %s (%s)\n", r, r < 0 ? strerrorname_np(errno) : "",
           r < 0 ? strerror(errno) : "");
    close(ep);
    close(fd);
}

static void part4(void)
{
    printf("\n4. A completion interface: io_uring\n");
    struct io_uring_params params;
    memset(&params, 0, sizeof params);
    errno = 0;
    long fd = syscall(__NR_io_uring_setup, 4, &params);
    if (fd < 0) {
        int e = errno;
        printf("  io_uring_setup(4 entries) -> -1, %s (%s)\n", strerrorname_np(e), strerror(e));
        FILE *f = fopen("/proc/sys/kernel/io_uring_disabled", "r");
        int dis = -1;
        if (f) {
            if (fscanf(f, "%d", &dis) != 1)
                dis = -1;
            fclose(f);
        }
        printf("  /proc/sys/kernel/io_uring_disabled: %d\n", dis);
        f = fopen("/proc/self/status", "r");
        char line[128];
        int mode;
        while (f && fgets(line, sizeof line, f))
            if (sscanf(line, "Seccomp: %d", &mode) == 1)
                printf("  this process's Seccomp mode: %d%s\n", mode,
                       mode == 2 ? " (a seccomp filter is installed)" : "");
        if (f)
            fclose(f);
        printf("  (completion via io_uring could not be shown here)\n");
        return;
    }
    printf("  io_uring_setup(4 entries) -> fd %ld: io_uring is available; sq %u entries, cq %u\n",
           fd, params.sq_entries, params.cq_entries);
    printf("  (this program stops here; see io_uring(7) for submitting a read)\n");
    close((int)fd);
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
