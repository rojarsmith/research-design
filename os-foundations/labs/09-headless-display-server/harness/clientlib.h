/*
 * clientlib.h - Lab 09: what the harness clients share. Connecting,
 * sending requests (with an fd when a request takes one), reading events,
 * and making memfd buffers, sealed or not.
 *
 * Every client prints one line per thing that happened, on stdout, line
 * buffered, as "TAG what key=value ...". check.py reads those lines. Times
 * are CLOCK_MONOTONIC in microseconds, the clock the server's frame_done
 * times use too.
 */
#ifndef LAB09_CLIENTLIB_H
#define LAB09_CLIENTLIB_H

#include <stddef.h>
#include <stdint.h>

#include "lab09-protocol.h"

struct lab_conn {
    int fd;
    const char *tag;              /* the first word of every log line */
    uint8_t in[4096];             /* bytes received, not yet parsed */
    size_t in_len;
    uint32_t version, width, height, refresh_mhz;   /* from welcome */
};

struct lab_event {
    uint32_t object, opcode, size, nargs;
    uint32_t arg[(LAB09_MAX_MESSAGE - LAB09_HEADER_SIZE) / 4];
};

/* A buffer in a memfd of its own. */
struct lab_shm {
    int fd;
    uint8_t *map;
    size_t size;                  /* the file size */
    int32_t offset, width, height, stride;
    uint32_t format;
};

/* lab_shm_create() sealing modes */
#define LAB_SEALED 0              /* MFD_ALLOW_SEALING, then F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL */
#define LAB_NO_SEALING 1          /* no MFD_ALLOW_SEALING: nothing can be sealed later */

int64_t lab_now_us(void);
void lab_log(const char *tag, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
const char *lab_error_name(uint32_t code);

/* Connect to the server's socket, retrying until timeout_ms while it does
 * not exist yet or refuses. 0 on success. */
int lab_connect(struct lab_conn *c, const char *path, const char *tag, int timeout_ms);

/* One request. fd < 0 sends no fd. Blocks until every byte is written.
 * 0 on success, -1 with errno set (EPIPE, ECONNRESET: the server closed). */
int lab_send(struct lab_conn *c, uint32_t object, uint32_t opcode,
             const uint32_t *args, int nargs, int fd);
/* Raw bytes, for messages built by hand (several at once, or a piece of
 * one). fd < 0 sends no fd. */
int lab_send_raw(struct lab_conn *c, const void *data, size_t len, int fd);
/* Append one message to buf at *len, for lab_send_raw(). */
void lab_put(uint8_t *buf, size_t *len, uint32_t object, uint32_t opcode,
             const uint32_t *args, int nargs);

/* The next event: 1 with *ev filled, 0 when timeout_ms passed first
 * (timeout_ms < 0 waits forever), -1 when the server closed the
 * connection or sent something that is not a message. */
int lab_next_event(struct lab_conn *c, struct lab_event *ev, int timeout_ms);

/* hello(version), then wait for welcome. 0 on success; logs what it got. */
int lab_hello(struct lab_conn *c, uint32_t version, int timeout_ms);

/* Make a buffer. file_size 0 means exactly offset + stride * height. */
int lab_shm_create(struct lab_shm *b, const char *name, int32_t offset,
                   int32_t width, int32_t height, int32_t stride,
                   uint32_t format, size_t file_size, int sealing);
uint32_t *lab_shm_row(const struct lab_shm *b, int y);

/* The requests, one function each. */
int lab_sync(struct lab_conn *c, uint32_t serial);
int lab_create_surface(struct lab_conn *c, uint32_t id);
int lab_create_buffer(struct lab_conn *c, uint32_t id, const struct lab_shm *b);
int lab_attach(struct lab_conn *c, uint32_t surface, uint32_t buffer);
int lab_set_position(struct lab_conn *c, uint32_t surface, int32_t x, int32_t y, int32_t z);
int lab_damage(struct lab_conn *c, uint32_t surface, int32_t x, int32_t y, int32_t w, int32_t h);
int lab_frame(struct lab_conn *c, uint32_t surface);
int lab_commit(struct lab_conn *c, uint32_t surface);
int lab_destroy(struct lab_conn *c, uint32_t object);   /* surface or buffer: opcode 0 */

/* Log an event the caller did not expect, in one line. */
void lab_log_event(const struct lab_conn *c, const char *what, const struct lab_event *ev);

#endif
