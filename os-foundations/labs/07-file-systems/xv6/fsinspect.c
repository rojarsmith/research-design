// fsinspect.c - Lab 07: read an xv6 file system image on the host, the
// way mkfs writes it and the kernel reads it, without booting xv6.
//
// Build it against the tree whose fs.img you inspect, so that it uses the
// same kernel/fs.h (NDIRECT, the size of struct dinode):
//   gcc -O2 -Wall -I <xv6 tree> -o fsinspect fsinspect.c
//
// usage: fsinspect <fs.img> [command]
//   (none)        superblock, layout, log header, blocks and inodes in use
//   log           the log header and every block it names: its home block,
//                 what that block is, and whether the log copy differs from
//                 the home copy; with n = 0, the first log blocks instead
//   check         a consistency check (an fsck): the image as it is, and,
//                 if the log header names blocks, as recovery will leave it
//   file <path>   one file's inode and the blocks it uses, level by level
//   ls [<path>]   a directory's entries with inode numbers, types and sizes
//
// The image is only read. "As recovery will leave it" is computed in
// memory by copying the logged blocks home, as recover_from_log() does.
//
// Layout: addrs[0 .. NDIRECT-1] are direct blocks and addrs[NDIRECT] is
// the singly-indirect block. If struct dinode has a slot after that, as in
// exercise B, addrs[NDIRECT + 1] is taken to be the doubly-indirect block.

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define stat xv6_stat // avoid a clash with the host's struct stat
#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/param.h"

#ifndef T_SYMLINK
#define T_SYMLINK 4
#endif

#define NADDRS     ((int)(sizeof(((struct dinode *)0)->addrs) / sizeof(uint)))
#define HAS_DOUBLE (NADDRS >= NDIRECT + 2)
#define NIND       ((int)NINDIRECT)

static unsigned char *disk; // the image as it is on the disk
static unsigned char *img;  // the image being looked at (disk, or recovered)
static long nimg;           // blocks in the image file
static struct superblock sb;
static uint datastart;

// ---- block and inode access ------------------------------------------

static unsigned char *
blk(uint b)
{
  if (b >= nimg) {
    fprintf(stderr, "fsinspect: block %u is past the end of the image\n", b);
    exit(2);
  }
  return img + (long)b * BSIZE;
}

static struct dinode *
dino(uint inum)
{
  return (struct dinode *)blk(IBLOCK(inum, sb)) + inum % IPB;
}

static int
bitset(uint b)
{
  return (blk(BBLOCK(b, sb))[(b % BPB) / 8] >> (b % 8)) & 1;
}

static const char *
tname(int type)
{
  switch (type) {
  case T_DIR:
    return "dir";
  case T_FILE:
    return "file";
  case T_DEVICE:
    return "device";
  case T_SYMLINK:
    return "symlink";
  }
  return "?";
}

// ---- inode numbers to path names ---------------------------------------

static char **pathof; // pathof[inum]: the first path found for it

static void walkdir(uint inum, const char *path, int depth);

// Read byte range [off, off + n) of an inode's contents, block by block.
// Returns 0 if a block in the range has no address.
static uint bmapr(struct dinode *d, uint bn);

static void
readinode(struct dinode *d, uint off, void *dst, uint n)
{
  unsigned char *p = dst;
  while (n > 0) {
    uint bn = off / BSIZE, o = off % BSIZE, m = BSIZE - o;
    if (m > n)
      m = n;
    uint a = bmapr(d, bn);
    if (a == 0 || a >= nimg)
      memset(p, 0, m);
    else
      memcpy(p, blk(a) + o, m);
    p += m;
    off += m;
    n -= m;
  }
}

static uint
bmapr(struct dinode *d, uint bn)
{
  if (bn < NDIRECT)
    return d->addrs[bn];
  bn -= NDIRECT;
  if (bn < NIND) {
    if (d->addrs[NDIRECT] == 0 || d->addrs[NDIRECT] >= nimg)
      return 0;
    return ((uint *)blk(d->addrs[NDIRECT]))[bn];
  }
  bn -= NIND;
  if (HAS_DOUBLE && bn < (uint)NIND * NIND) {
    uint top = d->addrs[NADDRS - 1];
    if (top == 0 || top >= nimg)
      return 0;
    uint mid = ((uint *)blk(top))[bn / NIND];
    if (mid == 0 || mid >= nimg)
      return 0;
    return ((uint *)blk(mid))[bn % NIND];
  }
  return 0;
}

