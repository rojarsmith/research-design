/*
 * evloop.c - Stage 08: a single-threaded epoll event loop. See evloop.h.
 */
#define _GNU_SOURCE
#include "evloop.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

enum kind { SRC_FD, SRC_TIMER, SRC_SIGNAL, SRC_EVENT };

struct evsource {
    struct evloop *loop;
    enum kind kind;
    int fd;    /* the watched fd; -1 once removed */
    int owned; /* the loop created fd and closes it */
    int dead;
    union {
        evloop_fd_fn fd;
        evloop_timer_fn timer;
        evloop_signal_fn signal;
        evloop_event_fn event;
    } fn;
    void *data;
    struct evsource *prev, *next; /* the live list, or the dead list */
};

struct evloop {
    int epfd;
    int dispatching;
    struct evsource *live; /* every source not yet removed */
    struct evsource *dead; /* removed during a dispatch, freed after it */
};

struct evloop *evloop_create(void)
{
    struct evloop *loop = calloc(1, sizeof *loop);
    if (!loop)
        return NULL;
    loop->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (loop->epfd < 0) {
        free(loop);
        return NULL;
    }
    return loop;
}

int evloop_get_fd(const struct evloop *loop)
{
    return loop->epfd;
}

static struct evsource *add_source(struct evloop *loop, enum kind kind, int fd, int owned,
                                   uint32_t events, void *data)
{
    struct evsource *src = calloc(1, sizeof *src);
    if (!src)
        goto fail;
    src->loop = loop;
    src->kind = kind;
    src->fd = fd;
    src->owned = owned;
    src->data = data;
    struct epoll_event ev = {.events = events, .data.ptr = src};
    if (epoll_ctl(loop->epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
        goto fail;
    src->next = loop->live;
    if (loop->live)
        loop->live->prev = src;
    loop->live = src;
    return src;
fail: {
    int e = errno;
    free(src);
    if (owned)
        close(fd);
    errno = e;
    return NULL;
}
}

struct evsource *evloop_add_fd(struct evloop *loop, int fd, uint32_t events, evloop_fd_fn fn,
                               void *data)
{
    struct evsource *src = add_source(loop, SRC_FD, fd, 0, events, data);
    if (src)
        src->fn.fd = fn;
    return src;
}

int evsource_fd_update(struct evsource *src, uint32_t events)
{
    struct epoll_event ev = {.events = events, .data.ptr = src};
    return epoll_ctl(src->loop->epfd, EPOLL_CTL_MOD, src->fd, &ev);
}

struct evsource *evloop_add_timer(struct evloop *loop, evloop_timer_fn fn, void *data)
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0)
        return NULL;
    struct evsource *src = add_source(loop, SRC_TIMER, fd, 1, EPOLLIN, data);
    if (src)
        src->fn.timer = fn;
    return src;
}

int evsource_timer_arm(struct evsource *src, uint64_t first_ns, uint64_t period_ns)
{
    struct itimerspec its = {
        .it_value = {.tv_sec = (time_t)(first_ns / 1000000000u),
                     .tv_nsec = (long)(first_ns % 1000000000u)},
        .it_interval = {.tv_sec = (time_t)(period_ns / 1000000000u),
                        .tv_nsec = (long)(period_ns % 1000000000u)},
    };
    return timerfd_settime(src->fd, 0, &its, NULL);
}

struct evsource *evloop_add_signal(struct evloop *loop, int signo, evloop_signal_fn fn,
                                   void *data)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signo);
    /* Block it first: a blocked signal stays pending, and the signalfd
     * reports pending signals. An unblocked one would run its normal
     * disposition instead (often: terminate the process). */
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
        return NULL;
    int fd = signalfd(-1, &set, SFD_NONBLOCK | SFD_CLOEXEC);
    if (fd < 0)
        return NULL;
    struct evsource *src = add_source(loop, SRC_SIGNAL, fd, 1, EPOLLIN, data);
    if (src)
        src->fn.signal = fn;
    return src;
}

struct evsource *evloop_add_event(struct evloop *loop, evloop_event_fn fn, void *data)
{
    int fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd < 0)
        return NULL;
    struct evsource *src = add_source(loop, SRC_EVENT, fd, 1, EPOLLIN, data);
    if (src)
        src->fn.event = fn;
    return src;
}

int evsource_event_post(struct evsource *src, uint64_t n)
{
    /* One 8-byte write adds n to the eventfd's counter; it is atomic, so
     * any thread may call this while the loop thread dispatches. */
    ssize_t r;
    do
        r = write(src->fd, &n, sizeof n);
    while (r < 0 && errno == EINTR);
    return r == (ssize_t)sizeof n ? 0 : -1;
}

void evsource_remove(struct evsource *src)
{
    struct evloop *loop = src->loop;
    if (src->dead)
        return;
    epoll_ctl(loop->epfd, EPOLL_CTL_DEL, src->fd, NULL);
    if (src->owned)
        close(src->fd);
    src->fd = -1;
    src->dead = 1;
    /* unlink from the live list */
    if (src->prev)
        src->prev->next = src->next;
    else
        loop->live = src->next;
    if (src->next)
        src->next->prev = src->prev;
    src->prev = NULL;
    if (loop->dispatching) {
        /* An event for src may still be in this batch; the dispatcher
         * skips dead sources, and frees them when the batch is done. */
        src->next = loop->dead;
        loop->dead = src;
    } else {
        free(src);
    }
}

static void run_source(struct evsource *src, uint32_t events)
{
    switch (src->kind) {
    case SRC_FD:
        src->fn.fd(src, src->fd, events, src->data);
        break;
    case SRC_TIMER: {
        uint64_t exp;
        if (read(src->fd, &exp, sizeof exp) == (ssize_t)sizeof exp)
            src->fn.timer(src, exp, src->data);
        break;
    }
    case SRC_SIGNAL: {
        struct signalfd_siginfo si;
        if (read(src->fd, &si, sizeof si) == (ssize_t)sizeof si)
            src->fn.signal(src, &si, src->data);
        break;
    }
    case SRC_EVENT: {
        uint64_t count;
        if (read(src->fd, &count, sizeof count) == (ssize_t)sizeof count)
            src->fn.event(src, count, src->data);
        break;
    }
    }
}

int evloop_dispatch(struct evloop *loop, int timeout_ms)
{
    struct epoll_event ev[32];
    int n = epoll_wait(loop->epfd, ev, 32, timeout_ms);
    if (n < 0)
        return errno == EINTR ? 0 : -1;
    loop->dispatching = 1;
    for (int i = 0; i < n; i++) {
        struct evsource *src = ev[i].data.ptr;
        if (!src->dead)
            run_source(src, ev[i].events);
    }
    loop->dispatching = 0;
    while (loop->dead) {
        struct evsource *d = loop->dead;
        loop->dead = d->next;
        free(d);
    }
    return n;
}

void evloop_destroy(struct evloop *loop)
{
    while (loop->live)
        evsource_remove(loop->live);
    close(loop->epfd);
    free(loop);
}
