/*
 * fdpass.c - Stage 08: SCM_RIGHTS and SO_PEERCRED helpers. See fdpass.h.
 */
#define _GNU_SOURCE /* struct ucred, MSG_CMSG_CLOEXEC */
#include "fdpass.h"

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

ssize_t fdpass_send(int sock, const void *buf, size_t len, const int *fds, int nfds)
{
    if (len == 0 || nfds < 0 || nfds > FDPASS_MAX_FDS) {
        errno = EINVAL;
        return -1;
    }
    struct iovec iov = {.iov_base = (void *)buf, .iov_len = len};
    struct msghdr msg = {.msg_iov = &iov, .msg_iovlen = 1};
    /* The control buffer must be aligned for struct cmsghdr; the union
     * guarantees that, as the cmsg(3) example does. */
    union {
        char buf[CMSG_SPACE(sizeof(int) * FDPASS_MAX_FDS)];
        struct cmsghdr align;
    } ctl;
    if (nfds > 0) {
        memset(&ctl, 0, sizeof ctl);
        msg.msg_control = ctl.buf;
        msg.msg_controllen = CMSG_SPACE(sizeof(int) * nfds);
        struct cmsghdr *c = CMSG_FIRSTHDR(&msg);
        c->cmsg_level = SOL_SOCKET;
        c->cmsg_type = SCM_RIGHTS;
        c->cmsg_len = CMSG_LEN(sizeof(int) * nfds);
        memcpy(CMSG_DATA(c), fds, sizeof(int) * nfds);
    }
    ssize_t r;
    do
        r = sendmsg(sock, &msg, MSG_NOSIGNAL);
    while (r < 0 && errno == EINTR);
    return r;
}

ssize_t fdpass_recv(int sock, void *buf, size_t len, int *fds, int *nfds, int *flags_out)
{
    int want = (nfds && fds) ? *nfds : 0;
    if (want < 0 || want > FDPASS_MAX_FDS) {
        errno = EINVAL;
        return -1;
    }
    struct iovec iov = {.iov_base = buf, .iov_len = len};
    struct msghdr msg = {.msg_iov = &iov, .msg_iovlen = 1};
    union {
        char buf[CMSG_SPACE(sizeof(int) * FDPASS_MAX_FDS)];
        struct cmsghdr align;
    } ctl;
    if (want > 0) {
        msg.msg_control = ctl.buf;
        msg.msg_controllen = CMSG_SPACE(sizeof(int) * want);
    }
    ssize_t r;
    do
        r = recvmsg(sock, &msg, MSG_CMSG_CLOEXEC);
    while (r < 0 && errno == EINTR);
    if (nfds)
        *nfds = 0;
    if (r < 0)
        return r;
    if (flags_out)
        *flags_out = msg.msg_flags & (MSG_CTRUNC | MSG_TRUNC);
    int got = 0;
    for (struct cmsghdr *c = CMSG_FIRSTHDR(&msg); c; c = CMSG_NXTHDR(&msg, c)) {
        if (c->cmsg_level != SOL_SOCKET || c->cmsg_type != SCM_RIGHTS)
            continue;
        int n = (int)((c->cmsg_len - CMSG_LEN(0)) / sizeof(int));
        int tmp[FDPASS_MAX_FDS];
        if (n > FDPASS_MAX_FDS)
            n = FDPASS_MAX_FDS; /* cannot happen: the buffer holds no more */
        memcpy(tmp, CMSG_DATA(c), sizeof(int) * n);
        for (int i = 0; i < n; i++) {
            if (got < want)
                fds[got++] = tmp[i];
            else
                close(tmp[i]); /* never leave a received fd unowned */
        }
    }
    if (nfds)
        *nfds = got;
    return r;
}

int fdpass_peer(int sock, pid_t *pid, uid_t *uid, gid_t *gid)
{
    struct ucred cr;
    socklen_t len = sizeof cr;
    if (getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &cr, &len) < 0)
        return -1;
    if (pid)
        *pid = cr.pid;
    if (uid)
        *uid = cr.uid;
    if (gid)
        *gid = cr.gid;
    return 0;
}

void fdpass_close_all(int *fds, int *n)
{
    for (int i = 0; i < *n; i++)
        if (fds[i] >= 0)
            close(fds[i]);
    *n = 0;
}

int fdpass_count_open(void)
{
    DIR *d = opendir("/proc/self/fd");
    if (!d)
        return -1;
    int n = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL)
        if (e->d_name[0] != '.')
            n++;
    closedir(d);
    return n - 1; /* the directory stream's own descriptor */
}
