/*
 * fdpass.h - Stage 08: pass file descriptors over a Unix-domain socket and
 * ask who is on the other end.
 *
 * A small, self-contained helper meant to be copied into later work (the
 * stage 09 display server needs exactly these calls). Build it together
 * with your program:
 *
 *     gcc -O2 -Wall -o prog prog.c fdpass.c
 *
 * What the calls do, in unix(7) terms:
 *  - fdpass_send() sends some bytes and, in the same sendmsg(2), up to
 *    FDPASS_MAX_FDS descriptors as one SCM_RIGHTS control message. The
 *    kernel installs new descriptors in the receiver that refer to the same
 *    open file descriptions: same file offset, same status flags, same
 *    access mode. Passing an fd hands over a capability, not a name.
 *  - fdpass_recv() receives with MSG_CMSG_CLOEXEC, so received descriptors
 *    are not leaked into programs the receiver later execs. Every received
 *    descriptor now belongs to the caller, including ones it did not want:
 *    close them (see exercise C of the lab).
 *  - fdpass_peer() reads SO_PEERCRED: the pid, uid and gid the peer had
 *    when it called connect(2) or socketpair(2), not when it sends.
 *
 * On a SOCK_STREAM socket you must send at least one byte with the control
 * message, and the receiver gets the descriptors with the bytes they came
 * with; the byte stream itself carries no message boundaries. The lab
 * programs use SOCK_SEQPACKET, which keeps one sendmsg() as one message.
 */
#ifndef LAB08_FDPASS_H
#define LAB08_FDPASS_H

#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>

/* The most descriptors one call sends or receives. The kernel's own limit
 * per message is SCM_MAX_FD (253 in Linux); stay far below it. */
#define FDPASS_MAX_FDS 8

/* Flags fdpass_recv() reports in *flags_out (a copy of msg_flags). */
#define FDPASS_CTRUNC MSG_CTRUNC /* more descriptors came than fit: the kernel closed the rest */
#define FDPASS_TRUNC MSG_TRUNC   /* the message was longer than len (SOCK_SEQPACKET) */

/* Send len bytes from buf (len >= 1) and nfds descriptors (0..FDPASS_MAX_FDS)
 * in one sendmsg(). Returns the number of bytes sent, or -1 with errno set.
 * The caller still owns its descriptors and may close them afterwards. */
ssize_t fdpass_send(int sock, const void *buf, size_t len, const int *fds, int nfds);

/* Receive one message: up to len bytes into buf and up to *nfds descriptors
 * (at most FDPASS_MAX_FDS) into fds. On return *nfds is the number of
 * descriptors received, and *flags_out (if not NULL) holds FDPASS_CTRUNC
 * and FDPASS_TRUNC as they occurred. Returns the byte count (0 at end of
 * stream), or -1 with errno set (EAGAIN on a non-blocking socket with no
 * message). */
ssize_t fdpass_recv(int sock, void *buf, size_t len, int *fds, int *nfds, int *flags_out);

/* The peer's credentials from SO_PEERCRED. Returns 0 or -1 with errno. */
int fdpass_peer(int sock, pid_t *pid, uid_t *uid, gid_t *gid);

/* Close every descriptor in fds[0..n-1] and set n to 0. */
void fdpass_close_all(int *fds, int *n);

/* The number of open descriptors in this process (entries in /proc/self/fd,
 * not counting the one used to list them), or -1. Handy for leak checks. */
int fdpass_count_open(void);

#endif
