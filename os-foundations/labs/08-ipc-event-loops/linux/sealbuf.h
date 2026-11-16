/*
 * sealbuf.h - Stage 08: shared buffers that the receiver can trust not to
 * shrink.
 *
 * A self-contained helper meant to be copied into later work (the stage 09
 * display server takes client buffers through sealbuf_check()). Build it
 * together with your program:
 *
 *     gcc -O2 -Wall -o prog prog.c sealbuf.c
 *
 * The sender:   fd = sealbuf_create("name", size);  write or map it;
 *               sealbuf_seal(fd, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL);
 *               send fd with fdpass_send().
 * The receiver: if (sealbuf_check(fd, size_it_will_read, F_SEAL_SHRINK,
 *                                 &size, why, sizeof why) < 0) reject;
 *               p = sealbuf_map_ro(fd, size);
 *
 * Why this is enough, in memfd_create(2) and fcntl(2) terms: seals belong
 * to the file (the inode), not to one descriptor, and can only be added,
 * never removed. Once F_SEAL_SHRINK is set, no descriptor anywhere, not
 * even a reopened /proc/PID/fd/N, can make the file smaller, so no page the
 * receiver mapped can vanish and raise SIGBUS. Checking only the size, by
 * contrast, is a time-of-check-to-time-of-use race: the sender can shrink
 * the file after the check.
 *
 * What F_SEAL_SHRINK does not give you: the contents can still change at
 * any time, from the sender's mapping, write(2) or a punched hole (which
 * reads back as zeros). Only F_SEAL_WRITE freezes them, and it can only be
 * added while no writable shared mapping exists. A receiver that must not
 * see the bytes change has to require F_SEAL_WRITE or copy them first.
 */
#ifndef LAB08_SEALBUF_H
#define LAB08_SEALBUF_H

#include <stddef.h>

/* Create an empty memfd with MFD_ALLOW_SEALING | MFD_CLOEXEC and size it to
 * size bytes. Returns the fd or -1 with errno. */
int sealbuf_create(const char *name, size_t size);

/* Add seals (F_SEAL_* from <fcntl.h>). Returns 0 or -1 with errno: EPERM if
 * F_SEAL_SEAL is already set, EBUSY for F_SEAL_WRITE while a writable
 * shared mapping exists, EINVAL if the file cannot be sealed at all. */
int sealbuf_seal(int fd, int seals);

/* Write the names of the seals in `seals` into buf, such as
 * "SEAL|SHRINK|GROW", or "none". Returns buf. */
const char *sealbuf_seal_names(int seals, char *buf, size_t len);

/* The receiver's check. Accept fd only if
 *   - it refers to a regular file whose seals can be read (F_GET_SEALS),
 *   - its seals include every seal in `required`, and `required` itself
 *     includes F_SEAL_SHRINK (the function adds it if you forgot),
 *   - it is at least min_size bytes long.
 * Returns 0 and the real size in *size_out (may be NULL), or -1 with a short
 * reason in why. Never changes the file. */
int sealbuf_check(int fd, size_t min_size, int required, size_t *size_out, char *why,
                  size_t whylen);

/* Map size bytes of fd read-only and shared. Returns the address or NULL. */
const void *sealbuf_map_ro(int fd, size_t size);

#endif
