/*
 * good-alpha.c - Lab 09, well-behaved client B. Two surfaces from one
 * connection, drawn once:
 *   surface 20: 128x96 at (96, 64), z 1, translucent premultiplied
 *               ARGB8888, in a buffer that starts 4096 bytes into its memfd
 *               (a server that ignores the offset shows the wrong rows);
 *   surface 21: 64x48 at (232, 16), z 2, XRGB8888 whose unused top byte
 *               holds varying junk (a server that blends it as alpha shows
 *               holes).
 * It says hello with version 7, as a newer client would, and must get
 * welcome with a version from 1 to 7. Then it commits surface 20 once per
 * frame_done, as a second paced client, until "quit" arrives on stdin.
 *
 * usage: good-alpha SOCKET
 */
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "clientlib.h"

#define S1 20
#define S2 21

/* check.py's pattern_b1() and pattern_b2() must compute the same values. */
static uint32_t pattern_b1(int x, int y)
{
    uint32_t a = 0x80u | ((uint32_t)(x ^ y) & 0x3f);   /* 128..191 */
    uint32_t r = a * (uint32_t)x / 128;                 /* x < 128, so r < a */
    uint32_t g = a / 4;
    uint32_t b = a * (uint32_t)y / 96;                  /* y < 96, so b < a */
    return a << 24 | r << 16 | g << 8 | b;
}

static uint32_t pattern_b2(int x, int y)
{
    uint32_t junk = (uint32_t)(x * 7 + y) & 0xff;
    return junk << 24 | (uint32_t)(255 - 2 * x) << 16 | (uint32_t)(4 * y) << 8 | 0x80u;
}

int main(int argc, char **argv)
{
    struct lab_conn c;
    struct lab_shm b1, b2;
    int quitting = 0, stdin_open = 1;
    unsigned long dones = 0, releases = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: %s SOCKET\n", argv[0]);
        return 2;
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (lab_connect(&c, argv[1], "B", 5000) < 0 || lab_hello(&c, 7, 2000) < 0)
        return 1;
    if (c.version < 1 || c.version > 7)
        lab_log("B", "bad-version got=%u asked=7", c.version);

    if (lab_shm_create(&b1, "lab09-B1", 4096, 128, 96, 128 * 4, LAB09_FORMAT_ARGB8888, 0,
                       LAB_SEALED) < 0 ||
        lab_shm_create(&b2, "lab09-B2", 0, 64, 48, 64 * 4, LAB09_FORMAT_XRGB8888, 0,
                       LAB_SEALED) < 0) {
        perror("B: memfd");
        return 1;
    }
    memset(b1.map, 0x5a, 4096);     /* the bytes before the offset are not pixels */
    for (int y = 0; y < 96; y++)
        for (int x = 0; x < 128; x++)
            lab_shm_row(&b1, y)[x] = pattern_b1(x, y);
    for (int y = 0; y < 48; y++)
        for (int x = 0; x < 64; x++)
            lab_shm_row(&b2, y)[x] = pattern_b2(x, y);

    lab_create_buffer(&c, 22, &b1);
    lab_create_buffer(&c, 23, &b2);
    lab_create_surface(&c, S1);
    lab_create_surface(&c, S2);
    lab_set_position(&c, S1, 96, 64, 1);
    lab_attach(&c, S1, 22);
    lab_damage(&c, S1, 0, 0, 128, 96);
    lab_frame(&c, S1);
    lab_commit(&c, S1);
    lab_set_position(&c, S2, 232, 16, 2);
    lab_attach(&c, S2, 23);
    lab_damage(&c, S2, 0, 0, 64, 48);
    lab_commit(&c, S2);

    while (!quitting) {
        struct pollfd pfd[2] = { { c.fd, POLLIN, 0 }, { 0, POLLIN, 0 } };
        if (poll(pfd, stdin_open ? 2 : 1, -1) < 0 && errno != EINTR)
            break;
        if (stdin_open && (pfd[1].revents & (POLLIN | POLLHUP))) {
            char buf[64];
            ssize_t r = read(0, buf, sizeof buf - 1);
            if (r <= 0)
                stdin_open = 0;
            else if (memmem(buf, (size_t)r, "quit", 4))
                quitting = 1;
        }
        if (quitting || !(pfd[0].revents & (POLLIN | POLLHUP | POLLERR)))
            continue;
        struct lab_event ev;
        int r;
        while ((r = lab_next_event(&c, &ev, 0)) == 1) {
            if (ev.object == S1 && ev.opcode == LAB09_SURFACE_FRAME_DONE && ev.nargs == 3) {
                long long vblank_us = (long long)(((uint64_t)ev.arg[1] << 32) | ev.arg[2]);
                if (dones++ == 0)
                    lab_log("B", "shown seq=%u", ev.arg[0]);
                lab_log("B", "done seq=%u vblank_us=%lld recv_us=%lld", ev.arg[0], vblank_us,
                        (long long)lab_now_us());
                lab_frame(&c, S1);
                lab_commit(&c, S1);
            } else if (ev.opcode == LAB09_BUFFER_RELEASE && (ev.object == 22 || ev.object == 23)) {
                releases++;
                lab_log("B", "release buffer=%u", ev.object);
            } else {
                lab_log_event(&c, "unexpected", &ev);
                if (ev.object == LAB09_DISPLAY_ID && ev.opcode == LAB09_DISPLAY_ERROR)
                    return 1;
            }
        }
        if (r < 0) {
            lab_log("B", "disconnected dones=%lu", dones);
            return 1;
        }
    }
    lab_destroy(&c, S1);
    lab_destroy(&c, S2);
    lab_destroy(&c, 22);
    lab_destroy(&c, 23);
    lab_log("B", "summary dones=%lu releases=%lu", dones, releases);
    close(c.fd);
    return 0;
}
