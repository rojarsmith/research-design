// ofdtest.c - Lab 07, exercise A: file descriptors and the open file
// descriptions behind them, on the stock xv6 kernel.
//
// In xv6, p->ofile[fd] points to a struct file (kernel/file.h). That
// struct is the open file description: it holds the offset, the
// readable and writable flags and the inode. dup() and fork() copy the
// pointer and bump f->ref; a second open() allocates a second struct file
// for the same inode. Each part writes through two descriptors and then
// prints the file, so you can tell from the bytes alone whether the two
// shared an offset.
//
// Install: copy to user/ofdtest.c and add $U/_ofdtest to UPROGS.
// Run: ofdtest

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static void
show(char *what)
{
  char buf[64];
  int fd = open("ofd.txt", O_RDONLY);
  int n = read(fd, buf, sizeof(buf) - 1);
  close(fd);
  buf[n < 0 ? 0 : n] = 0;
  printf("  %s\n    the file is now \"%s\"\n", what, buf);
}

static uint
ino(int fd)
{
  struct stat st;
  fstat(fd, &st);
  return st.ino;
}

int
main(void)
{
  int a, b, c, d, pid;

  printf("1. dup(): two descriptors, one struct file\n");
  a = open("ofd.txt", O_CREATE | O_RDWR | O_TRUNC);
  b = dup(a);
  write(a, "AAAA", 4);
  write(b, "BBBB", 4);
  printf("  fd %d and fd %d, inode %d and %d\n", a, b, ino(a), ino(b));
  show("write AAAA via fd a, then BBBB via fd b:");
  close(a);
  close(b);

  printf("2. open() twice: two struct files, one inode\n");
  c = open("ofd.txt", O_RDWR);
  d = open("ofd.txt", O_RDWR);
  write(c, "cccc", 4);
  write(d, "dd", 2);
  printf("  fd %d and fd %d, inode %d and %d\n", c, d, ino(c), ino(d));
  show("write cccc via fd c, then dd via fd d:");
  close(c);
  close(d);

  printf("3. fork(): the child's descriptor is the parent's struct file\n");
  a = open("ofd.txt", O_RDWR | O_TRUNC);
  pid = fork();
  if (pid == 0) {
    write(a, "child.", 6);
    exit(0);
  }
  wait(0);
  write(a, "parent", 6);
  show("the child writes child. and exits, then the parent writes parent:");
  close(a);

  printf("4. the access mode lives in the struct file, so a dup has it too\n");
  a = open("ofd.txt", O_RDONLY);
  b = dup(a);
  printf("  write via the O_RDONLY fd: %d, via its dup: %d\n",
         write(a, "x", 1), write(b, "x", 1));
  close(a);
  close(b);

  unlink("ofd.txt");
  exit(0);
}
