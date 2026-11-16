/*
 * clientlib.c - Lab 09: the code the harness clients share. See clientlib.h.
 */
#define _GNU_SOURCE
#include "clientlib.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

int64_t lab_now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void lab_log(const char *tag, const char *fmt, ...)
{
    va_list ap;
    printf("%s ", tag);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    fflush(stdout);
}

const char *lab_error_name(uint32_t code)
{
    switch (code) {
    case LAB09_ERROR_INVALID_OBJECT: return "invalid_object";
    case LAB09_ERROR_INVALID_METHOD: return "invalid_method";
    case LAB09_ERROR_INVALID_MESSAGE: return "invalid_message";
    case LAB09_ERROR_INVALID_ID: return "invalid_id";
    case LAB09_ERROR_NO_FD: return "no_fd";
    case LAB09_ERROR_BAD_BUFFER: return "bad_buffer";
    case LAB09_ERROR_BAD_FORMAT: return "bad_format";
    case LAB09_ERROR_NO_RESOURCES: return "no_resources";
    case LAB09_ERROR_VERSION: return "version";
    default: return "unknown";
    }
}

int lab_connect(struct lab_conn *c, const char *path, const char *tag, int timeout_ms)
{
    struct sockaddr_un sa;
    int64_t deadline = lab_now_us() + (int64_t)timeout_ms * 1000;

    memset(c, 0, sizeof *c);
    c->tag = tag;
    c->fd = -1;
    if (strlen(path) >= sizeof sa.sun_path) {
        lab_log(tag, "connect-failed reason=path-too-long");
        return -1;
    }
    memset(&sa, 0, sizeof sa);
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, path);
    for (;;) {
        int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0)
            return -1;
        if (connect(fd, (struct sockaddr *)&sa, sizeof sa) == 0) {
            c->fd = fd;
            return 0;
        }
        int err = errno;
        close(fd);
        if ((err != ENOENT && err != ECONNREFUSED) || lab_now_us() > deadline) {
            lab_log(tag, "connect-failed errno=%d (%s)", err, strerror(err));
            return -1;
        }
        usleep(20000);
    }
}

