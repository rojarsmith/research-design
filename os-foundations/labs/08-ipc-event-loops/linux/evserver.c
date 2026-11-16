/*
 * evserver.c - Stage 08, exercise C: one single-threaded epoll loop serving
 * a Unix socket, a timerfd, a signalfd and an eventfd.
 *
 * Build:  gcc -O2 -Wall -pthread -o evserver evserver.c evloop.c fdpass.c sealbuf.c
 * Run:    ./evserver demo               server plus a scripted series of clients
 *         ./evserver serve PATH         the server alone, until SIGINT or SIGTERM;
 *                                       SIGUSR1 prints its statistics
 *         ./evserver client PATH KIND   one client; KIND is good, unsealed, stall,
 *                                       crash, fdflood or foreign
 *
 * The server's loop thread watches, through evloop.c:
 *   - the listening socket and every client socket (SOCK_SEQPACKET);
 *   - a timerfd at 60 Hz, standing in for vblank. It counts expirations: a
 *     read that returns more than 1 means the loop was late;
 *   - a signalfd for SIGINT, SIGTERM (shut down) and SIGUSR1 (statistics);
 *   - an eventfd that a worker thread posts when it has finished a job.
 * The worker thread checksums client buffers (and sleeps 20 ms to stand in
 * for real work), so the loop never waits for it. The loop thread is the
 * only one that touches client state or sockets.
 *
 * The protocol is one fixed-size struct msg per SOCK_SEQPACKET message.
 * A BUFFER message carries one memfd, which the server accepts only if
 * sealbuf_check() says it can never shrink.
 *
 * This server has a resource leak that exercise C asks you to find and fix.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "evloop.h"
#include "fdpass.h"
#include "sealbuf.h"

enum {
    /* client to server */
    MSG_HELLO = 1, /* text: the client's name */
    MSG_BUFFER,    /* arg: bytes to read; carries one memfd */
    MSG_PING,
    MSG_STALL, /* arg: ms the server should block for (a bug on purpose) */
    MSG_BYE,
    /* server to client */
    MSG_WELCOME = 100, /* arg: client id */
    MSG_OK,            /* arg: checksum of the buffer */
    MSG_REJECT,        /* text: why */
    MSG_PONG,          /* arg: vblank ticks so far */
};

struct msg {
    uint32_t type;
    uint32_t arg;
    char text[56];
};

#define BUF_SIZE (64 * 1024)
#define TICK_NS 16666667ull /* 1/60 s */

static struct timespec t0;

static double now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)(t.tv_sec - t0.tv_sec) * 1e3 + (double)(t.tv_nsec - t0.tv_nsec) / 1e6;
}

#define LOG(...)                                                                                   \
    do {                                                                                           \
        printf("[%7.1f ms] ", now_ms());                                                           \
        printf(__VA_ARGS__);                                                                       \
        printf("\n");                                                                              \
    } while (0)

static void die(const char *what)
{
    perror(what);
    exit(1);
}

/* ========================== the server =================================== */

struct server;

struct client {
    struct server *s;
    int id;
    int fd;
    pid_t pid;
    char name[32];
    struct evsource *src;
    struct client *next;
};

struct job {
    int client_id;
    int fd;
    const unsigned char *p;
    size_t size;
    uint32_t sum;
    struct job *next;
};

struct server {
    struct evloop *loop;
    int listen_fd;
    const char *path;
    int running;
    struct client *clients;
    int next_id;
    /* vblank statistics */
    uint64_t ticks, late_reads, max_exp;
    double started_ms;
    int fds_at_start;
    /* the worker */
    struct evsource *done_ev;
    pthread_t worker;
    pthread_mutex_t mu;
    pthread_cond_t cv;
    struct job *todo, *done;
    int stop;
};

