// mmaptest.c - Lab 07, exercise D: file-backed mmap() and munmap().
//
// Needs, from your kernel change, in user/user.h:
//   void *mmap(void *addr, uint64 len, int prot, int flags, int fd,
//              uint64 off);
//   int munmap(void *addr, uint64 len);
// and in kernel/fcntl.h the constants below (this file falls back to the
// same values). addr is always 0 (the kernel chooses) and off is always 0
// here. mmap() returns (void *)-1 on failure.
//
// Lines marked "ok" or "FAILED" are requirements. Lines marked "observed"
// are behavior that the requirements leave open; compare them with Linux
// (Lab 03, exercise D, and linux/pagecache.c in this lab).
//
// Install: copy to user/mmaptest.c and add $U/_mmaptest to UPROGS.
// Run: mmaptest

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#ifndef PROT_READ
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define MAP_SHARED  0x01
#define MAP_PRIVATE 0x02
#endif

#define PG    4096
#define FSIZE (2 * PG + PG / 2) // the file ends in the middle of page 2
#define MAPLEN (3 * PG)
#define FAIL ((char *)-1)

static int failures;

static void
ok(int cond, char *what)
{
  printf("  %s: %s\n", cond ? "ok" : "FAILED", what);
  if (!cond)
    failures++;
}

static void
observed(char *what, char *answer)
{
  printf("  observed: %s: %s\n", what, answer);
}

// The byte at offset i of a freshly made test file.
static char
want(int i)
{
  return (char)(i % 251 + i / PG + 1);
}

static void
makefile(char *name)
{
  static char b[FSIZE];
  for (int i = 0; i < FSIZE; i++)
    b[i] = want(i);
  unlink(name);
  int fd = open(name, O_CREATE | O_RDWR);
  if (fd < 0 || write(fd, b, FSIZE) != FSIZE) {
    printf("mmaptest: cannot make %s\n", name);
    exit(1);
  }
  close(fd);
}

static int
matches(char *p, int from, int to)
{
  for (int i = from; i < to; i++)
    if (p[i] != want(i))
      return 0;
  return 1;
}

static int
zeros(char *p, int n)
{
  for (int i = 0; i < n; i++)
    if (p[i] != 0)
      return 0;
  return 1;
}

// Read n bytes at offset off of the file through read(), into buf.
static void
fileat(char *name, int off, char *buf, int n)
{
  char skip[512];
  int fd = open(name, O_RDONLY);
  while (off > 0) {
    int m = off < sizeof skip ? off : sizeof skip;
    read(fd, skip, m);
    off -= m;
  }
  memset(buf, 0, n);
  read(fd, buf, n);
  close(fd);
}

static int
filesize(char *name)
{
  struct stat st;
  if (stat(name, &st) < 0)
    return -1;
  return st.size;
}

// Run f in a child; return the child's exit status (-1 when the kernel
// killed it).
static int
inchild(void (*f)(char *), char *p)
{
  int st;
  if (fork() == 0) {
    f(p);
    exit(0);
  }
  wait(&st);
  return st;
}

static void
touch(char *p)
{
  volatile char c = *p;
  (void)c;
}

static void
poke(char *p)
{
  *(volatile char *)p = 'X';
}

static void
part1(void)
{
  printf("1. a private, read-only mapping of a %d-byte file\n", FSIZE);
  makefile("mm.dat");
  int fd = open("mm.dat", O_RDONLY);
  char *brk = sbrk(0);
  char *p = mmap(0, MAPLEN, PROT_READ, MAP_PRIVATE, fd, 0);
  ok(p != FAIL, "mmap(0, 3 pages, PROT_READ, MAP_PRIVATE, fd, 0)");
  if (p == FAIL)
    exit(1);
  printf("  mapped at %p; sbrk(0) is %p\n", p, brk);
  ok((uint64)p % PG == 0, "the address is page-aligned");
  ok(p >= brk && sbrk(0) == brk, "above the heap, and the heap did not move");
  close(fd);
  ok(matches(p, 0, FSIZE), "every byte matches the file, after close(fd)");
  ok(zeros(p + FSIZE, MAPLEN - FSIZE), "the rest of the last page reads 0");
  ok(munmap(p, MAPLEN) == 0, "munmap()");
  ok(inchild(touch, p) == -1, "a child that reads the unmapped page is killed");
}

