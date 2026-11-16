// pagecache.c - Lab 07, exercise H: who decides when dirty mapped pages
// reach the disk on Linux?
//
// A MAP_SHARED mapping of a file maps the file's page-cache pages, the
// same pages read(2) and write(2) use. A store makes a page dirty. This
// program counts the dirty pages of its own mapping in /proc/self/smaps
// (Shared_Dirty + Private_Dirty of that one mapping, which counts a page
// whose PTE or page is dirty) and watches them become clean:
//   1. coherence: a store through the mapping and pread() through an fd
//   2. msync(MS_ASYNC), then msync(MS_SYNC)
//   3. munmap() and close() without msync(), then fsync()
//   4. nothing at all: wait for the kernel's flusher threads
// /proc/meminfo's Dirty and Writeback are printed as well; they count the
// whole (virtual) machine, so other work shows up in them.
//
// usage: ./pagecache [directory] [pages] [seconds to wait in part 4]
//        defaults: the current directory, 4096 pages (16 MiB), 60 s
// Run it on a disk file system (ext4 in WSL 2), not tmpfs: tmpfs pages
// have no disk to be written to.

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>

static long pg;

static void
die(const char *what)
{
  perror(what);
  exit(1);
}

static double
now(void)
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

// Dirty kB of the mapping that starts at addr, from /proc/self/smaps.
static long
mapdirty(void *addr)
{
  char want[32], line[256];
  long kb = 0, v;
  int in = 0;
  FILE *f = fopen("/proc/self/smaps", "r");
  if (!f)
    die("/proc/self/smaps");
  snprintf(want, sizeof want, "%lx-", (unsigned long)addr);
  while (fgets(line, sizeof line, f)) {
    if (strchr(line, '-') && strchr(line, ' ') &&
        strchr(line, '-') < strchr(line, ' ')) // a mapping's first line
      in = strncmp(line, want, strlen(want)) == 0;
    else if (in && (sscanf(line, "Shared_Dirty: %ld", &v) == 1 ||
                    sscanf(line, "Private_Dirty: %ld", &v) == 1))
      kb += v;
  }
  fclose(f);
  return kb;
}

static long
meminfo(const char *key)
{
  char line[256];
  long v = -1;
  FILE *f = fopen("/proc/meminfo", "r");
  while (f && fgets(line, sizeof line, f))
    if (!strncmp(line, key, strlen(key)) && line[strlen(key)] == ':')
      sscanf(line + strlen(key) + 1, "%ld", &v);
  if (f)
    fclose(f);
  return v;
}

static void
state(const char *what, void *map)
{
  printf("  %-44s mapping dirty %6ld kB | system Dirty %7ld kB, Writeback "
         "%5ld kB\n",
         what, map ? mapdirty(map) : -1L, meminfo("Dirty"),
         meminfo("Writeback"));
}

static long
readlong(const char *p)
{
  long v = -1;
  FILE *f = fopen(p, "r");
  if (f) {
    if (fscanf(f, "%ld", &v) != 1)
      v = -1;
    fclose(f);
  }
  return v;
}

static void
dirty_all(char *m, long len, int round)
{
  for (long i = 0; i < len; i += pg)
    m[i] = (char)('a' + round);
}