static void *worker_main(void *arg)
{
    struct server *s = arg;
    pthread_mutex_lock(&s->mu);
    for (;;) {
        while (!s->todo && !s->stop)
            pthread_cond_wait(&s->cv, &s->mu);
        if (s->stop)
            break;
        struct job *j = s->todo;
        s->todo = j->next;
        pthread_mutex_unlock(&s->mu);

        uint32_t sum = 0;
        for (size_t i = 0; i < j->size; i++)
            sum = sum * 31 + j->p[i];
        j->sum = sum;
        struct timespec work = {0, 20 * 1000 * 1000}; /* pretend this is expensive */
        nanosleep(&work, NULL);

        pthread_mutex_lock(&s->mu);
        j->next = s->done;
        s->done = j;
        pthread_mutex_unlock(&s->mu);
        evsource_event_post(s->done_ev, 1); /* wake the loop */
        pthread_mutex_lock(&s->mu);
    }
    pthread_mutex_unlock(&s->mu);
    return NULL;
}

static struct client *find_client(struct server *s, int id)
{
    for (struct client *c = s->clients; c; c = c->next)
        if (c->id == id)
            return c;
    return NULL;
}

static void reply(struct client *c, uint32_t type, uint32_t arg, const char *text)
{
    struct msg m = {.type = type, .arg = arg};
    if (text)
        snprintf(m.text, sizeof m.text, "%s", text);
    /* A blocking send could stall the whole loop on a client that never
     * reads; MSG_DONTWAIT makes a full socket an error instead. What to do
     * then (queue, drop, disconnect) is stage 09's back-pressure problem. */
    if (send(c->fd, &m, sizeof m, MSG_DONTWAIT | MSG_NOSIGNAL) < 0)
        LOG("server: reply to client %d failed: %s", c->id, strerror(errno));
}

static void drop_client(struct server *s, struct client *c, const char *why)
{
    LOG("server: client %d (%s) %s", c->id, c->name[0] ? c->name : "?", why);
    evsource_remove(c->src);
    close(c->fd);
    for (struct client **pp = &s->clients; *pp; pp = &(*pp)->next)
        if (*pp == c) {
            *pp = c->next;
            break;
        }
    free(c);
}

static void handle_buffer(struct server *s, struct client *c, const struct msg *m, int fd)
{
    char why[128];
    size_t size;
    if (sealbuf_check(fd, m->arg, F_SEAL_SHRINK, &size, why, sizeof why) < 0) {
        LOG("server: client %d buffer rejected: %s", c->id, why);
        reply(c, MSG_REJECT, 0, why);
        close(fd);
        return;
    }
    const void *p = sealbuf_map_ro(fd, m->arg);
    if (!p) {
        reply(c, MSG_REJECT, 0, "mmap failed");
        close(fd);
        return;
    }
    LOG("server: client %d buffer accepted (%s); queued for the worker", c->id, why);
    struct job *j = calloc(1, sizeof *j);
    j->client_id = c->id;
    j->fd = fd;
    j->p = p;
    j->size = m->arg;
    pthread_mutex_lock(&s->mu);
    j->next = s->todo;
    s->todo = j;
    pthread_cond_signal(&s->cv);
    pthread_mutex_unlock(&s->mu);
}

static void handle_message(struct server *s, struct client *c, const struct msg *m, int *fds,
                           int nfds)
{
    switch (m->type) {
    case MSG_HELLO: {
        /* the text is untrusted: it need not be terminated */
        size_t n = strnlen(m->text, sizeof m->text);
        if (n >= sizeof c->name)
            n = sizeof c->name - 1;
        memcpy(c->name, m->text, n);
        c->name[n] = '\0';
        LOG("server: client %d says hello: \"%s\"", c->id, c->name);
        reply(c, MSG_WELCOME, (uint32_t)c->id, NULL);
        break;
    }
    case MSG_BUFFER:
        if (nfds < 1) {
            reply(c, MSG_REJECT, 0, "BUFFER without an fd");
            break;
        }
        handle_buffer(s, c, m, fds[0]);
        break;
    case MSG_PING:
        reply(c, MSG_PONG, (uint32_t)s->ticks, NULL);
        break;
    case MSG_STALL: {
        LOG("server: client %d asked the loop to block for %u ms; blocking", c->id, m->arg);
        struct timespec d = {m->arg / 1000, (long)(m->arg % 1000) * 1000000};
        nanosleep(&d, NULL); /* never do this in an event loop */
        reply(c, MSG_PONG, (uint32_t)s->ticks, NULL);
        break;
    }
    default:
        LOG("server: client %d sent unknown type %u", c->id, m->type);
        break;
    }
}

