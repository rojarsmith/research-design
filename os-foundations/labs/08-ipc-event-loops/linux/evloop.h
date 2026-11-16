/*
 * evloop.h - Stage 08: a small single-threaded event loop on epoll.
 *
 * One epoll instance watches every source; evloop_dispatch() sleeps in
 * epoll_wait(2) until some source is ready, then calls each ready source's
 * callback on the calling thread. Four kinds of source:
 *
 *   evloop_add_fd()      any pollable fd you own: sockets, pipes, devices
 *   evloop_add_timer()   a timerfd (CLOCK_MONOTONIC); the callback gets the
 *                        number of expirations since the last one, so a
 *                        value above 1 means the loop was late
 *   evloop_add_signal()  a signalfd for one signal, which the call blocks
 *                        in the calling thread
 *   evloop_add_event()   an eventfd; evsource_event_post() may be called
 *                        from any thread and wakes the loop
 *
 * The shape follows libwayland-server's wl_event_loop (add_fd, add_timer,
 * add_signal, dispatch, get_fd), so that the libwayland reading in the lab
 * has something to compare with, and so that the stage 09 server can be
 * built on it. It is level-triggered: a source that is still ready after
 * its callback (unread bytes, an unread connection) is reported again by
 * the next dispatch.
 *
 * Rules:
 *  - Only the thread that calls evloop_dispatch() may add or remove
 *    sources. evsource_event_post() is the one call meant for other threads.
 *  - Add signal sources before creating any threads, so that the threads
 *    inherit the blocked mask. A signal that is not blocked in every thread
 *    may be delivered to a thread the normal way and never reach the
 *    signalfd (signalfd(2)).
 *  - evsource_remove() may be called from inside any callback, also for a
 *    source whose event is still waiting in the same batch: removed sources
 *    are marked dead and freed only after the batch.
 *  - Callbacks must not block. A callback that sleeps for 100 ms delays
 *    every other source by 100 ms; the timer's expiration count shows it.
 *
 * Build it together with your program:  gcc -O2 -Wall -o prog prog.c evloop.c
 */
#ifndef LAB08_EVLOOP_H
#define LAB08_EVLOOP_H

#include <stdint.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>

struct evloop;
struct evsource;

typedef void (*evloop_fd_fn)(struct evsource *src, int fd, uint32_t events, void *data);
typedef void (*evloop_timer_fn)(struct evsource *src, uint64_t expirations, void *data);
typedef void (*evloop_signal_fn)(struct evsource *src, const struct signalfd_siginfo *si,
                                 void *data);
typedef void (*evloop_event_fn)(struct evsource *src, uint64_t count, void *data);

/* Create and destroy a loop. evloop_destroy() removes every source left. */
struct evloop *evloop_create(void);
void evloop_destroy(struct evloop *loop);

/* The epoll fd itself. It is readable whenever a source is ready, so this
 * loop can be nested inside another loop (as wl_event_loop_get_fd allows). */
int evloop_get_fd(const struct evloop *loop);

/* Watch fd for events (EPOLLIN, EPOLLOUT, ...; EPOLLHUP and EPOLLERR are
 * always reported). The loop does not take ownership: close fd yourself,
 * after evsource_remove(). Returns NULL with errno on failure. */
struct evsource *evloop_add_fd(struct evloop *loop, int fd, uint32_t events, evloop_fd_fn fn,
                               void *data);
int evsource_fd_update(struct evsource *src, uint32_t events);

/* A timer, created disarmed. evsource_timer_arm(): first expiry first_ns
 * from now, then every period_ns (0 for a one-shot); 0, 0 disarms. */
struct evsource *evloop_add_timer(struct evloop *loop, evloop_timer_fn fn, void *data);
int evsource_timer_arm(struct evsource *src, uint64_t first_ns, uint64_t period_ns);

/* Deliver signo through the loop instead of a signal handler. */
struct evsource *evloop_add_signal(struct evloop *loop, int signo, evloop_signal_fn fn,
                                   void *data);

/* A wakeup that other threads can post. The callback gets the sum of all
 * posts since the last callback (eventfd(2) without EFD_SEMAPHORE). */
struct evsource *evloop_add_event(struct evloop *loop, evloop_event_fn fn, void *data);
int evsource_event_post(struct evsource *src, uint64_t n);

/* Stop watching a source and free it (later, if called during dispatch).
 * Closes the timerfd, signalfd or eventfd the loop created; never the fd
 * you passed to evloop_add_fd(). */
void evsource_remove(struct evsource *src);

/* Wait up to timeout_ms (-1: forever) for ready sources and run their
 * callbacks. Returns the number of events handled, 0 on timeout or EINTR,
 * or -1 with errno. Call it in a loop until your program decides to stop. */
int evloop_dispatch(struct evloop *loop, int timeout_ms);

#endif
