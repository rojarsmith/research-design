// badptr.c - Stage 01, exercise D: hand the xv6 kernel pointers it must
// not trust, and compare with touching the same address from user code.
//
// Compiled and run on 2026-11-16 against xv6-riscv master (commit fa2ddcb)
// in an Ubuntu 24.04 container; output in xv6/example-output.txt. The
// addresses follow that tree; check them against kernel/memlayout.h and
// kernel/riscv.h in yours before you trust the comments.
//
// Install: copy to user/badptr.c, add $U/_badptr to UPROGS in the
// Makefile, run make qemu, then type badptr at the xv6 shell.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define KERNBASE_VA  0x80000000UL   // KERNBASE: kernel text and RAM
#define TRAPFRAME_VA 0x3FFFFFE000UL // TRAPFRAME = MAXVA - 2 * PGSIZE
#define MAXVA_VA     0x4000000000UL // MAXVA = 1L << 38 (riscv.h)

struct probe {
  char *name;
  uint64 addr;
};

static struct probe probes[] = {
  { "0 (start of user text)", 0 },
  { "KERNBASE", KERNBASE_VA },
  { "TRAPFRAME", TRAPFRAME_VA },
  { "MAXVA", MAXVA_VA },
};
#define NPROBE ((int)(sizeof(probes) / sizeof(probes[0])))

int
main(int argc, char *argv[])
{
  int i, r, status;
  struct stat st;

  printf("1. write(1, addr, 8): the kernel copies IN from addr\n");
  for (i = 0; i < NPROBE; i++) {
    printf("   %s: [", probes[i].name);
    r = write(1, (void *)probes[i].addr, 8);
    printf("] returned %d\n", r);
  }

  printf("2. pipe(addr): the kernel copies OUT two ints to addr\n");
  for (i = 0; i < NPROBE; i++) {
    r = pipe((int *)probes[i].addr);
    printf("   %s: returned %d\n", probes[i].name, r);
  }

  printf("3. fstat(0, addr): the kernel copies OUT a struct stat\n");
  for (i = 0; i < NPROBE; i++) {
    r = fstat(0, (struct stat *)probes[i].addr);
    printf("   %s: returned %d\n", probes[i].name, r);
  }
  r = fstat(0, &st);
  printf("   a real struct stat on the stack: returned %d, type %d\n", r, st.type);

  printf("4. user code loads from KERNBASE itself (in a child)\n");
  if (fork() == 0) {
    volatile char *p = (volatile char *)KERNBASE_VA;
    char c = *p;
    printf("   read %d from KERNBASE: this line should not print\n", c);
    exit(0);
  }
  wait(&status);
  printf("   child exit status %d\n", status);
  exit(0);
}