static void on_client(struct evsource *src, int fd, uint32_t events, void *data)
{
    (void)src;
    (void)events;
    struct client *c = data;
    struct server *s = c->s;
    /* Handle at most 16 messages, then return to the loop, so that one busy
     * client cannot keep the others waiting. Level-triggered epoll reports
     * the socket again if messages are left. */
    for (int i = 0; i < 16; i++) {
        struct msg m;
        int fds[FDPASS_MAX_FDS], nfds = FDPASS_MAX_FDS, flags = 0;
        ssize_t r = fdpass_recv(fd, &m, sizeof m, fds, &nfds, &flags);
        if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return;
        if (r <= 0) {
            drop_client(s, c, r == 0 ? "hung up" : strerror(errno));
            return;
        }
        if (flags & FDPASS_CTRUNC)
            LOG("server: client %d sent more fds than fit; the kernel closed the rest", c->id);
        if (r != (ssize_t)sizeof m) {
            fdpass_close_all(fds, &nfds);
            drop_client(s, c, "sent a malformed message");
            return;
        }
        if (m.type == MSG_BYE) {
            drop_client(s, c, "said bye");
            return;
        }
        handle_message(s, c, &m, fds, nfds);
    }
}

static void on_listen(struct evsource *src, int fd, uint32_t events, void *data)
{
    (void)src;
    (void)events;
    struct server *s = data;
    for (;;) {
        int cfd = accept4(fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (cfd < 0)
            return; /* EAGAIN: no more pending connections */
        pid_t pid;
        uid_t uid;
        gid_t gid;
        if (fdpass_peer(cfd, &pid, &uid, &gid) < 0 || uid != geteuid()) {
            LOG("server: refused a connection from pid %d uid %d", (int)pid, (int)uid);
            close(cfd);
            continue;
        }
        struct client *c = calloc(1, sizeof *c);
        c->s = s;
        c->id = ++s->next_id;
        c->fd = cfd;
        c->pid = pid;
        c->src = evloop_add_fd(s->loop, cfd, EPOLLIN, on_client, c);
        c->next = s->clients;
        s->clients = c;
        LOG("server: client %d connected: pid %d, uid %d", c->id, (int)pid, (int)uid);
    }
}

static void on_tick(struct evsource *src, uint64_t exp, void *data)
{
    (void)src;
    struct server *s = data;
    s->ticks += exp;
    if (exp > s->max_exp)
        s->max_exp = exp;
    if (exp > 1) {
        s->late_reads++;
        LOG("server: timerfd read returned %llu expirations: %llu vblank tick(s) went by unseen",
            (unsigned long long)exp, (unsigned long long)(exp - 1));
    }
}

static void on_jobs_done(struct evsource *src, uint64_t count, void *data)
{
    (void)src;
    struct server *s = data;
    pthread_mutex_lock(&s->mu);
    struct job *list = s->done;
    s->done = NULL;
    pthread_mutex_unlock(&s->mu);
    int n = 0;
    for (struct job *j = list; j; j = j->next)
        n++;
    LOG("server: eventfd read %llu (worker posts since the last read); %d job(s) done",
        (unsigned long long)count, n);
    while (list) {
        struct job *j = list;
        list = j->next;
        struct client *c = find_client(s, j->client_id);
        if (c) {
            char t[56];
            snprintf(t, sizeof t, "checksum %08x", j->sum);
            reply(c, MSG_OK, j->sum, t);
        } else {
            LOG("server: client %d left before its job finished; result dropped", j->client_id);
        }
        munmap((void *)j->p, j->size);
        close(j->fd);
        free(j);
    }
}

static void print_stats(struct server *s, const char *when)
{
    double secs = (now_ms() - s->started_ms) / 1e3;
    int nclients = 0;
    for (struct client *c = s->clients; c; c = c->next)
        nclients++;
    LOG("server: %s: %.2f s, %llu vblank ticks (%.0f expected), %llu late timer reads, at most "
        "%llu expirations in one read; %d client(s); open fds %d (at start %d)",
        when, secs, (unsigned long long)s->ticks, secs * 1e9 / (double)TICK_NS,
        (unsigned long long)s->late_reads, (unsigned long long)s->max_exp, nclients,
        fdpass_count_open(), s->fds_at_start);
}

static void on_signal(struct evsource *src, const struct signalfd_siginfo *si, void *data)
{
    (void)src;
    struct server *s = data;
    LOG("server: signalfd: %s from pid %d", strsignal((int)si->ssi_signo), (int)si->ssi_pid);
    if (si->ssi_signo == SIGUSR1)
        print_stats(s, "statistics");
    else
        s->running = 0;
}

static int serve(const char *path, int ready_fd)
{
    struct server s;
    memset(&s, 0, sizeof s);
    s.path = path;
    s.loop = evloop_create();
    if (!s.loop)
        die("evloop_create");

    /* Signals first: the worker thread created below inherits the blocked
     * mask, so these signals can only arrive through the signalfd. */
    if (!evloop_add_signal(s.loop, SIGINT, on_signal, &s) ||
        !evloop_add_signal(s.loop, SIGTERM, on_signal, &s) ||
        !evloop_add_signal(s.loop, SIGUSR1, on_signal, &s))
        die("signalfd");
    signal(SIGPIPE, SIG_IGN);

    s.listen_fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    struct sockaddr_un a = {.sun_family = AF_UNIX};
    snprintf(a.sun_path, sizeof a.sun_path, "%s", path);
    unlink(path);
    if (s.listen_fd < 0 || bind(s.listen_fd, (struct sockaddr *)&a, sizeof a) < 0 ||
        listen(s.listen_fd, 16) < 0)
        die("listen");
    if (!evloop_add_fd(s.loop, s.listen_fd, EPOLLIN, on_listen, &s))
        die("evloop_add_fd");

    struct evsource *timer = evloop_add_timer(s.loop, on_tick, &s);
    if (!timer || evsource_timer_arm(timer, TICK_NS, TICK_NS) < 0)
        die("timerfd");
    s.done_ev = evloop_add_event(s.loop, on_jobs_done, &s);
    if (!s.done_ev)
        die("eventfd");
    pthread_mutex_init(&s.mu, NULL);
    pthread_cond_init(&s.cv, NULL);
    if (pthread_create(&s.worker, NULL, worker_main, &s) != 0)
        die("pthread_create");

    s.running = 1;
    s.started_ms = now_ms();
    LOG("server: pid %d listening on %s; epoll fd %d watches the socket, a 60 Hz timerfd, "
        "signalfds for SIGINT, SIGTERM and SIGUSR1, and an eventfd",
        (int)getpid(), path, evloop_get_fd(s.loop));
    if (ready_fd >= 0) {
        if (write(ready_fd, "r", 1) != 1)
            die("ready");
        close(ready_fd);
    }
    s.fds_at_start = fdpass_count_open();
    while (s.running)
        if (evloop_dispatch(s.loop, -1) < 0)
            die("epoll_wait");

    print_stats(&s, "shutting down");
    pthread_mutex_lock(&s.mu);
    s.stop = 1;
    pthread_cond_signal(&s.cv);
    pthread_mutex_unlock(&s.mu);
    pthread_join(s.worker, NULL);
    struct job *lists[2] = {s.todo, s.done};
    for (int i = 0; i < 2; i++)
        while (lists[i]) {
            struct job *j = lists[i];
            lists[i] = j->next;
            munmap((void *)j->p, j->size);
            close(j->fd);
            free(j);
        }
    while (s.clients)
        drop_client(&s, s.clients, "closed at shutdown");
    evloop_destroy(s.loop);
    close(s.listen_fd);
    unlink(path);
    return 0;
}

/* ========================== the clients ================================== */

static int connect_to(const char *path)
{
    int fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    struct sockaddr_un a = {.sun_family = AF_UNIX};
    snprintf(a.sun_path, sizeof a.sun_path, "%s", path);
    if (fd < 0 || connect(fd, (struct sockaddr *)&a, sizeof a) < 0) {
        int e = errno;
        if (fd >= 0)
            close(fd);
        errno = e;
        return -1;
    }
    return fd;
}

static void csend(int fd, uint32_t type, uint32_t arg, const char *text, int *fds, int nfds)
{
    struct msg m = {.type = type, .arg = arg};
    if (text)
        snprintf(m.text, sizeof m.text, "%s", text);
    if (fdpass_send(fd, &m, sizeof m, fds, nfds) < 0)
        die("client send");
}

static int crecv(int fd, struct msg *m)
{
    ssize_t r = recv(fd, m, sizeof *m, 0);
    return r == (ssize_t)sizeof *m ? 1 : 0;
}

static int make_buffer(int sealed)
{
    int fd = sealbuf_create("evserver-client", BUF_SIZE);
    if (fd < 0)
        die("memfd_create");
    unsigned char *p = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    for (int i = 0; i < BUF_SIZE; i++)
        p[i] = (unsigned char)(i * 7);
    munmap(p, BUF_SIZE);
    if (sealed && sealbuf_seal(fd, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL) < 0)
        die("seal");
    return fd;
}

static void print_reply(const char *who, const struct msg *m)
{
    switch (m->type) {
    case MSG_WELCOME:
        LOG("client %s: welcome, I am client %u", who, m->arg);
        break;
    case MSG_OK:
        LOG("client %s: OK, %s", who, m->text);
        break;
    case MSG_REJECT:
        LOG("client %s: REJECT, %s", who, m->text);
        break;
    case MSG_PONG:
        LOG("client %s: PONG, server's vblank tick count %u", who, m->arg);
        break;
    default:
        LOG("client %s: reply type %u", who, m->type);
    }
}

static int client(const char *path, const char *kind)
{
    struct msg m;
    if (strcmp(kind, "foreign") == 0) {
        if (geteuid() != 0) {
            LOG("client foreign: not root, cannot switch uid; skipped");
            return 0;
        }
        if (setgid(65534) < 0 || setuid(65534) < 0)
            die("setuid");
    }
    int fd = connect_to(path);
    if (fd < 0) {
        LOG("client %s (uid %d): connect: %s", kind, (int)getuid(), strerror(errno));
        return 0;
    }
    csend(fd, MSG_HELLO, 0, kind, NULL, 0);
    if (!crecv(fd, &m)) {
        LOG("client %s: the server closed the connection", kind);
        return 0;
    }
    print_reply(kind, &m);

    if (strcmp(kind, "good") == 0) {
        int b = make_buffer(1);
        csend(fd, MSG_BUFFER, BUF_SIZE, NULL, &b, 1);
        close(b);
        if (crecv(fd, &m))
            print_reply(kind, &m);
        csend(fd, MSG_PING, 0, NULL, NULL, 0);
        if (crecv(fd, &m))
            print_reply(kind, &m);
    } else if (strcmp(kind, "unsealed") == 0) {
        int b = make_buffer(0);
        csend(fd, MSG_BUFFER, BUF_SIZE, NULL, &b, 1);
        if (crecv(fd, &m))
            print_reply(kind, &m);
        LOG("client %s: now claiming twice the size of a sealed buffer", kind);
        close(b);
        b = make_buffer(1);
        csend(fd, MSG_BUFFER, 2 * BUF_SIZE, NULL, &b, 1);
        if (crecv(fd, &m))
            print_reply(kind, &m);
        close(b);
    } else if (strcmp(kind, "stall") == 0) {
        /* Three buffers (20 ms of worker time each), then a request that
         * blocks the loop for 100 ms. The worker finishes all three while
         * the loop is blocked. */
        for (int i = 0; i < 3; i++) {
            int b = make_buffer(1);
            csend(fd, MSG_BUFFER, BUF_SIZE, NULL, &b, 1);
            close(b);
        }
        csend(fd, MSG_STALL, 100, NULL, NULL, 0);
        for (int i = 0; i < 4 && crecv(fd, &m); i++)
            print_reply(kind, &m);
    } else if (strcmp(kind, "crash") == 0) {
        int b = make_buffer(1);
        csend(fd, MSG_BUFFER, BUF_SIZE, NULL, &b, 1);
        LOG("client %s: sent a buffer, now exiting without waiting or saying bye", kind);
        _exit(1);
    } else if (strcmp(kind, "fdflood") == 0) {
        int extra[2] = {open("/dev/null", O_RDONLY | O_CLOEXEC),
                        open("/dev/null", O_RDONLY | O_CLOEXEC)};
        int n = 0;
        for (int i = 0; i < 200; i++) {
            csend(fd, MSG_PING, 0, NULL, extra, 2);
            if (crecv(fd, &m))
                n++;
        }
        LOG("client %s: sent 200 PINGs with 2 fds attached to each; %d PONGs came back", kind,
            n);
        close(extra[0]);
        close(extra[1]);
    }
    csend(fd, MSG_BYE, 0, NULL, NULL, 0);
    close(fd);
    return 0;
}

/* ========================== the demo driver ============================== */

static void run_client(const char *path, const char *kind)
{
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0)
        die("fork");
    if (pid == 0) {
        client(path, kind);
        fflush(stdout);
        _exit(0);
    }
    waitpid(pid, NULL, 0);
    struct timespec gap = {0, 50 * 1000 * 1000}; /* let the server's log catch up */
    nanosleep(&gap, NULL);
}

