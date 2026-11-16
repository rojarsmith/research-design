// bigfile.c - Lab 07, exercise B: write one file as large as the file
// system allows, read it back, and delete it.
//
// Every block starts with its own block number and is filled with a
// pattern that depends on it, so a block written to the wrong place, or
// read from the wrong place, is caught on the way back.
//
// The same program runs on the stock kernel (MAXFILE = 12 + 256 = 268
// blocks) and on yours with a doubly-indirect block (11 + 256 + 256*256 =
// 65803 blocks). It prints the MAXFILE it was compiled with, which comes
// from kernel/fs.h, so build it in the tree you are testing.
//
// Install: copy to user/bigfile.c and add $U/_bigfile to UPROGS.
// Run:     bigfile            write until the kernel refuses, then check
//          bigfile 6000       stop after 6000 blocks
//          bigfile 6000 keep  and leave big.file on the disk afterwards

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

#define CHUNK 8 // blocks per write() call; filewrite() splits it further

static char buf[CHUNK * BSIZE];

// Fill one block of buf with the contents that block bn must have.
static void
fill(char *b, uint bn)
{
  for (int i = 0; i < BSIZE; i++)
    b[i] = (char)(bn * 7 + i);
  *(uint *)b = bn;
}

static int
check(char *b, uint bn)
{
  if (*(uint *)b != bn)
    return 0;
  for (int i = sizeof(uint); i < BSIZE; i++)
    if (b[i] != (char)(bn * 7 + i))
      return 0;
  return 1;
}

static uint
filesize(int fd)
{
  struct stat st;
  if (fstat(fd, &st) < 0) {
    printf("bigfile: fstat failed\n");
    exit(1);
  }
  return st.size;
}

int
main(int argc, char *argv[])
{
  uint limit = argc > 1 ? atoi(argv[1]) : 0; // 0: until refused
  int keep = argc > 2 && strcmp(argv[2], "keep") == 0;
  uint bn, nb, i;
  int fd, refused = 0;
  int t0, t1;

  printf("bigfile: MAXFILE = %d blocks (NDIRECT %d, NINDIRECT %d), "
         "%d bytes\n",
         (int)MAXFILE, NDIRECT, (int)NINDIRECT, (int)(MAXFILE * BSIZE));

  unlink("big.file");
  fd = open("big.file", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("bigfile: cannot create big.file\n");
    exit(1);
  }

  t0 = uptime();
  bn = 0;
  while (limit == 0 || bn < limit) {
    nb = CHUNK;
    if (limit && limit - bn < nb)
      nb = limit - bn;
    for (i = 0; i < nb; i++)
      fill(buf + i * BSIZE, bn + i);
    if (write(fd, buf, nb * BSIZE) == nb * BSIZE) {
      bn += nb;
      if (bn % 8192 < CHUNK)
        printf("  %d blocks written\n", bn);
      continue;
    }
    // Part of the chunk may have gone in. Continue one block at a time
    // from wherever the file now ends, until a single block is refused.
    bn = filesize(fd) / BSIZE;
    for (;;) {
      fill(buf, bn);
      if (write(fd, buf, BSIZE) != BSIZE)
        break;
      bn++;
    }
    refused = 1;
    break;
  }
  t1 = uptime();
  if (refused)
    printf("wrote %d blocks (%d bytes); the write of block %d was refused\n",
           bn, filesize(fd), bn);
  else
    printf("wrote %d blocks (%d bytes)\n", bn, filesize(fd));
  printf("  %d ticks for the writes\n", t1 - t0);
  close(fd);

  fd = open("big.file", O_RDONLY);
  if (fd < 0) {
    printf("bigfile: cannot reopen big.file\n");
    exit(1);
  }
  for (i = 0; i < bn; i++) {
    if (read(fd, buf, BSIZE) != BSIZE) {
      printf("read back: short read at block %d\n", i);
      exit(1);
    }
    if (!check(buf, i)) {
      printf("read back: block %d holds the wrong data (it says block %d)\n",
             i, *(uint *)buf);
      exit(1);
    }
  }
  if (read(fd, buf, 1) != 0) {
    printf("read back: data past block %d\n", bn);
    exit(1);
  }
  close(fd);
  printf("read back %d blocks: every block holds its own data\n", bn);

  if (!keep) {
    if (unlink("big.file") < 0) {
      printf("bigfile: unlink failed\n");
      exit(1);
    }
    printf("unlinked big.file\n");
  }
  exit(0);
}