int lab_send_raw(struct lab_conn *c, const void *data, size_t len, int fd)
{
    const uint8_t *p = data;
    int fd_pending = fd >= 0;

    while (len > 0) {
        struct iovec iov = { (void *)p, len };
        struct msghdr msg;
        union {
            struct cmsghdr h;
            char buf[CMSG_SPACE(sizeof(int))];
        } ctl;
        memset(&msg, 0, sizeof msg);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        if (fd_pending) {
            /* The fd rides on the first byte of the message that needs it. */
            memset(&ctl, 0, sizeof ctl);
            msg.msg_control = ctl.buf;
            msg.msg_controllen = sizeof ctl.buf;
            struct cmsghdr *cm = CMSG_FIRSTHDR(&msg);
            cm->cmsg_level = SOL_SOCKET;
            cm->cmsg_type = SCM_RIGHTS;
            cm->cmsg_len = CMSG_LEN(sizeof(int));
            memcpy(CMSG_DATA(cm), &fd, sizeof(int));
        }
        /* MSG_NOSIGNAL: a closed server gives EPIPE, not a SIGPIPE.
         * EINTR returns too, so that a signal handler's flag is seen. */
        ssize_t n = sendmsg(c->fd, &msg, MSG_NOSIGNAL);
        if (n < 0)
            return -1;
        fd_pending = 0;
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

void lab_put(uint8_t *buf, size_t *len, uint32_t object, uint32_t opcode,
             const uint32_t *args, int nargs)
{
    uint32_t w[2] = { object, LAB09_WORD1(LAB09_HEADER_SIZE + 4 * nargs, opcode) };
    memcpy(buf + *len, w, sizeof w);
    *len += sizeof w;
    if (nargs > 0) {
        memcpy(buf + *len, args, 4 * (size_t)nargs);
        *len += 4 * (size_t)nargs;
    }
}

int lab_send(struct lab_conn *c, uint32_t object, uint32_t opcode,
             const uint32_t *args, int nargs, int fd)
{
    uint8_t buf[LAB09_MAX_MESSAGE];
    size_t len = 0;
    lab_put(buf, &len, object, opcode, args, nargs);
    return lab_send_raw(c, buf, len, fd);
}

int lab_next_event(struct lab_conn *c, struct lab_event *ev, int timeout_ms)
{
    int64_t deadline = timeout_ms < 0 ? -1 : lab_now_us() + (int64_t)timeout_ms * 1000;

    for (;;) {
        if (c->in_len >= LAB09_HEADER_SIZE) {
            uint32_t w[2];
            memcpy(w, c->in, sizeof w);
            uint32_t size = LAB09_SIZE(w[1]);
            if (size < LAB09_HEADER_SIZE || size % 4 != 0 || size > LAB09_MAX_MESSAGE) {
                lab_log(c->tag, "bad-event object=%u word1=0x%08x", w[0], w[1]);
                return -1;
            }
            if (c->in_len >= size) {
                ev->object = w[0];
                ev->opcode = LAB09_OPCODE(w[1]);
                ev->size = size;
                ev->nargs = (size - LAB09_HEADER_SIZE) / 4;
                memcpy(ev->arg, c->in + LAB09_HEADER_SIZE, size - LAB09_HEADER_SIZE);
                memmove(c->in, c->in + size, c->in_len - size);
                c->in_len -= size;
                return 1;
            }
        }
        int wait = -1;      /* timeout_ms 0: look once, do not wait */
        if (deadline >= 0) {
            int64_t left = deadline - lab_now_us();
            wait = left <= 0 ? 0 : (int)((left + 999) / 1000);
        }
        struct pollfd pfd = { c->fd, POLLIN, 0 };
        int r = poll(&pfd, 1, wait);
        if (r < 0) {
            if (errno == EINTR)
                return 0;
            return -1;
        }
        if (r == 0)
            return 0;
        ssize_t n = recv(c->fd, c->in + c->in_len, sizeof c->in - c->in_len, 0);
        if (n < 0 && (errno == EINTR || errno == EAGAIN))
            continue;
        if (n <= 0)
            return -1;      /* EOF, or ECONNRESET when the server closed with our requests unread */
        c->in_len += (size_t)n;
    }
}

void lab_log_event(const struct lab_conn *c, const char *what, const struct lab_event *ev)
{
    if (ev->object == LAB09_DISPLAY_ID && ev->opcode == LAB09_DISPLAY_ERROR && ev->nargs >= 2)
        lab_log(c->tag, "%s error object=%u code=%u name=%s", what, ev->arg[0], ev->arg[1],
                lab_error_name(ev->arg[1]));
    else
        lab_log(c->tag, "%s event object=%u opcode=%u size=%u", what, ev->object, ev->opcode, ev->size);
}

int lab_hello(struct lab_conn *c, uint32_t version, int timeout_ms)
{
    struct lab_event ev;
    uint32_t a[1] = { version };
    if (lab_send(c, LAB09_DISPLAY_ID, LAB09_DISPLAY_HELLO, a, 1, -1) < 0) {
        lab_log(c->tag, "hello-failed errno=%d", errno);
        return -1;
    }
    int r = lab_next_event(c, &ev, timeout_ms);
    if (r <= 0) {
        lab_log(c->tag, "no-welcome reason=%s", r == 0 ? "timeout" : "closed");
        return -1;
    }
    if (ev.object != LAB09_DISPLAY_ID || ev.opcode != LAB09_DISPLAY_WELCOME || ev.nargs != 4) {
        lab_log_event(c, "no-welcome", &ev);
        return -1;
    }
    c->version = ev.arg[0];
    c->width = ev.arg[1];
    c->height = ev.arg[2];
    c->refresh_mhz = ev.arg[3];
    lab_log(c->tag, "welcome asked=%u version=%u width=%u height=%u refresh_mhz=%u",
            version, c->version, c->width, c->height, c->refresh_mhz);
    return 0;
}

int lab_shm_create(struct lab_shm *b, const char *name, int32_t offset,
                   int32_t width, int32_t height, int32_t stride,
                   uint32_t format, size_t file_size, int sealing)
{
    memset(b, 0, sizeof *b);
    b->offset = offset;
    b->width = width;
    b->height = height;
    b->stride = stride;
    b->format = format;
    b->size = file_size ? file_size : (size_t)offset + (size_t)stride * (size_t)height;
    b->fd = memfd_create(name, MFD_CLOEXEC | (sealing == LAB_SEALED ? MFD_ALLOW_SEALING : 0));
    if (b->fd < 0)
        return -1;
    if (ftruncate(b->fd, (off_t)b->size) < 0)
        return -1;
    /* Map what the buffer says it needs, even when the file is smaller:
     * only a client that wants to be bad does that, and it never touches
     * the missing part. */
    size_t need = (size_t)offset + (size_t)stride * (size_t)height;
    size_t maplen = need > b->size ? need : b->size;
    b->map = mmap(NULL, maplen, PROT_READ | PROT_WRITE, MAP_SHARED, b->fd, 0);
    if (b->map == MAP_FAILED)
        return -1;
    if (sealing == LAB_SEALED &&
        fcntl(b->fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL) < 0)
        return -1;
    return 0;
}

uint32_t *lab_shm_row(const struct lab_shm *b, int y)
{
    return (uint32_t *)(b->map + b->offset + (size_t)y * (size_t)b->stride);
}

int lab_sync(struct lab_conn *c, uint32_t serial)
{
    uint32_t a[1] = { serial };
    return lab_send(c, LAB09_DISPLAY_ID, LAB09_DISPLAY_SYNC, a, 1, -1);
}

int lab_create_surface(struct lab_conn *c, uint32_t id)
{
    uint32_t a[1] = { id };
    return lab_send(c, LAB09_DISPLAY_ID, LAB09_DISPLAY_CREATE_SURFACE, a, 1, -1);
}

int lab_create_buffer(struct lab_conn *c, uint32_t id, const struct lab_shm *b)
{
    uint32_t a[6] = { id, (uint32_t)b->offset, (uint32_t)b->width, (uint32_t)b->height,
                      (uint32_t)b->stride, b->format };
    return lab_send(c, LAB09_DISPLAY_ID, LAB09_DISPLAY_CREATE_BUFFER, a, 6, b->fd);
}

int lab_attach(struct lab_conn *c, uint32_t surface, uint32_t buffer)
{
    uint32_t a[1] = { buffer };
    return lab_send(c, surface, LAB09_SURFACE_ATTACH, a, 1, -1);
}

int lab_set_position(struct lab_conn *c, uint32_t surface, int32_t x, int32_t y, int32_t z)
{
    uint32_t a[3] = { (uint32_t)x, (uint32_t)y, (uint32_t)z };
    return lab_send(c, surface, LAB09_SURFACE_SET_POSITION, a, 3, -1);
}

int lab_damage(struct lab_conn *c, uint32_t surface, int32_t x, int32_t y, int32_t w, int32_t h)
{
    uint32_t a[4] = { (uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h };
    return lab_send(c, surface, LAB09_SURFACE_DAMAGE, a, 4, -1);
}

int lab_frame(struct lab_conn *c, uint32_t surface)
{
    return lab_send(c, surface, LAB09_SURFACE_FRAME, NULL, 0, -1);
}

int lab_commit(struct lab_conn *c, uint32_t surface)
{
    return lab_send(c, surface, LAB09_SURFACE_COMMIT, NULL, 0, -1);
}

int lab_destroy(struct lab_conn *c, uint32_t object)
{
    return lab_send(c, object, 0, NULL, 0, -1);
}
