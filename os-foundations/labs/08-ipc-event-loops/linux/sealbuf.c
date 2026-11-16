/*
 * sealbuf.c - Stage 08: sealed memfd buffers. See sealbuf.h.
 */
#define _GNU_SOURCE /* memfd_create, F_ADD_SEALS, F_GET_SEALS */
#include "sealbuf.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int sealbuf_create(const char *name, size_t size)
{
    int fd = memfd_create(name, MFD_ALLOW_SEALING | MFD_CLOEXEC);
    if (fd < 0)
        return -1;
    if (ftruncate(fd, (off_t)size) < 0) {
        int e = errno;
        close(fd);
        errno = e;
        return -1;
    }
    return fd;
}

int sealbuf_seal(int fd, int seals)
{
    return fcntl(fd, F_ADD_SEALS, seals) < 0 ? -1 : 0;
}

const char *sealbuf_seal_names(int seals, char *buf, size_t len)
{
    static const struct {
        int bit;
        const char *name;
    } names[] = {
        {F_SEAL_SEAL, "SEAL"},   {F_SEAL_SHRINK, "SHRINK"},
        {F_SEAL_GROW, "GROW"},   {F_SEAL_WRITE, "WRITE"},
#ifdef F_SEAL_FUTURE_WRITE
        {F_SEAL_FUTURE_WRITE, "FUTURE_WRITE"},
#endif
#ifdef F_SEAL_EXEC
        {F_SEAL_EXEC, "EXEC"},
#endif
    };
    size_t used = 0;
    buf[0] = '\0';
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        if (!(seals & names[i].bit))
            continue;
        int n = snprintf(buf + used, len - used, "%s%s", used ? "|" : "", names[i].name);
        if (n < 0 || (size_t)n >= len - used)
            break;
        used += (size_t)n;
    }
    if (used == 0)
        snprintf(buf, len, "none");
    return buf;
}

int sealbuf_check(int fd, size_t min_size, int required, size_t *size_out, char *why,
                  size_t whylen)
{
    char have[64], need[64];
    struct stat st;

    required |= F_SEAL_SHRINK; /* the one seal this whole check is about */
    if (fstat(fd, &st) < 0) {
        snprintf(why, whylen, "fstat: %s", strerror(errno));
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        snprintf(why, whylen, "not a regular file");
        return -1;
    }
    int seals = fcntl(fd, F_GET_SEALS);
    if (seals < 0) {
        snprintf(why, whylen, "cannot be sealed (F_GET_SEALS: %s)", strerror(errno));
        return -1;
    }
    if ((seals & required) != required) {
        snprintf(why, whylen, "could still shrink or change: seals %s, need %s",
                 sealbuf_seal_names(seals, have, sizeof have),
                 sealbuf_seal_names(required, need, sizeof need));
        return -1;
    }
    if ((size_t)st.st_size < min_size) {
        snprintf(why, whylen, "claims %zu bytes but has %lld", min_size, (long long)st.st_size);
        return -1;
    }
    if (size_out)
        *size_out = (size_t)st.st_size;
    snprintf(why, whylen, "seals %s, %lld bytes", sealbuf_seal_names(seals, have, sizeof have),
             (long long)st.st_size);
    return 0;
}

const void *sealbuf_map_ro(int fd, size_t size)
{
    void *p = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    return p == MAP_FAILED ? NULL : p;
}