static void
walkdir(uint inum, const char *path, int depth)
{
  struct dinode *d = dino(inum);
  if (d->type != T_DIR || depth > 32)
    return;
  for (uint off = 0; off + sizeof(struct dirent) <= d->size;
       off += sizeof(struct dirent)) {
    struct dirent de;
    readinode(d, off, &de, sizeof(de));
    if (de.inum == 0 || de.inum >= sb.ninodes)
      continue;
    char name[DIRSIZ + 1];
    memcpy(name, de.name, DIRSIZ);
    name[DIRSIZ] = 0;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;
    if (pathof[de.inum])
      continue;
    char *p = malloc(strlen(path) + DIRSIZ + 2);
    sprintf(p, "%s%s%s", path, strcmp(path, "/") == 0 ? "" : "/", name);
    pathof[de.inum] = p;
    walkdir(de.inum, p, depth + 1);
  }
}

static void
findpaths(void)
{
  if (pathof) {
    for (uint i = 0; i < sb.ninodes; i++)
      free(pathof[i]);
    free(pathof);
  }
  pathof = calloc(sb.ninodes, sizeof(char *));
  pathof[ROOTINO] = strdup("/");
  walkdir(ROOTINO, "/", 0);
}

static const char *
pname(uint inum)
{
  return pathof && inum < sb.ninodes && pathof[inum] ? pathof[inum]
                                                      : "(no path)";
}

// ---- what each block is ---------------------------------------------

enum { R_FREE, R_DATA, R_IND, R_DIND, R_DMID };

struct owner {
  uint inum;  // 0: none
  int role;   // R_DATA: data block fbn; R_IND / R_DIND / R_DMID: index blocks
  uint fbn;   // file block number, or the index of a middle block
  int nrefs;  // how many pointers name this block
};
static struct owner *own;

static int problems;

