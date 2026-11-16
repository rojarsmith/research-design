/*
 * bad-crash.c - Lab 09, bad client 1: crashes in the middle of a frame.
 *
 * It shows a 96x72 striped surface at (200, 150), z 3, and waits for the
 * frame_done that proves it is on screen. Then it starts the next frame
 * and dies half-way through:
 *   1. a second buffer (id 32) is created, so the server holds a mapping
 *      that nothing will ever attach;
 *   2. attach, damage and frame for that buffer go out together with a
 *      third memfd, which no request will ever consume, so it sits in the
 *      server's queue of received fds;
 *   3. then the first 6 of the 8 bytes of the commit header;
 *   4. it waits 500 ms, so a server that blocks reading the rest of a
 *      message stalls visibly, and then kills itself with SIGKILL.
 * Its socket now has a partial message, an unused fd and an uncommitted
 * attach. The server must notice the hang-up, free the surface, both
 * buffers and the fd, stop drawing the surface, and never write to the
 * dead socket in a way that kills the server (SIGPIPE).
 *
 * usage: bad-crash SOCKET
 */
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "clientlib.h"

#define SURFACE 30

/* check.py's pattern_c() must compute the same values. */
static uint32_t pattern_c(int x, int y)
{
    return ((x + y) / 8) & 1 ? 0xffff00ffu : 0xff800080u;
}

int main(int argc, char **argv)
{
    struct lab_conn c;
    struct lab_shm b, b2, spare;
    struct lab_event ev;

    if (argc != 2) {
        fprintf(stderr, "usage: %s SOCKET\n", argv[0]);
        return 2;
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (lab_connect(&c, argv[1], "C", 5000) < 0 || lab_hello(&c, 1, 2000) < 0)
        return 1;
    if (lab_shm_create(&b, "lab09-C", 0, 96, 72, 96 * 4, LAB09_FORMAT_ARGB8888, 0, LAB_SEALED) < 0 ||
        lab_shm_create(&b2, "lab09-C2", 0, 96, 72, 96 * 4, LAB09_FORMAT_ARGB8888, 0, LAB_SEALED) < 0 ||
        lab_shm_create(&spare, "lab09-C3", 0, 16, 16, 64, LAB09_FORMAT_ARGB8888, 0, LAB_SEALED) < 0) {
        perror("C: memfd");
        return 1;
    }
    for (int y = 0; y < 72; y++)
        for (int x = 0; x < 96; x++) {
            lab_shm_row(&b, y)[x] = pattern_c(x, y);
            lab_shm_row(&b2, y)[x] = 0xffffff00u;   /* yellow: must never be shown */
        }

    lab_create_buffer(&c, 31, &b);
    lab_create_surface(&c, SURFACE);
    lab_set_position(&c, SURFACE, 200, 150, 3);
    lab_attach(&c, SURFACE, 31);
    lab_damage(&c, SURFACE, 0, 0, 96, 72);
    lab_frame(&c, SURFACE);
    lab_commit(&c, SURFACE);
    for (;;) {
        int r = lab_next_event(&c, &ev, 2000);
        if (r <= 0) {
            lab_log("C", "no-frame-done reason=%s", r == 0 ? "timeout" : "closed");
            return 1;
        }
        if (ev.object == SURFACE && ev.opcode == LAB09_SURFACE_FRAME_DONE) {
            lab_log("C", "visible seq=%u", ev.arg[0]);
            break;
        }
        if (ev.opcode != LAB09_BUFFER_RELEASE)
            lab_log_event(&c, "unexpected", &ev);
    }

    /* The next frame, cut short. */
    uint8_t msg[3 * LAB09_MAX_MESSAGE];
    size_t len = 0;
    uint32_t a1[1] = { 32 };
    uint32_t a4[4] = { 0, 0, 96, 72 };
    lab_create_buffer(&c, 32, &b2);
    lab_put(msg, &len, SURFACE, LAB09_SURFACE_ATTACH, a1, 1);
    lab_put(msg, &len, SURFACE, LAB09_SURFACE_DAMAGE, a4, 4);
    lab_put(msg, &len, SURFACE, LAB09_SURFACE_FRAME, NULL, 0);
    lab_send_raw(&c, msg, len, spare.fd);
    len = 0;
    lab_put(msg, &len, SURFACE, LAB09_SURFACE_COMMIT, NULL, 0);
    lab_send_raw(&c, msg, 6, -1);
    lab_log("C", "partial-commit-sent bytes=6 unused_fds=1 then SIGKILL in 500 ms");
    usleep(500000);
    kill(getpid(), SIGKILL);
    return 0;
}
