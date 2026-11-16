/*
 * bad-noread.c - Lab 09, bad client 2: never reads its socket. Two modes.
 *
 * flood (log tag N): it says hello but never reads the welcome, shows a
 *   48x48 cyan surface at (120, 176), z 3, and then writes as fast as it
 *   can: blocks of 340 sync requests (each asks for a done event) followed
 *   by a frame request and a commit (each asks for a frame_done). Every
 *   event the server sends back piles up, first in this socket's receive
 *   buffer, then wherever the server keeps what it could not send.
 *   A server that writes with a blocking call stops at the first full
 *   buffer, and every other client stops getting frames. A server that
 *   queues without limit grows until something kills it. It must either
 *   stop reading this client (then this client blocks in sendmsg) or
 *   disconnect it (then sendmsg fails with EPIPE or ECONNRESET).
 *
 * shutrd (log tag R): it shuts down its own read side with
 *   shutdown(SHUT_RD) before it says hello, then trickles sync requests.
 *   On Linux that also marks the server's end of the connection as unable
 *   to send, so the server's first reply, the welcome, fails with EPIPE,
 *   and raises SIGPIPE as well unless the server ignores SIGPIPE or sends
 *   with MSG_NOSIGNAL (unix(7)). The default action of SIGPIPE ends the
 *   process: the server dies.
 *
 * usage: bad-noread SOCKET [flood|shutrd]      (default flood)
 * Runs until the server disconnects it or a SIGTERM arrives, then prints
 * how many bytes it wrote and whether its last write was blocked.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "clientlib.h"

#define SURFACE 40
#define SYNCS 340

static volatile sig_atomic_t stop;

static void on_term(int sig)
{
    (void)sig;
    stop = 1;
}

int main(int argc, char **argv)
{
    struct lab_conn c;
    struct lab_shm b;
    static uint8_t block[(SYNCS + 2) * 12];
    size_t len = 0;
    unsigned long long sent = 0;
    int64_t t0, last_report, send_start = 0;

    if (argc < 2 || argc > 3 || (argc == 3 && strcmp(argv[2], "flood") && strcmp(argv[2], "shutrd"))) {
        fprintf(stderr, "usage: %s SOCKET [flood|shutrd]\n", argv[0]);
        return 2;
    }
    int shutrd = argc == 3 && strcmp(argv[2], "shutrd") == 0;
    const char *tag = shutrd ? "R" : "N";
    setvbuf(stdout, NULL, _IOLBF, 0);
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_term;     /* no SA_RESTART: a blocked sendmsg returns EINTR */
    sigaction(SIGTERM, &sa, NULL);

    if (lab_connect(&c, argv[1], tag, 5000) < 0)
        return 1;
    if (shutrd && shutdown(c.fd, SHUT_RD) == 0)
        lab_log(tag, "read-side-shut");
    uint32_t v[1] = { 1 };
    lab_send(&c, LAB09_DISPLAY_ID, LAB09_DISPLAY_HELLO, v, 1, -1);
    if (!shutrd) {
        if (lab_shm_create(&b, "lab09-N", 0, 48, 48, 48 * 4, LAB09_FORMAT_ARGB8888, 0,
                           LAB_SEALED) < 0) {
            perror("N: memfd");
            return 1;
        }
        for (int y = 0; y < 48; y++)
            for (int x = 0; x < 48; x++)
                lab_shm_row(&b, y)[x] = 0xff00ffffu;
        lab_create_buffer(&c, 41, &b);
        lab_create_surface(&c, SURFACE);
        lab_set_position(&c, SURFACE, 120, 176, 3);
        lab_attach(&c, SURFACE, 41);
        lab_damage(&c, SURFACE, 0, 0, 48, 48);
        lab_commit(&c, SURFACE);
    }
    lab_log(tag, "flooding syncs_per_block=%d", SYNCS);

    for (uint32_t i = 0; i < SYNCS; i++) {
        uint32_t a[1] = { i };
        lab_put(block, &len, LAB09_DISPLAY_ID, LAB09_DISPLAY_SYNC, a, 1);
    }
    if (!shutrd) {
        lab_put(block, &len, SURFACE, LAB09_SURFACE_FRAME, NULL, 0);
        lab_put(block, &len, SURFACE, LAB09_SURFACE_COMMIT, NULL, 0);
    }

    t0 = last_report = lab_now_us();
    const char *how = "stopped";
    while (!stop) {
        send_start = lab_now_us();
        ssize_t n = send(c.fd, block, len, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            how = errno == EPIPE ? "disconnected-epipe" :
                  errno == ECONNRESET ? "disconnected-econnreset" : "send-error";
            break;
        }
        sent += (unsigned long long)n;
        if (n != (ssize_t)len) {
            /* finish the block so the stream stays framed */
            size_t off = (size_t)n;
            while (off < len && !stop) {
                ssize_t m = send(c.fd, block + off, len - off, MSG_NOSIGNAL);
                if (m < 0) {
                    if (errno == EINTR)
                        continue;
                    break;
                }
                off += (size_t)m;
                sent += (unsigned long long)m;
            }
        }
        int64_t now = lab_now_us();
        if (now - last_report > 250000) {
            lab_log(tag, "progress sent_bytes=%llu ms=%lld", sent, (long long)(now - t0) / 1000);
            last_report = now;
        }
        if (shutrd)
            usleep(10000);      /* a trickle is enough here */
    }
    int64_t now = lab_now_us();
    long long blocked_ms = stop ? (long long)(now - send_start) / 1000 : 0;
    lab_log(tag, "end how=%s sent_bytes=%llu ms=%lld last_send_blocked_ms=%lld", how, sent,
            (long long)(now - t0) / 1000, blocked_ms);
    return 0;
}