static void
problem(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  printf("  PROBLEM: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
  problems++;
}

static int checking; // report problems while scanning?

static void
claim(uint b, uint inum, int role, uint fbn)
{
  if (b < datastart || b >= sb.size) {
    if (checking)
      problem("inode %u (%s) points to block %u, outside the data area "
              "(%u-%u)",
              inum, pname(inum), b, datastart, sb.size - 1);
    return;
  }
  struct owner *o = &own[b];
  o->nrefs++;
  if (o->nrefs > 1) {
    if (checking)
      problem("block %u is used twice: by inode %u (%s) and inode %u (%s)", b,
              o->inum, pname(o->inum), inum, pname(inum));
    return;
  }
  o->inum = inum;
  o->role = role;
  o->fbn = fbn;
}

// Record every block that inode inum points to. Returns the number of
// data blocks found.
static uint
scaninode(uint inum)
{
  struct dinode *d = dino(inum);
  uint ndata = 0, need = (d->size + BSIZE - 1) / BSIZE;

  for (int i = 0; i < NDIRECT; i++) {
    if (d->addrs[i]) {
      claim(d->addrs[i], inum, R_DATA, i);
      ndata++;
    } else if (checking && (uint)i < need)
      problem("inode %u (%s): size %u needs file block %d, which has no "
              "address",
              inum, pname(inum), d->size, i);
  }
  uint ind = d->addrs[NDIRECT];
  if (ind) {
    claim(ind, inum, R_IND, 0);
    if (ind >= datastart && ind < sb.size) {
      uint *a = (uint *)blk(ind);
      for (int j = 0; j < NIND; j++)
        if (a[j]) {
          claim(a[j], inum, R_DATA, NDIRECT + j);
          ndata++;
        }
    }
  }
  if (HAS_DOUBLE && d->addrs[NADDRS - 1]) {
    uint top = d->addrs[NADDRS - 1];
    claim(top, inum, R_DIND, 0);
    if (top >= datastart && top < sb.size) {
      uint *a = (uint *)blk(top);
      for (int j = 0; j < NIND; j++) {
        if (a[j] == 0)
          continue;
        claim(a[j], inum, R_DMID, j);
        if (a[j] < datastart || a[j] >= sb.size)
          continue;
        uint *a2 = (uint *)blk(a[j]);
        for (int k = 0; k < NIND; k++)
          if (a2[k]) {
            claim(a2[k], inum, R_DATA, NDIRECT + NIND + j * NIND + k);
            ndata++;
          }
      }
    }
  }
  if (checking && ndata < need)
    problem("inode %u (%s): size %u bytes needs %u blocks, only %u have "
            "addresses",
            inum, pname(inum), d->size, need, ndata);
  return ndata;
}

static void
scanall(void)
{
  free(own);
  own = calloc(sb.size, sizeof(struct owner));
  for (uint inum = 1; inum < sb.ninodes; inum++)
    if (dino(inum)->type != 0)
      scaninode(inum);
}

static void
role(uint b, char *out, int n)
{
  if (b == 0)
    snprintf(out, n, "boot block");
  else if (b == 1)
    snprintf(out, n, "superblock");
  else if (b >= sb.logstart && b < sb.logstart + sb.nlog)
    snprintf(out, n, "log block");
  else if (b >= sb.inodestart && b < sb.bmapstart) {
    uint first = (b - sb.inodestart) * IPB;
    snprintf(out, n, "inode block (inodes %u-%u)", first,
             first + (uint)IPB - 1);
  } else if (b >= sb.bmapstart && b < datastart) {
    uint first = (b - sb.bmapstart) * BPB;
    snprintf(out, n, "bitmap block (bits for blocks %u-%u)", first,
             first + BPB - 1);
  } else if (b < sb.size && own && own[b].inum) {
    struct owner *o = &own[b];
    const char *p = pname(o->inum);
    switch (o->role) {
    case R_DATA:
      snprintf(out, n, "%s block %u of inode %u %s",
               dino(o->inum)->type == T_DIR ? "directory" : "data", o->fbn,
               o->inum, p);
      break;
    case R_IND:
      snprintf(out, n, "indirect block of inode %u %s", o->inum, p);
      break;
    case R_DIND:
      snprintf(out, n, "doubly-indirect block of inode %u %s", o->inum, p);
      break;
    case R_DMID:
      snprintf(out, n, "indirect block %u under the doubly-indirect block "
               "of inode %u %s",
               o->fbn, o->inum, p);
      break;
    }
  } else if (b < sb.size)
    snprintf(out, n, "data block, not used by any inode");
  else
    snprintf(out, n, "past the end of the file system");
}

// The first bytes of a block: quoted text if they look like text.
static void
preview(const unsigned char *p, char *out, int n)
{
  int text = 1;
  for (int i = 0; i < 20; i++)
    if (!isprint(p[i]))
      text = 0;
  if (text)
    snprintf(out, n, "\"%.20s\"", (const char *)p);
  else
    snprintf(out, n, "%02x %02x %02x %02x %02x %02x %02x %02x ...", p[0],
             p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
}

// ---- the log -----------------------------------------------------------

struct lhead {
  int n;
  int block[LOGBLOCKS];
};

static struct lhead *
loghead(void)
{
  return (struct lhead *)blk(sb.logstart);
}

static int
logvalid(struct lhead *lh)
{
  if (lh->n < 0 || lh->n > LOGBLOCKS || lh->n > (int)sb.nlog - 1)
    return 0;
  for (int i = 0; i < lh->n; i++)
    if (lh->block[i] <= 0 || (uint)lh->block[i] >= sb.size)
      return 0;
  return 1;
}

// A copy of the image with the logged blocks installed, as
// recover_from_log() leaves it (before it clears the header).
static unsigned char *
recovered(void)
{
  unsigned char *r = malloc(nimg * BSIZE);
  memcpy(r, disk, nimg * BSIZE);
  struct lhead *lh = (struct lhead *)(disk + (long)sb.logstart * BSIZE);
  for (int i = 0; i < lh->n; i++)
    memcpy(r + (long)lh->block[i] * BSIZE,
           disk + (long)(sb.logstart + 1 + i) * BSIZE, BSIZE);
  ((struct lhead *)(r + (long)sb.logstart * BSIZE))->n = 0;
  return r;
}

static void
cmd_log(void)
{
  struct lhead *lh = loghead();
  char r[160], pv[64];

  if (!logvalid(lh)) {
    printf("log header at block %u: n = %d, not a valid header\n", sb.logstart,
           lh->n);
    return;
  }
  if (lh->n == 0) {
    printf("log header at block %u: n = 0. Recovery will install nothing.\n",
           sb.logstart);
    printf("The log blocks still hold whatever write_log() put there last, "
           "but no header names them:\n");
    for (uint i = 1; i <= 8 && i < sb.nlog; i++) {
      preview(disk + (long)(sb.logstart + i) * BSIZE, pv, sizeof pv);
      printf("  log block %3u: %s\n", sb.logstart + i, pv);
    }
    return;
  }
  printf("log header at block %u: n = %d. A committed transaction: recovery "
         "will copy these %d blocks home.\n",
         sb.logstart, lh->n, lh->n);
  // Describe the blocks as they will be after recovery, so that a block
  // this transaction allocates is described by its new owner.
  unsigned char *after = recovered();
  img = after;
  findpaths();
  scanall();
  img = disk;
  for (int i = 0; i < lh->n; i++) {
    uint home = lh->block[i];
    uint lb = sb.logstart + 1 + i;
    img = after;
    role(home, r, sizeof r);
    img = disk;
    int same = memcmp(disk + (long)lb * BSIZE, disk + (long)home * BSIZE,
                      BSIZE) == 0;
    preview(disk + (long)lb * BSIZE, pv, sizeof pv);
    printf("  log block %3u -> home %5u  %-12s %s\n", lb, home,
           same ? "same as home" : "differs", r);
    printf("                               log copy starts %s\n", pv);
  }
  free(after);
  img = disk;
}

// ---- check -------------------------------------------------------------

static void
check(const char *what)
{
  uint nfree = 0, nused = 0, ninodes = 0, norphan = 0;
  int *refs = calloc(sb.ninodes, sizeof(int));

  printf("%s\n", what);
  problems = 0;
  findpaths();
  checking = 1;
  scanall();
  checking = 0;

  // The metadata blocks must be marked in use; data blocks marked in use
  // exactly when some inode points to them.
  for (uint b = 0; b < sb.size; b++) {
    int set = bitset(b);
    if (b < datastart) {
      if (!set)
        problem("metadata block %u is marked free in the bitmap", b);
      continue;
    }
    if (set)
      nused++;
    else
      nfree++;
    if (own[b].nrefs && !set) {
      char r[160];
      role(b, r, sizeof r);
      problem("block %u is in use (%s) but marked free in the bitmap", b, r);
    } else if (!own[b].nrefs && set)
      problem("block %u is marked in use in the bitmap but no inode points "
              "to it (leaked)",
              b);
  }

  // Directory entries against inodes and link counts.
  for (uint inum = 1; inum < sb.ninodes; inum++) {
    struct dinode *d = dino(inum);
    if (d->type == 0)
      continue;
    ninodes++;
    if (d->type < T_DIR || d->type > T_SYMLINK)
      problem("inode %u has unknown type %d", inum, d->type);
    if (d->type != T_DIR)
      continue;
    for (uint off = 0; off + sizeof(struct dirent) <= d->size;
         off += sizeof(struct dirent)) {
      struct dirent de;
      readinode(d, off, &de, sizeof(de));
      if (de.inum == 0)
        continue;
      char name[DIRSIZ + 1];
      memcpy(name, de.name, DIRSIZ);
      name[DIRSIZ] = 0;
      if (de.inum >= sb.ninodes) {
        problem("directory %s: entry \"%s\" names inode %u, past the inode "
                "table",
                pname(inum), name, de.inum);
        continue;
      }
      if (strcmp(name, ".") == 0) {
        if (de.inum != inum)
          problem("directory %s: \".\" names inode %u, not itself",
                  pname(inum), de.inum);
        continue; // "." does not count as a link in xv6
      }
      if (dino(de.inum)->type == 0)
        problem("directory %s: entry \"%s\" names inode %u, which is free",
                pname(inum), name, de.inum);
      refs[de.inum]++;
    }
  }
  for (uint inum = 1; inum < sb.ninodes; inum++) {
    struct dinode *d = dino(inum);
    if (d->type == 0)
      continue;
    if (d->nlink == 0) {
      norphan++;
      if (refs[inum])
        problem("inode %u has nlink 0 but %d directory entries", inum,
                refs[inum]);
      else
        printf("  note: inode %u has nlink 0 and no directory entry: an "
               "orphan, which ireclaim() frees at boot\n",
               inum);
      continue;
    }
    if (refs[inum] != d->nlink)
      problem("inode %u (%s): nlink %d, but %d directory entries name it",
              inum, pname(inum), d->nlink, refs[inum]);
  }
  free(refs);

  if (problems == 0)
    printf("  consistent: %u inodes in use (%u orphaned), %u data blocks in "
           "use, %u free\n",
           ninodes, norphan, nused, nfree);
  else
    printf("  %d problem(s); %u inodes in use, %u data blocks in use, %u "
           "free\n",
           problems, ninodes, nused, nfree);
}

static void
cmd_check(void)
{
  struct lhead *lh = loghead();
  int bad = 0;

  if (!logvalid(lh)) {
    printf("the log header at block %u is not valid (n = %d)\n", sb.logstart,
           lh->n);
    lh = 0;
  }
  img = disk;
  check("the image as it is on the disk:");
  bad += problems;
  if (lh && lh->n > 0) {
    unsigned char *r = recovered();
    char what[128];
    snprintf(what, sizeof what,
             "the image as recovery will leave it (%d logged blocks "
             "installed):",
             lh->n);
    img = r;
    check(what);
    img = disk;
    free(r);
  }
  (void)bad;
}

// ---- file and ls ---------------------------------------------------------

static uint
lookup(const char *path)
{
  findpaths();
  for (uint i = 1; i < sb.ninodes; i++)
    if (pathof[i] && strcmp(pathof[i], path) == 0)
      return i;
  return 0;
}

// Print a list of block numbers as ranges: 1100-1110 1200 ...
static void
ranges(uint *b, int n)
{
  int printed = 0;
  for (int i = 0; i < n;) {
    if (b[i] == 0) {
      i++;
      continue;
    }
    int j = i;
    while (j + 1 < n && b[j + 1] == b[j] + 1)
      j++;
    if (printed++ == 6) {
      printf(" ...");
      break;
    }
    if (j > i)
      printf(" %u-%u", b[i], b[j]);
    else
      printf(" %u", b[i]);
    i = j + 1;
  }
  printf("\n");
}

static int
used(uint *a, int n)
{
  int k = 0;
  for (int i = 0; i < n; i++)
    k += a[i] != 0;
  return k;
}

static void
cmd_file(const char *path)
{
  uint inum = lookup(path);
  if (inum == 0) {
    printf("%s: not found\n", path);
    return;
  }
  struct dinode *d = dino(inum);
  printf("%s: inode %u, type %d (%s), nlink %d, size %u bytes (%u blocks)\n",
         path, inum, d->type, tname(d->type), d->nlink, d->size,
         (d->size + BSIZE - 1) / BSIZE);
  printf("  inode lives in block %u; struct dinode is %d bytes, %d addrs "
         "(NDIRECT %d)\n",
         (uint)IBLOCK(inum, sb), (int)sizeof(struct dinode), NADDRS, NDIRECT);
  printf("  direct (%d of %d used):", used(d->addrs, NDIRECT), NDIRECT);
  ranges(d->addrs, NDIRECT);
  uint meta = 0;
  if (d->addrs[NDIRECT]) {
    uint *a = (uint *)blk(d->addrs[NDIRECT]);
    printf("  singly indirect, block %u (%d of %d entries used):",
           d->addrs[NDIRECT], used(a, NIND), NIND);
    ranges(a, NIND);
    meta++;
  } else
    printf("  singly indirect: none\n");
  if (HAS_DOUBLE) {
    uint top = d->addrs[NADDRS - 1];
    if (top) {
      uint *a = (uint *)blk(top);
      uint ndata = 0;
      for (int j = 0; j < NIND; j++)
        if (a[j])
          ndata += used((uint *)blk(a[j]), NIND);
      printf("  doubly indirect, block %u (%d of %d indirect blocks, %u data "
             "blocks):",
             top, used(a, NIND), NIND, ndata);
      ranges(a, NIND);
      meta += 1 + used(a, NIND);
    } else
      printf("  doubly indirect: none\n");
  }
  if (d->type == T_SYMLINK) {
    char t[MAXPATH + 1];
    uint n = d->size < MAXPATH ? d->size : MAXPATH;
    readinode(d, 0, t, n);
    t[n] = 0;
    printf("  symlink target: \"%s\"\n", t);
  }
  printf("  index blocks (indirect and doubly indirect) for this file: %u\n",
         meta);
}

static void
cmd_ls(const char *path)
{
  uint inum = lookup(path);
  if (inum == 0 || dino(inum)->type != T_DIR) {
    printf("%s: not a directory\n", path);
    return;
  }
  struct dinode *d = dino(inum);
  for (uint off = 0; off + sizeof(struct dirent) <= d->size;
       off += sizeof(struct dirent)) {
    struct dirent de;
    readinode(d, off, &de, sizeof(de));
    if (de.inum == 0)
      continue;
    char name[DIRSIZ + 1];
    memcpy(name, de.name, DIRSIZ);
    name[DIRSIZ] = 0;
    struct dinode *e = dino(de.inum);
    printf("  %-14s inode %3u  %-7s nlink %d  %u bytes\n", name, de.inum,
           tname(e->type), e->nlink, e->size);
  }
}

// ---- main --------------------------------------------------------------

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "usage: fsinspect fs.img [log | check | file <path> | ls "
                    "[<path>]]\n");
    return 2;
  }
  FILE *f = fopen(argv[1], "rb");
  if (f == 0) {
    perror(argv[1]);
    return 2;
  }
  fseek(f, 0, SEEK_END);
  nimg = ftell(f) / BSIZE;
  fseek(f, 0, SEEK_SET);
  disk = malloc(nimg * BSIZE);
  if (fread(disk, BSIZE, nimg, f) != (size_t)nimg) {
    perror("read");
    return 2;
  }
  fclose(f);
  img = disk;
  memcpy(&sb, blk(1), sizeof(sb));
  if (sb.magic != FSMAGIC) {
    printf("%s: bad superblock magic 0x%x\n", argv[1], sb.magic);
    return 1;
  }
  datastart = sb.bmapstart + sb.size / BPB + 1;

  const char *cmd = argc > 2 ? argv[2] : "";
  if (strcmp(cmd, "log") == 0)
    cmd_log();
  else if (strcmp(cmd, "check") == 0)
    cmd_check();
  else if (strcmp(cmd, "file") == 0 && argc > 3)
    cmd_file(argv[3]);
  else if (strcmp(cmd, "ls") == 0)
    cmd_ls(argc > 3 ? argv[3] : "/");
  else if (*cmd == 0) {
    struct lhead *lh = loghead();
    printf("%s: %ld blocks of %d bytes\n", argv[1], nimg, BSIZE);
    printf("  superblock: size %u, nblocks %u, ninodes %u, nlog %u\n", sb.size,
           sb.nblocks, sb.ninodes, sb.nlog);
    printf("  layout: 0 boot | 1 super | %u-%u log (header + %u) | %u-%u "
           "inodes (%d per block) | %u-%u bitmap | %u-%u data\n",
           sb.logstart, sb.logstart + sb.nlog - 1, sb.nlog - 1, sb.inodestart,
           sb.bmapstart - 1, (int)IPB, sb.bmapstart, datastart - 1, datastart,
           sb.size - 1);
    printf("  log header: n = %d\n", lh->n);
    findpaths();
    scanall();
    uint nused = 0, ninodes = 0;
    for (uint b = datastart; b < sb.size; b++)
      nused += bitset(b);
    for (uint i = 1; i < sb.ninodes; i++)
      ninodes += dino(i)->type != 0;
    printf("  data blocks in use (bitmap): %u of %u; inodes in use: %u of %u\n",
           nused, sb.size - datastart, ninodes, sb.ninodes - 1);
  } else {
    fprintf(stderr, "fsinspect: unknown command %s\n", cmd);
    return 2;
  }
  return problems ? 1 : 0;
}
