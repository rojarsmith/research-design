/*
 * lab09-protocol.h - Lab 09: the numbers of the headless display protocol,
 * version 1.
 *
 * PROTOCOL.md is the specification. This header only gives its numbers C
 * names. The harness clients include it, and your server may include it
 * too (run.sh passes -I to the harness directory), or define its own
 * names for the same numbers. If this file and PROTOCOL.md disagree,
 * PROTOCOL.md wins and this file has a bug.
 *
 * Every message is a sequence of 32-bit words in host byte order:
 *   word 0  object id
 *   word 1  (size << 16) | opcode, where size counts bytes, header included
 *   word 2… arguments, one word each
 * File descriptors travel as SCM_RIGHTS ancillary data, outside the bytes,
 * in the same order as the requests that take them.
 */
#ifndef LAB09_PROTOCOL_H
#define LAB09_PROTOCOL_H

#include <stdint.h>

#define LAB09_VERSION 1

/* The simulated output. */
#define LAB09_WIDTH 320
#define LAB09_HEIGHT 240
#define LAB09_REFRESH_MHZ 60000u
#define LAB09_PERIOD_NS 16666667LL     /* 1/60 s, rounded */
#define LAB09_BACKGROUND 0xff202030u   /* opaque, premultiplied ARGB8888 */

/* Message framing. */
#define LAB09_HEADER_SIZE 8
#define LAB09_MAX_MESSAGE 64           /* no version 1 message is larger */
#define LAB09_WORD1(size, opcode) (((uint32_t)(size) << 16) | ((uint32_t)(opcode) & 0xffffu))
#define LAB09_SIZE(word1) ((uint32_t)(word1) >> 16)
#define LAB09_OPCODE(word1) ((uint32_t)(word1) & 0xffffu)

/* Object ids. The display is object 1 from the start; clients choose the
 * ids of everything they create. */
#define LAB09_DISPLAY_ID 1u
#define LAB09_MIN_CLIENT_ID 2u
#define LAB09_MAX_CLIENT_ID 65535u

/* display: requests */
#define LAB09_DISPLAY_HELLO 0           /* uint version                         12 bytes */
#define LAB09_DISPLAY_SYNC 1            /* uint serial                          12 */
#define LAB09_DISPLAY_CREATE_SURFACE 2  /* new_id surface                       12 */
#define LAB09_DISPLAY_CREATE_BUFFER 3   /* new_id buffer, fd (out of band),
                                           int offset, int width, int height,
                                           int stride, uint format              32 */
/* display: events */
#define LAB09_DISPLAY_WELCOME 0         /* uint version, uint width,
                                           uint height, uint refresh_mhz        24 */
#define LAB09_DISPLAY_DONE 1            /* uint serial                          12 */
#define LAB09_DISPLAY_ERROR 2           /* uint object_id, uint code            16 */

/* surface: requests */
#define LAB09_SURFACE_DESTROY 0         /*                                       8 */
#define LAB09_SURFACE_ATTACH 1          /* uint buffer (0: no content)          12 */
#define LAB09_SURFACE_SET_POSITION 2    /* int x, int y, int z                  20 */
#define LAB09_SURFACE_DAMAGE 3          /* int x, int y, int width, int height  24 */
#define LAB09_SURFACE_FRAME 4           /*                                       8 */
#define LAB09_SURFACE_COMMIT 5          /*                                       8 */
/* surface: events */
#define LAB09_SURFACE_FRAME_DONE 0      /* uint seq, uint time_us_hi,
                                           uint time_us_lo                      20 */

/* buffer: requests */
#define LAB09_BUFFER_DESTROY 0          /*                                       8 */
/* buffer: events */
#define LAB09_BUFFER_RELEASE 0          /*                                       8 */

/* Pixel formats: the same values as wl_shm. Both are 32-bit words in host
 * byte order, A (or x) in bits 31..24, then R, G, B. ARGB8888 is
 * premultiplied. In XRGB8888 the top byte means nothing. */
#define LAB09_FORMAT_ARGB8888 0u
#define LAB09_FORMAT_XRGB8888 1u

/* Limits every version 1 server accepts at least. */
#define LAB09_MAX_BUFFER_SIDE 4096
#define LAB09_MIN_DAMAGE_RECTS 16       /* per commit, kept exactly */

/* display.error codes */
#define LAB09_ERROR_INVALID_OBJECT 1    /* unknown object, or wrong type */
#define LAB09_ERROR_INVALID_METHOD 2    /* opcode unknown for this interface */
#define LAB09_ERROR_INVALID_MESSAGE 3   /* bad size, no hello first, second hello */
#define LAB09_ERROR_INVALID_ID 4        /* new_id out of range or in use */
#define LAB09_ERROR_NO_FD 5             /* a request needs an fd and none came */
#define LAB09_ERROR_BAD_BUFFER 6        /* unsealed, too small, bad geometry */
#define LAB09_ERROR_BAD_FORMAT 7        /* unknown format */
#define LAB09_ERROR_NO_RESOURCES 8      /* a server limit was reached */
#define LAB09_ERROR_VERSION 9           /* hello with version 0 */

#endif
