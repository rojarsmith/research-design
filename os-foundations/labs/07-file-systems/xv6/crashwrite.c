// crashwrite.c - Lab 07, exercises E and F: give the log a transaction to
// lose, then check after the next boot what survived.
//
// Every block of /crashfile is 32 copies of one 32-byte line,
//   "blk   12 gen    2 crashwrite...\n"
// naming the block's own number and the round that wrote it. A block that
// holds another block's line, a mix of lines, or zeros was torn by a crash.
//
// usage:
//   crashwrite prepare        create /crashfile: 12 blocks of generation 1
//                             (exactly the direct blocks of a stock inode)
//   crashwrite append P [K]   arm crash point P (and K), then append blocks
//                             12-14, generation 2, with one write(): one
//                             transaction that allocates the indirect block
//   crashwrite trunc P [K]    arm crash point P (and K), then truncate
//                             /crashfile with open(O_TRUNC): one transaction
//                             that frees every block of the file
//   crashwrite loop           rewrite /crashfile round after round, and
//                             create and delete /crash.tmp, until killed
//   crashwrite other          create /other, 4 blocks of its own lines
//   crashwrite check          print /crashfile's size and check every block
//
// append and trunc need the crashpoint() system call from
// lab07-crashpoint.patch; P = 0 runs the same write without a crash point.
// Crash points: 1 after write_log(), before write_head() commits;
// 2 after the commit, before home block K is installed (K counts from 0);
// 3 after every block is installed, before the header is cleared.
//
// Install: copy to user/crashwrite.c and add $U/_crashwrite to UPROGS
// (xv6/run.sh does both).

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

#define LINE  32
#define NPREP 12 // blocks written by prepare
#define NAPP  3  // blocks appended in one transaction

int crashpoint(int, int); // from lab07-crashpoint.patch

static char buf[NAPP * BSIZE];

static void
putnum(char *p, int width, int v)
{
  for (int i = width - 1; i >= 0; i--) {
    p[i] = v ? '0' + v % 10 : (i == width - 1 ? '0' : ' ');
    v /= 10;
  }
}

// Fill one block with the line for block bn of generation gen.
static void
fill(char *b, int bn, int gen)
{
  char line[LINE];
  memmove(line, "blk xxxx gen xxxx crashwrite...\n", LINE);
  putnum(line + 4, 4, bn);
  putnum(line + 13, 4, gen);
  for (int i = 0; i < BSIZE / LINE; i++)
    memmove(b + i * LINE, line, LINE);
}

// What does block b say? Returns its generation, or -1 if the block is
// not 32 copies of the line for block bn.
static int
blockgen(char *b, int bn)
{
  char want[9];
  memmove(want, "blk xxxx ", 9);
  putnum(want + 4, 4, bn);
  if (memcmp(b, want, 9) != 0)
    return -1;
  for (int i = 1; i < BSIZE / LINE; i++)
    if (memcmp(b, b + i * LINE, LINE) != 0)
      return -1;
  return atoi(b + 13 + (b[13] == ' ') + (b[14] == ' ') + (b[15] == ' '));
}

static void
writeblocks(int fd, int first, int n, int gen)
{
  for (int i = 0; i < n; i++)
    fill(buf + i * BSIZE, first + i, gen);
  if (write(fd, buf, n * BSIZE) != n * BSIZE) {
    printf("crashwrite: write failed\n");
    exit(1);
  }
}

static void
arm(int argc, char **argv)
{
  int p = argc > 2 ? atoi(argv[2]) : 0;
  int k = argc > 3 ? atoi(argv[3]) : 0;
  if (p && crashpoint(p, k) < 0) {
    printf("crashwrite: crashpoint(%d, %d) failed\n", p, k);
    exit(1);
  }
  if (p)
    printf("crashwrite: crash point %d armed (k = %d) for the next commit\n", p,
           k);
}