static int demo(void)
{
    char dir[] = "/tmp/lab08-evserver-XXXXXX";
    if (!mkdtemp(dir))
        die("mkdtemp");
    char path[128];
    snprintf(path, sizeof path, "%s/evserver.sock", dir);
    int ready[2];
    if (pipe2(ready, O_CLOEXEC) < 0)
        die("pipe");
    fflush(stdout);
    pid_t srv = fork();
    if (srv == 0) {
        close(ready[0]);
        _exit(serve(path, ready[1]));
    }
    close(ready[1]);
    char c;
    if (read(ready[0], &c, 1) != 1)
        die("server did not start");
    close(ready[0]);

    const char *kinds[] = {"good", "unsealed", "foreign", "stall", "crash", "fdflood"};
    for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; i++) {
        LOG("demo: --- client \"%s\" ---", kinds[i]);
        run_client(path, kinds[i]);
    }
    LOG("demo: --- kill -USR1, then kill -TERM ---");
    kill(srv, SIGUSR1);
    struct timespec gap = {0, 200 * 1000 * 1000};
    nanosleep(&gap, NULL);
    kill(srv, SIGTERM);
    int st;
    waitpid(srv, &st, 0);
    LOG("demo: server exited with status %d", WIFEXITED(st) ? WEXITSTATUS(st) : -1);
    rmdir(dir);
    return 0;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    if (argc >= 2 && strcmp(argv[1], "demo") == 0)
        return demo();
    if (argc >= 3 && strcmp(argv[1], "serve") == 0)
        return serve(argv[2], -1);
    if (argc >= 4 && strcmp(argv[1], "client") == 0)
        return client(argv[2], argv[3]);
    fprintf(stderr, "usage: %s demo | serve PATH | client PATH KIND\n", argv[0]);
    return 2;
}
