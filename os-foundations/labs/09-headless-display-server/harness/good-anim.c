/*
 * good-anim.c - Lab 09, well-behaved client A. One opaque 160x120 surface
 * at (16, 16), z 0, with two buffers. It commits once per frame_done, so
 * the server's frame events pace it, and it changes its picture when
 * check.py asks on stdin. Its frame_done times are what check.py measures
 * the server's cadence with.
 *
 * usage: good-anim SOCKET
 * stdin, one command per line, each applied at the next frame_done:
 *   tag K full      draw picture K into the free buffer, attach it, damage all of it
 *   tag K partial   draw picture K into the rectangle (40, 30) 32x24 of the buffer
 *                   shown now, attach that same buffer again, damage only that rectangle
 *   quit            destroy the surface and the buffers, print a summary, exit
 * stdout: welcome, done (every frame_done), shown (the frame_done of a
 * commit that changed the picture), release, summary, and anything wrong.
 *
 * The buffers have a stride of 168 pixels for 160 visible ones, and the
 * padding holds a colour that never belongs in a frame: a server that
 * assumes stride == 4 * width shows it.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clientlib.h"

#define W 160
#define H 120
#define STRIDE_PX 168
#define SURFACE 10
#define PART_X 40
#define PART_Y 30
#define PART_W 32
#define PART_H 24

/* check.py's pattern_a() must compute the same values. */
static uint32_t pattern_a(int tag, int x, int y)
{
    uint32_t r = (uint32_t)(x * 3 + tag * 40) & 0xff;
    uint32_t g = (uint32_t)(y * 2) & 0xff;
    uint32_t b = (((x >> 3) ^ (y >> 3)) & 1) ? 0xc0 : ((uint32_t)(tag * 50) & 0xff);
    return 0xff000000u | r << 16 | g << 8 | b;
}

static void draw(struct lab_shm *b, int tag, int x0, int y0, int w, int h)
{
    for (int y = y0; y < y0 + h; y++) {
        uint32_t *row = lab_shm_row(b, y);
        for (int x = x0; x < x0 + w; x++)
            row[x] = pattern_a(tag, x, y);
        if (x0 == 0 && w == W)
            for (int x = W; x < STRIDE_PX; x++)
                row[x] = 0xffff00ffu;       /* padding: opaque magenta */
    }
}

