// kmemstress.c - Lab 05, exercise A: make several processes allocate and
// free physical pages as fast as they can, then print the kernel's lock
// statistics. Every sbrk(4096) calls kalloc() and every sbrk(-4096) calls
// kfree(), and both take the allocator's lock.
//
// Needs the lockstat() system call from lab05-lockstat.patch.
//
// Install: copy to user/kmemstress.c (the patch already adds
//          $U/_kmemstress to UPROGS), then make CPUS=3 qemu.
// Run:     kmemstress            (3 processes, 20000 rounds each)
//          kmemstress 1 60000    (one process: the same work, no contention)

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int nproc = argc > 1 ? atoi(argv[1]) : 3;
  int rounds = argc > 2 ? atoi(argv[2]) : 20000;

  if (nproc < 1 || nproc > 16 || rounds < 1) {
    fprintf(2, "usage: kmemstress [nproc 1-16] [rounds]\n");
    exit(1);
  }

  lockstat(1); // zero the counters
  int t0 = uptime();

  for (int i = 0; i < nproc; i++) {
    int pid = fork();
    if (pid < 0) {
      fprintf(2, "kmemstress: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      for (int r = 0; r < rounds; r++) {
        char *a = sbrk(4096); // eager: kalloc() a page and map it
        if (a == SBRK_ERROR) {
          fprintf(2, "kmemstress: sbrk failed\n");
          exit(1);
        }
        a[0] = (char)r;
        if (sbrk(-4096) == SBRK_ERROR) { // unmap and kfree() it
          fprintf(2, "kmemstress: sbrk(-4096) failed\n");
          exit(1);
        }
      }
      exit(0);
    }
  }

  int bad = 0;
  for (int i = 0; i < nproc; i++) {
    int status;
    wait(&status);
    if (status != 0)
      bad++;
  }
  int t1 = uptime();

  printf("kmemstress: %d process(es) x %d rounds of sbrk(4096) + sbrk(-4096) "
         "in %d ticks%s\n",
         nproc, rounds, t1 - t0, bad ? ", SOME CHILDREN FAILED" : "");
  lockstat(0); // print
  exit(bad ? 1 : 0);
}