int
main(int argc, char *argv[])
{
  const char *dir = argc > 1 ? argv[1] : ".";
  long npages = argc > 2 ? atol(argv[2]) : 4096;
  int waitsecs = argc > 3 ? atoi(argv[3]) : 60;
  char path[4096];
  struct statfs sfs;
  double t;

  pg = sysconf(_SC_PAGESIZE);
  long len = npages * pg;
  snprintf(path, sizeof path, "%s/pagecache.dat", dir);
  if (statfs(dir, &sfs) < 0)
    die(dir);
  const char *fsname = sfs.f_type == 0xEF53       ? "ext2/3/4"
                       : sfs.f_type == 0x794c7630 ? "overlayfs"
                       : sfs.f_type == 0x01021994 ? "tmpfs"
                       : sfs.f_type == 0x01021997 ? "v9fs"
                                                  : "other";
  printf("file %s on a %s file system (f_type 0x%lx), %ld pages of %ld bytes\n",
         path, fsname, (long)sfs.f_type, npages, pg);
  printf("vm.dirty_expire_centisecs %ld, vm.dirty_writeback_centisecs %ld, "
         "vm.dirty_background_ratio %ld, vm.dirty_ratio %ld\n",
         readlong("/proc/sys/vm/dirty_expire_centisecs"),
         readlong("/proc/sys/vm/dirty_writeback_centisecs"),
         readlong("/proc/sys/vm/dirty_background_ratio"),
         readlong("/proc/sys/vm/dirty_ratio"));

  int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd < 0)
    die(path);
  if (ftruncate(fd, len) < 0)
    die("ftruncate");
  if (fsync(fd) < 0)
    die("fsync");
  char *m = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (m == MAP_FAILED)
    die("mmap");

  printf("1. one page cache: a store through the mapping and pread()\n");
  int fd2 = open(path, O_RDWR);
  char c = 0;
  m[5] = 'M';
  if (pread(fd2, &c, 1, 5) != 1)
    die("pread");
  printf("  store 'M' at byte 5 of the mapping; pread() of byte 5 on another "
         "fd gives '%c'\n",
         c);
  if (pwrite(fd2, "W", 1, 6) != 1)
    die("pwrite");
  printf("  pwrite() 'W' at byte 6 on that fd; the mapping shows '%c'\n",
         m[6]);
  close(fd2);

  printf("2. dirty every page through the mapping, then msync()\n");
  if (msync(m, len, MS_SYNC) < 0)
    die("msync");
  state("clean, after a first msync(MS_SYNC):", m);
  t = now();
  dirty_all(m, len, 1);
  printf("  stores to %ld pages took %.1f ms\n", npages, (now() - t) * 1e3);
  state("after the stores:", m);
  t = now();
  if (msync(m, len, MS_ASYNC) < 0)
    die("msync");
  printf("  msync(MS_ASYNC) returned after %.3f ms\n", (now() - t) * 1e3);
  state("after msync(MS_ASYNC):", m);
  t = now();
  if (msync(m, len, MS_SYNC) < 0)
    die("msync");
  printf("  msync(MS_SYNC) returned after %.1f ms\n", (now() - t) * 1e3);
  state("after msync(MS_SYNC):", m);

  printf("3. dirty every page, munmap() and close() without msync(), map "
         "again\n");
  dirty_all(m, len, 2);
  state("after the stores:", m);
  if (munmap(m, len) < 0)
    die("munmap");
  close(fd);
  state("after munmap() and close():", NULL);
  fd = open(path, O_RDWR);
  m = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (m == MAP_FAILED)
    die("mmap");
  volatile char sum = 0;
  for (long i = 0; i < len; i += pg)
    sum += m[i]; // only reads: the new PTEs are clean
  printf("  mapped again and read every page (byte 0 is '%c')\n", m[0]);
  state("the new mapping, read only:", m);
  t = now();
  if (fsync(fd) < 0)
    die("fsync");
  printf("  fsync(fd) returned after %.1f ms\n", (now() - t) * 1e3);
  state("after fsync(fd):", m);

  printf("4. dirty every page and call nothing: who writes them back?\n");
  dirty_all(m, len, 3);
  double t0 = now();
  long first = mapdirty(m), last = first;
  state("after the stores:", m);
  for (int s = 1; s <= waitsecs; s++) {
    sleep(1);
    long d = mapdirty(m);
    if (d != last || s % 10 == 0) {
      char what[64];
      snprintf(what, sizeof what, "%2d s later:", s);
      state(what, m);
    }
    last = d;
    if (d == 0) {
      printf("  the flusher wrote them back %.1f s after the stores\n",
             now() - t0);
      break;
    }
  }
  if (last != 0)
    printf("  still %ld kB dirty after %d s\n", last, waitsecs);

  munmap(m, len);
  close(fd);
  unlink(path);
  return 0;
}