int main(int argc, char **argv)
{
    struct lab_conn c;
    struct lab_shm buf[2];
    const uint32_t buf_id[2] = { 11, 12 };
    int released[2] = { 1, 1 };
    int cur = 0;                         /* the buffer attached last */
    int want_tag = -1, want_partial = 0; /* a command waiting for the next frame_done */
    int shown_tag = -1, shown_partial = 0;
    int quitting = 0, stdin_open = 1;
    unsigned long commits = 0, frames = 0, dones = 0, attaches = 0, releases = 0;
    char line[128];
    size_t line_len = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: %s SOCKET\n", argv[0]);
        return 2;
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (lab_connect(&c, argv[1], "A", 5000) < 0 || lab_hello(&c, 1, 2000) < 0)
        return 1;
    for (int i = 0; i < 2; i++) {
        if (lab_shm_create(&buf[i], "lab09-A", 0, W, H, STRIDE_PX * 4,
                           LAB09_FORMAT_ARGB8888, 0, LAB_SEALED) < 0) {
            perror("A: memfd");
            return 1;
        }
        lab_create_buffer(&c, buf_id[i], &buf[i]);
    }
    lab_create_surface(&c, SURFACE);
    lab_set_position(&c, SURFACE, 16, 16, 0);
    draw(&buf[0], 0, 0, 0, W, H);
    lab_attach(&c, SURFACE, buf_id[0]);
    released[0] = 0;
    attaches++;
    lab_damage(&c, SURFACE, 0, 0, W, H);
    lab_frame(&c, SURFACE);
    lab_commit(&c, SURFACE);
    commits++;
    frames++;
    shown_tag = 0;
    lab_log("A", "commit tag=0 kind=full sent_us=%lld", (long long)lab_now_us());

    while (!quitting) {
        struct pollfd pfd[2] = { { c.fd, POLLIN, 0 }, { 0, POLLIN, 0 } };
        int n = poll(pfd, stdin_open ? 2 : 1, -1);
        if (n < 0 && errno != EINTR)
            break;
        if (stdin_open && (pfd[1].revents & (POLLIN | POLLHUP))) {
            ssize_t r = read(0, line + line_len, sizeof line - 1 - line_len);
            if (r <= 0) {
                stdin_open = 0;
            } else {
                line_len += (size_t)r;
                char *nl;
                while ((nl = memchr(line, '\n', line_len)) != NULL) {
                    *nl = '\0';
                    int k;
                    char kind[16];
                    if (strcmp(line, "quit") == 0) {
                        quitting = 1;
                    } else if (sscanf(line, "tag %d %15s", &k, kind) == 2) {
                        want_tag = k;
                        want_partial = strcmp(kind, "partial") == 0;
                    } else {
                        lab_log("A", "bad-command text=%s", line);
                    }
                    size_t used = (size_t)(nl - line) + 1;
                    memmove(line, nl + 1, line_len - used);
                    line_len -= used;
                }
                if (line_len == sizeof line - 1)
                    line_len = 0;
            }
        }
        if (quitting || !(pfd[0].revents & (POLLIN | POLLHUP | POLLERR)))
            continue;

        struct lab_event ev;
        int r;
        while ((r = lab_next_event(&c, &ev, 0)) == 1) {
            int64_t now = lab_now_us();
            if (ev.object == SURFACE && ev.opcode == LAB09_SURFACE_FRAME_DONE && ev.nargs == 3) {
                long long vblank_us = (long long)(((uint64_t)ev.arg[1] << 32) | ev.arg[2]);
                dones++;
                lab_log("A", "done seq=%u vblank_us=%lld recv_us=%lld", ev.arg[0], vblank_us,
                        (long long)now);
                if (shown_tag >= 0) {
                    lab_log("A", "shown tag=%d kind=%s seq=%u", shown_tag,
                            shown_partial ? "partial" : "full", ev.arg[0]);
                    shown_tag = -1;
                }
                /* The next commit: a new picture if one was asked for and
                 * its buffer is free, otherwise just a frame request. */
                if (want_tag >= 0) {
                    int b = want_partial ? cur : 1 - cur;
                    if (released[b]) {
                        if (want_partial)
                            draw(&buf[b], want_tag, PART_X, PART_Y, PART_W, PART_H);
                        else
                            draw(&buf[b], want_tag, 0, 0, W, H);
                        lab_attach(&c, SURFACE, buf_id[b]);
                        released[b] = 0;
                        attaches++;
                        if (want_partial)
                            lab_damage(&c, SURFACE, PART_X, PART_Y, PART_W, PART_H);
                        else
                            lab_damage(&c, SURFACE, 0, 0, W, H);
                        cur = b;
                        shown_tag = want_tag;
                        shown_partial = want_partial;
                        lab_log("A", "commit tag=%d kind=%s sent_us=%lld", want_tag,
                                want_partial ? "partial" : "full", (long long)lab_now_us());
                        want_tag = -1;
                    } else {
                        lab_log("A", "waiting-release buffer=%u", buf_id[b]);
                    }
                }
                lab_frame(&c, SURFACE);
                lab_commit(&c, SURFACE);
                commits++;
                frames++;
            } else if (ev.opcode == LAB09_BUFFER_RELEASE && ev.nargs == 0 &&
                       (ev.object == buf_id[0] || ev.object == buf_id[1])) {
                int b = ev.object == buf_id[1];
                released[b] = 1;
                releases++;
                lab_log("A", "release buffer=%u", ev.object);
            } else {
                lab_log_event(&c, "unexpected", &ev);
                if (ev.object == LAB09_DISPLAY_ID && ev.opcode == LAB09_DISPLAY_ERROR)
                    return 1;
            }
        }
        if (r < 0) {
            lab_log("A", "disconnected dones=%lu", dones);
            return 1;
        }
    }

    /* Give the last release a moment to arrive, then clean up. */
    struct lab_event ev;
    int64_t until = lab_now_us() + 100000;
    while (releases < attaches && lab_now_us() < until) {
        int r = lab_next_event(&c, &ev, 20);
        if (r < 0)
            break;
        if (r == 1 && ev.opcode == LAB09_BUFFER_RELEASE)
            releases++;
    }
    lab_destroy(&c, SURFACE);
    lab_destroy(&c, buf_id[0]);
    lab_destroy(&c, buf_id[1]);
    lab_log("A", "summary commits=%lu frame_requests=%lu dones=%lu attaches=%lu releases=%lu",
            commits, frames, dones, attaches, releases);
    close(c.fd);
    return 0;
}