static int
check(void)
{
  struct stat st;
  char b[BSIZE];
  int fd = open("crashfile", O_RDONLY);
  if (fd < 0) {
    printf("check: no /crashfile\n");
    return 1;
  }
  fstat(fd, &st);
  int n = (st.size + BSIZE - 1) / BSIZE;
  printf("check: /crashfile is %d bytes, %d blocks\n", (int)st.size, n);
  int bad = 0, start = 0, lastgen = -2;
  for (int i = 0; i <= n; i++) {
    int g = -2;
    if (i < n) {
      memset(b, 0, BSIZE);
      if (read(fd, b, BSIZE) != BSIZE) {
        printf("check: short read at block %d\n", i);
        bad++;
      }
      g = blockgen(b, i);
      if (g < 0) {
        bad++;
        printf("check: block %d is torn, it starts \"%c%c%c%c%c%c%c%c%c%c%c%c"
               "%c%c%c%c%c\"\n",
               i, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9],
               b[10], b[11], b[12], b[13], b[14], b[15], b[16]);
      }
    }
    if (i > 0 && (g != lastgen || i == n)) {
      if (lastgen >= 0)
        printf("check:   blocks %d-%d: generation %d\n", start, i - 1, lastgen);
      start = i;
    }
    lastgen = g;
  }
  close(fd);
  if (bad == 0)
    printf("check: every block holds its own line\n");
  else
    printf("check: %d block(s) torn\n", bad);
  return bad != 0;
}

int
main(int argc, char *argv[])
{
  int fd;
  char *cmd = argc > 1 ? argv[1] : "";

  if (strcmp(cmd, "prepare") == 0) {
    unlink("crashfile");
    fd = open("crashfile", O_CREATE | O_RDWR);
    for (int i = 0; i < NPREP; i += NAPP)
      writeblocks(fd, i, NAPP, 1);
    close(fd);
    printf("crashwrite: /crashfile has %d blocks of generation 1\n", NPREP);
  } else if (strcmp(cmd, "append") == 0) {
    fd = open("crashfile", O_RDWR);
    if (fd < 0) {
      printf("crashwrite: run crashwrite prepare first\n");
      exit(1);
    }
    char b[BSIZE];
    while (read(fd, b, BSIZE) == BSIZE) // move to the end of the file
      ;
    arm(argc, argv);
    printf("crashwrite: appending blocks %d-%d in one write()\n", NPREP,
           NPREP + NAPP - 1);
    writeblocks(fd, NPREP, NAPP, 2);
    close(fd);
    printf("crashwrite: append done\n");
  } else if (strcmp(cmd, "trunc") == 0) {
    arm(argc, argv);
    printf("crashwrite: truncating /crashfile with open(O_TRUNC)\n");
    fd = open("crashfile", O_RDWR | O_TRUNC);
    close(fd);
    printf("crashwrite: truncate done\n");
  } else if (strcmp(cmd, "other") == 0) {
    unlink("other");
    fd = open("other", O_CREATE | O_RDWR);
    char b[BSIZE];
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < BSIZE / LINE; j++)
        memmove(b + j * LINE, "other: a new file's own block.\n", LINE);
      write(fd, b, BSIZE);
    }
    close(fd);
    printf("crashwrite: /other has 4 blocks\n");
  } else if (strcmp(cmd, "loop") == 0) {
    // Print nothing more: console output is slow, and the kill should
    // land in the middle of file system work, not of a printf().
    printf("crashwrite: rewriting /crashfile until killed\n");
    for (int gen = 1;; gen++) {
      unlink("crash.tmp");
      fd = open("crash.tmp", O_CREATE | O_RDWR);
      writeblocks(fd, 0, 1, gen);
      close(fd);
      fd = open("crashfile", O_CREATE | O_RDWR | O_TRUNC);
      for (int i = 0; i < 30; i += NAPP)
        writeblocks(fd, i, NAPP, gen);
      close(fd);
    }
  } else if (strcmp(cmd, "check") == 0) {
    exit(check());
  } else {
    printf("usage: crashwrite prepare | append P [K] | trunc P [K] | loop | "
           "other | check\n");
    exit(1);
  }
  exit(0);
}
