/*
 * bad-damage.c - Lab 09, bad client 3: damage outside its buffer.
 *
 * A 64x64 surface at (16, 160), z 3, in three steps:
 *   state 0  picture P0, attached with damage over the whole buffer;
 *   state 1  the whole buffer is redrawn with picture P1, but the same
 *            buffer is attached again with only these damage rectangles:
 *              (-1000, -1000, 10, 10)        all outside
 *              (60, 60, INT32_MAX, INT32_MAX) x + width overflows int32;
 *                                            clipped, it is (60, 60) 4x4
 *              (INT32_MIN, 10, INT32_MAX, 2) ends at x = -1: all outside
 *              (10, -5, 4, 10)               clipped to (10, 0) 4x5
 *              (20, 20, -5, -5)              negative size: empty
 *              (64, 0, 10, 10), (0, 64, 1, 1) start on the far edge: empty
 *            PROTOCOL.md says damage is clipped to the buffer and only
 *            damaged pixels are copied, so state 1 shows P0 except two
 *            small P1 corners;
 *   state 2  the same buffer again with full damage: all P1.
 * A server that does not clip reads or writes outside its own memory; a
 * server that clips with 32-bit sums loses the second rectangle.
 *
 * usage: bad-damage SOCKET
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "clientlib.h"

#define SURFACE 50
#define BUFFER 51

/* check.py's pattern_d() must compute the same values. */
static uint32_t pattern_d(int state, int x, int y)
{
    if (state == 0)
        return 0xff000000u | (uint32_t)(0x80 + x) << 8 | ((uint32_t)(y * 4) & 0xff);
    return 0xffff0000u | ((uint32_t)(x * 4) & 0xff) << 8 | 0x20u;
}

/* Wait for the frame_done of the last commit; note a release on the way.
 * Returns the seq, or 0 if the server sent an error or closed. */
static uint32_t wait_done(struct lab_conn *c, int *released)
{
    struct lab_event ev;
    for (;;) {
        int r = lab_next_event(c, &ev, 2000);
        if (r <= 0) {
            lab_log("D", "no-frame-done reason=%s", r == 0 ? "timeout" : "closed");
            return 0;
        }
        if (ev.object == SURFACE && ev.opcode == LAB09_SURFACE_FRAME_DONE)
            return ev.arg[0];
        if (ev.object == BUFFER && ev.opcode == LAB09_BUFFER_RELEASE) {
            *released = 1;
            continue;
        }
        lab_log_event(c, "unexpected", &ev);
        if (ev.object == LAB09_DISPLAY_ID && ev.opcode == LAB09_DISPLAY_ERROR)
            return 0;
    }
}

int main(int argc, char **argv)
{
    struct lab_conn c;
    struct lab_shm b;
    int released = 0;
    uint32_t seq;

    if (argc != 2) {
        fprintf(stderr, "usage: %s SOCKET\n", argv[0]);
        return 2;
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (lab_connect(&c, argv[1], "D", 5000) < 0 || lab_hello(&c, 1, 2000) < 0)
        return 1;
    if (lab_shm_create(&b, "lab09-D", 0, 64, 64, 64 * 4, LAB09_FORMAT_ARGB8888, 0, LAB_SEALED) < 0) {
        perror("D: memfd");
        return 1;
    }
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            lab_shm_row(&b, y)[x] = pattern_d(0, x, y);
    lab_create_buffer(&c, BUFFER, &b);
    lab_create_surface(&c, SURFACE);
    lab_set_position(&c, SURFACE, 16, 160, 3);
    lab_attach(&c, SURFACE, BUFFER);
    lab_damage(&c, SURFACE, 0, 0, 64, 64);
    lab_frame(&c, SURFACE);
    lab_commit(&c, SURFACE);
    if ((seq = wait_done(&c, &released)) == 0)
        return 1;
    lab_log("D", "shown state=0 seq=%u released=%d", seq, released);

    /* State 1: new pixels everywhere, damage that is mostly nonsense. */
    if (!released)
        lab_log("D", "note buffer-not-released-before-redraw");
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            lab_shm_row(&b, y)[x] = pattern_d(1, x, y);
    released = 0;
    lab_attach(&c, SURFACE, BUFFER);
    lab_damage(&c, SURFACE, -1000, -1000, 10, 10);
    lab_damage(&c, SURFACE, 60, 60, INT32_MAX, INT32_MAX);
    lab_damage(&c, SURFACE, INT32_MIN, 10, INT32_MAX, 2);
    lab_damage(&c, SURFACE, 10, -5, 4, 10);
    lab_damage(&c, SURFACE, 20, 20, -5, -5);
    lab_damage(&c, SURFACE, 64, 0, 10, 10);
    lab_damage(&c, SURFACE, 0, 64, 1, 1);
    lab_frame(&c, SURFACE);
    lab_commit(&c, SURFACE);
    if ((seq = wait_done(&c, &released)) == 0)
        return 1;
    lab_log("D", "shown state=1 seq=%u released=%d", seq, released);

    /* State 2: the same buffer with honest damage. */
    released = 0;
    lab_attach(&c, SURFACE, BUFFER);
    lab_damage(&c, SURFACE, 0, 0, 64, 64);
    lab_frame(&c, SURFACE);
    lab_commit(&c, SURFACE);
    if ((seq = wait_done(&c, &released)) == 0)
        return 1;
    lab_log("D", "shown state=2 seq=%u released=%d", seq, released);

    lab_destroy(&c, SURFACE);
    lab_destroy(&c, BUFFER);
    lab_sync(&c, 1);
    struct lab_event ev;
    int r, got = 0;
    while (!got && (r = lab_next_event(&c, &ev, 2000)) == 1)
        got = ev.object == LAB09_DISPLAY_ID && ev.opcode == LAB09_DISPLAY_DONE;
    lab_log("D", "exit sync=%s", got ? "done" : "missing");
    close(c.fd);
    return 0;
}
