// trapinfotest.c - Stage 01, exercise C: test for the trapinfo() system
// call you add to xv6. It needs kernel/trapinfo.h (below) and your
// kernel changes; see the lab README for the five places to edit.
//
// Compiled and run on 2026-11-16 against xv6-riscv master (commit fa2ddcb),
// with a trapinfo() written only for that check and not shipped here;
// output in xv6/example-output.txt.
//
// kernel/trapinfo.h, shared by the kernel and this program:
//
//   struct trapinfo {
//     uint64 epc; // user pc that sret will return to
//     uint64 ra;  // user ra at the ecall
//     uint64 sp;  // user sp at the ecall
//     uint64 a0;  // a0 as uservec saved it, before the return value
//     uint64 a7;  // system call number
//   };
//
// Install: copy to user/trapinfotest.c, add $U/_trapinfotest to UPROGS.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/trapinfo.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct trapinfo ti;
  int r;

  r = trapinfo(&ti);
  printf("trapinfo(&ti) returned %d\n", r);
  printf("  epc %p\n", (void *)ti.epc);
  printf("  ra  %p\n", (void *)ti.ra);
  printf("  sp  %p\n", (void *)ti.sp);
  printf("  a0  %p   (&ti is %p: %s)\n", (void *)ti.a0, (void *)&ti,
         ti.a0 == (uint64)&ti ? "same" : "different");
  printf("  a7  %lu\n", ti.a7);

  // Bad destinations: the kernel must refuse these with -1.
  printf("trapinfo(0)          returned %d\n", trapinfo((struct trapinfo *)0));
  printf("trapinfo(KERNBASE)   returned %d\n",
         trapinfo((struct trapinfo *)0x80000000UL));
  printf("trapinfo(TRAPFRAME)  returned %d\n",
         trapinfo((struct trapinfo *)0x3FFFFFE000UL));
  exit(0);
}