static void
part2(void)
{
  char buf[8];
  printf("2. pages are read from the file on first touch\n");
  makefile("mm.dat");
  int fd = open("mm.dat", O_RDONLY);
  char *p = mmap(0, 2 * PG, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  touch(p); // page 0 now, page 1 not yet
  // Overwrite the first 5000 bytes of the file through write().
  static char n[5000];
  memset(n, 'N', sizeof n);
  fd = open("mm.dat", O_RDWR);
  write(fd, n, sizeof n);
  close(fd);
  fileat("mm.dat", 0, buf, 1);
  printf("  write() put 'N' in bytes 0-4999; read() now sees '%c'\n", buf[0]);
  ok(p[PG] == 'N',
     "page 1, first touched after the write(), shows 'N': read lazily");
  observed("page 0, touched before the write()",
           p[0] == 'N' ? "shows 'N' (the mapping saw the write)"
                       : "still shows the old byte (its own copy)");
  munmap(p, 2 * PG);
}

static void
part3(void)
{
  char buf[16];
  printf("3. MAP_SHARED: writes reach the file, but not past its end\n");
  makefile("mm.dat");
  int fd = open("mm.dat", O_RDWR);
  char *p = mmap(0, MAPLEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  ok(p != FAIL, "mmap(PROT_READ | PROT_WRITE, MAP_SHARED) of an O_RDWR fd");
  close(fd);
  memmove(p, "shared!", 7);
  memmove(p + FSIZE + 10, "past EOF", 8); // in the last page, past the end
  fileat("mm.dat", 0, buf, 7);
  observed("read() of the file before munmap()",
           memcmp(buf, "shared!", 7) == 0
             ? "sees the mapped write already"
             : "sees the old bytes (written back at munmap)");
  ok(munmap(p, MAPLEN) == 0, "munmap()");
  fileat("mm.dat", 0, buf, 7);
  ok(memcmp(buf, "shared!", 7) == 0, "after munmap(), read() sees the write");
  ok(filesize("mm.dat") == FSIZE, "the file did not grow");
}

static void
part4(void)
{
  char buf[8];
  printf("4. MAP_PRIVATE: writes stay in the process\n");
  makefile("mm.dat");
  int fd = open("mm.dat", O_RDONLY);
  char *p = mmap(0, MAPLEN, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  ok(p != FAIL, "mmap(PROT_READ | PROT_WRITE, MAP_PRIVATE) of an O_RDONLY fd");
  close(fd);
  memmove(p, "private", 7);
  ok(memcmp(p, "private", 7) == 0, "the process sees its own write");
  munmap(p, MAPLEN);
  fileat("mm.dat", 0, buf, 7);
  ok(buf[0] == want(0) && buf[6] == want(6), "the file is unchanged");
}

static void
part5(void)
{
  printf("5. permissions\n");
  makefile("mm.dat");
  int fd = open("mm.dat", O_RDONLY);
  ok(mmap(0, PG, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0) == FAIL,
     "PROT_WRITE + MAP_SHARED of an O_RDONLY fd is refused");
  close(fd);
  fd = open("mm.dat", O_WRONLY);
  ok(mmap(0, PG, PROT_READ, MAP_SHARED, fd, 0) == FAIL,
     "PROT_READ of an O_WRONLY fd is refused");
  close(fd);
  fd = open("mm.dat", O_RDWR);
  char *p = mmap(0, PG, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  ok(inchild(poke, p) == -1, "a child that writes a PROT_READ page is killed");
  munmap(p, PG);
}

static void
part6(void)
{
  char buf[8];
  printf("6. munmap() of the first page, then of the rest\n");
  makefile("mm.dat");
  int fd = open("mm.dat", O_RDWR);
  char *p = mmap(0, MAPLEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  p[0] = 'F';
  p[2 * PG] = 'L';
  ok(munmap(p, PG) == 0, "munmap() of page 0");
  fileat("mm.dat", 0, buf, 1);
  ok(buf[0] == 'F', "page 0 was written back");
  fileat("mm.dat", 2 * PG, buf, 1);
  observed("page 2, still mapped", buf[0] == 'L' ? "already in the file"
                                                  : "not in the file yet");
  ok(p[2 * PG] == 'L' && p[PG] == want(PG), "pages 1 and 2 still work");
  ok(munmap(p + PG, 2 * PG) == 0, "munmap() of pages 1 and 2");
  fileat("mm.dat", 2 * PG, buf, 1);
  ok(buf[0] == 'L', "page 2 was written back");
}

static void
part7(void)
{
  char buf[8];
  int st;
  printf("7. fork(), and exit() without munmap()\n");
  makefile("mm.dat");
  int fd = open("mm.dat", O_RDWR);
  char *s = mmap(0, MAPLEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  makefile("mm2.dat");
  fd = open("mm2.dat", O_RDONLY);
  char *v = mmap(0, PG, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  close(fd);
  ok(s != FAIL && v != FAIL && s != v, "two mappings at two addresses");
  touch(s);                // the parent has page 0 of s before the fork
  memmove(v, "parent", 6); // and a private change in v
  if (fork() == 0) {
    int good = matches(s, PG, FSIZE);
    s[0] = 'C'; // the child's write; it exits without munmap()
    exit(good ? (memcmp(v, "parent", 6) == 0 ? 2 : 3) : 1);
  }
  wait(&st);
  ok(st >= 2, "the child read the inherited shared mapping");
  observed("the child's view of the parent's private write",
           st == 2 ? "inherited" : "not inherited (read again from the file)");
  fileat("mm.dat", 0, buf, 1);
  ok(buf[0] == 'C', "the child's exit() wrote its change back");
  observed("the parent's own mapping of that byte",
           s[0] == 'C' ? "shows the child's write" : "does not show it");
  munmap(s, MAPLEN);
  munmap(v, PG);
}

static void
part8(void)
{
  char buf[8];
  int st, ping[2], pong[2];
  printf("8. two processes map the same file MAP_SHARED\n");
  makefile("mm.dat");
  pipe(ping);
  pipe(pong);
  if (fork() == 0) {
    int fd = open("mm.dat", O_RDWR);
    char *q = mmap(0, PG, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    touch(q);
    write(pong[1], "m", 1); // mapped and touched
    read(ping[0], buf, 1);  // the parent has written
    exit(q[0] == 'P' ? 2 : 1);
  }
  int fd = open("mm.dat", O_RDWR);
  char *p = mmap(0, PG, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  read(pong[0], buf, 1);
  p[0] = 'P';
  write(ping[1], "w", 1);
  wait(&st);
  observed("B's mapping after A wrote to its own", st == 2
                                                      ? "sees A's write"
                                                      : "does not see it");
  munmap(p, PG);
  close(ping[0]);
  close(ping[1]);
  close(pong[0]);
  close(pong[1]);
}

static void
part9(void)
{
  char buf[16];
  printf("9. the kernel reads a mapped page that was never touched\n");
  makefile("mm.dat");
  int fd = open("mm.dat", O_RDONLY);
  char *p = mmap(0, MAPLEN, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  unlink("mm3.dat");
  fd = open("mm3.dat", O_CREATE | O_RDWR);
  ok(write(fd, p + PG, 16) == 16, "write(fd, untouched mapped page, 16)");
  close(fd);
  fileat("mm3.dat", 0, buf, 16);
  int same = 1;
  for (int i = 0; i < 16; i++)
    same = same && buf[i] == want(PG + i);
  ok(same, "the right 16 bytes arrived");
  munmap(p, MAPLEN);
  unlink("mm3.dat");
}

static void
part10(void)
{
  printf("10. 100 rounds of mmap() and munmap()\n");
  makefile("mm.dat");
  int fd = open("mm.dat", O_RDWR), good = 1;
  char *first = 0;
  for (int i = 0; i < 100 && good; i++) {
    char *p = mmap(0, MAPLEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == FAIL) {
      good = 0;
      break;
    }
    if (i == 0)
      first = p;
    p[PG] = want(PG);
    if (munmap(p, MAPLEN) < 0)
      good = 0;
  }
  close(fd);
  ok(good, "100 mappings, more than NVMA slots: munmap() frees the slot");
  char *again = 0;
  fd = open("mm.dat", O_RDONLY);
  again = mmap(0, PG, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  printf("  first mapping at %p, a new one now at %p\n", first, again);
  munmap(again, PG);
}

int
main(void)
{
  part1();
  part2();
  part3();
  part4();
  part5();
  part6();
  part7();
  part8();
  part9();
  part10();
  unlink("mm.dat");
  unlink("mm2.dat");
  printf("mmaptest: %d failure(s)\n", failures);
  exit(failures);
}
