/*
 * bad-unsealed.c - Lab 09, bad client 4: a buffer that can shrink under
 * the server.
 *
 * Two connections, one after the other, each trying to show a 48x40
 * surface at (250, 100), z 3:
 *   unsealed   a memfd made without MFD_ALLOW_SEALING, so it can never be
 *              sealed. After create_buffer the client sends a sync and
 *              waits for its done: by then the server has handled the
 *              buffer, mapped it if it accepted it. Then the client
 *              truncates the memfd to 0 bytes, attaches the buffer and
 *              commits. A server that accepted it now copies from pages
 *              that no longer exist and gets SIGBUS.
 *   too-small  a memfd sealed against shrinking and growing, but only half
 *              as large as offset + stride * height. Mapping it works;
 *              reading the second half gets SIGBUS.
 * PROTOCOL.md says the server rejects both at create_buffer with
 * display.error bad_buffer and closes the connection. The client prints
 * one "verdict" line per answer: result=error code=6 is correct;
 * result=accepted means the server took the buffer (and, if the server is
 * still alive, only luck saved it).
 *
 * usage: bad-unsealed SOCKET
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "clientlib.h"

#define SURFACE 61
#define BUFFER 60
#define W 48
#define H 40

/* Wait for an event that answers the last request; report it. Returns 1
 * if the buffer was accepted (the sync came back), 0 otherwise. */
static int verdict(struct lab_conn *c, const char *variant, const char *when)
{
    struct lab_event ev;
    for (;;) {
        int r = lab_next_event(c, &ev, 1000);
        if (r == 0) {
            lab_log("U", "verdict variant=%s when=%s result=no-reply", variant, when);
            return 0;
        }
        if (r < 0) {
            lab_log("U", "verdict variant=%s when=%s result=closed-without-error", variant, when);
            return 0;
        }
        if (ev.object == LAB09_DISPLAY_ID && ev.opcode == LAB09_DISPLAY_ERROR && ev.nargs == 2) {
            lab_log("U", "verdict variant=%s when=%s result=error code=%u name=%s object=%u", variant, when,
                    ev.arg[1], lab_error_name(ev.arg[1]), ev.arg[0]);
            /* the server closes the connection after an error */
            r = lab_next_event(c, &ev, 1000);
            lab_log("U", "after-error variant=%s state=%s", variant,
                    r < 0 ? "closed" : r == 0 ? "still-open" : "more-events");
            return 0;
        }
        if (ev.object == LAB09_DISPLAY_ID && ev.opcode == LAB09_DISPLAY_DONE) {
            lab_log("U", "verdict variant=%s when=%s result=accepted", variant, when);
            return 1;
        }
        if (ev.object == SURFACE && ev.opcode == LAB09_SURFACE_FRAME_DONE) {
            lab_log("U", "verdict variant=%s when=%s result=shown seq=%u", variant, when, ev.arg[0]);
            return 1;
        }
        /* a release, or anything else: keep waiting */
    }
}

static void attempt(const char *path, const char *variant, int sealing, size_t file_size)
{
    struct lab_conn c;
    struct lab_shm b;
    if (lab_connect(&c, path, "U", 5000) < 0 || lab_hello(&c, 1, 2000) < 0)
        return;
    if (lab_shm_create(&b, "lab09-U", 0, W, H, W * 4, LAB09_FORMAT_ARGB8888, file_size, sealing) < 0) {
        lab_log("U", "memfd-failed variant=%s errno=%d", variant, errno);
        return;
    }
    size_t rows = b.size / (W * 4);     /* touch only what the file holds */
    for (size_t y = 0; y < rows && y < H; y++)
        for (int x = 0; x < W; x++)
            lab_shm_row(&b, (int)y)[x] = 0xffff8000u;
    lab_log("U", "attempt variant=%s file_bytes=%zu needs=%d", variant, b.size, W * 4 * H);

    lab_create_buffer(&c, BUFFER, &b);
    lab_sync(&c, 1);
    if (verdict(&c, variant, "create_buffer")) {
        if (sealing != LAB_SEALED && ftruncate(b.fd, 0) == 0)
            lab_log("U", "shrunk variant=%s to_bytes=0", variant);
        lab_create_surface(&c, SURFACE);
        lab_set_position(&c, SURFACE, 250, 100, 3);
        lab_attach(&c, SURFACE, BUFFER);
        lab_damage(&c, SURFACE, 0, 0, W, H);
        lab_frame(&c, SURFACE);
        lab_commit(&c, SURFACE);
        verdict(&c, variant, "commit");
    }
    close(c.fd);
    munmap(b.map, (size_t)W * 4 * H);
    close(b.fd);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s SOCKET\n", argv[0]);
        return 2;
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    attempt(argv[1], "unsealed", LAB_NO_SEALING, 0);
    attempt(argv[1], "too-small", LAB_SEALED, (size_t)W * 4 * H / 2);
    lab_log("U", "exit");
    return 0;
}
