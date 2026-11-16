// cowtest.c - Stage 03, exercises B and C: count the pages fork copies,
// first with xv6's full copy and then with your copy-on-write fork, and
// check that copy-on-write still keeps the two processes apart.
//
// Needs the vmstat() system call you add in exercise B,
//   int vmstat(struct vmstat *);
// and kernel/vmstat.h, shared by the kernel and this program:
//
//   struct vmstat {
//     uint64 fork_copied; // pages uvmcopy() copied into a child
//     uint64 fork_shared; // pages uvmcopy() mapped into a child without copying
//     uint64 cow_copied;  // copy-on-write pages copied when written
//     uint64 cow_reused;  // copy-on-write pages made writable again, no copy
//     uint64 free_pages;  // pages on the kalloc() free list right now
//   };
//
// The same program runs before and after the copy-on-write change; only
// the numbers should differ.
//
// Install: copy to user/cowtest.c and add $U/_cowtest to UPROGS.
// Run: cowtest

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/vmstat.h"
#include "user/user.h"

#define PG    4096
#define NPAGE 64

// A page of its own in .bss, for part 3.
static int gfds[PG / sizeof(int)] __attribute__((aligned(PG)));

static void
snap(struct vmstat *s)
{
  if (vmstat(s) < 0) {
    printf("cowtest: vmstat failed\n");
    exit(1);
  }
}

// One line of counter differences between two snapshots.
static void
row(char *what, struct vmstat *a, struct vmstat *b)
{
  printf("  %s: fork copied %lu, fork shared %lu, cow copied %lu, "
         "cow reused %lu\n",
         what, b->fork_copied - a->fork_copied, b->fork_shared - a->fork_shared,
         b->cow_copied - a->cow_copied, b->cow_reused - a->cow_reused);
}

static void
part1(void)
{
  struct vmstat s0, s1;
  int i, pid, status;
  char *heap;

  printf("1. fork a process with a %d-page heap; the child reads every "
         "heap page, then writes 4\n",
         NPAGE);
  heap = sbrk(NPAGE * PG);
  if (heap == SBRK_ERROR) {
    printf("  sbrk failed\n");
    exit(1);
  }
  for (i = 0; i < NPAGE; i++)
    heap[i * PG] = 'p';
  printf("  process size %lu pages\n", (uint64)sbrk(0) / PG);

  snap(&s0);
  pid = fork();
  if (pid < 0) {
    printf("  fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    struct vmstat c1, c2, c3;
    int sum = 0;

    snap(&c1);
    for (i = 0; i < NPAGE; i++)
      sum += heap[i * PG];
    snap(&c2);
    for (i = 0; i < 4; i++)
      heap[i * PG] = 'c';
    snap(&c3);
    row("the fork itself     ", &s0, &c1);
    row("child read 64 pages ", &c1, &c2);
    row("child wrote 4 pages ", &c2, &c3);
    printf("  free pages: %lu before the fork, %lu in the child just after "
           "it\n",
           s0.free_pages, c1.free_pages);
    exit(sum == NPAGE * 'p' ? 0 : 1);
  }
  wait(&status);
  snap(&s1);
  row("parent, after wait  ", &s0, &s1);
  int same = 1;
  for (i = 0; i < NPAGE; i++)
    if (heap[i * PG] != 'p')
      same = 0;
  printf("  child status %d; the parent still sees its own heap: %s\n", status,
         same ? "ok" : "WRONG");
  printf("  free pages: %lu before the fork, %lu after the child exited: %s\n",
         s0.free_pages, s1.free_pages,
         s0.free_pages == s1.free_pages ? "ok" : "LEAK");
  sbrk(-NPAGE * PG);
}

static void
part2(void)
{
  struct vmstat s;
  int n, pid, status;
  char *big;

  snap(&s);
  n = s.free_pages * 2 / 3;
  printf("2. fork a process that holds 2/3 of the free memory (%d of %lu "
         "pages)\n",
         n, s.free_pages);
  big = sbrk(n * PG);
  if (big == SBRK_ERROR) {
    printf("  sbrk failed\n");
    exit(1);
  }
  pid = fork();
  if (pid < 0) {
    printf("  fork failed: a full copy needs more pages than are free\n");
  } else if (pid == 0) {
    big[0] = 'x';
    big[(n - 1) * PG] = 'y';
    exit(0);
  } else {
    wait(&status);
    printf("  fork succeeded; child status %d\n", status);
  }
  sbrk(-n * PG);
}

static void
part3(void)
{
  int pid, status;

  printf("3. the kernel writes into a copy-on-write page: pipe() in the "
         "child\n");
  gfds[0] = gfds[1] = -7;
  pid = fork();
  if (pid < 0) {
    printf("  fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    if (pipe(gfds) < 0) {
      printf("  child: pipe() failed\n");
      exit(1);
    }
    printf("  child: pipe() stored fds %d and %d\n", gfds[0], gfds[1]);
    exit(0);
  }
  wait(&status);
  printf("  parent: child status %d; its own copy still holds %d and %d: %s\n",
         status, gfds[0], gfds[1],
         (gfds[0] == -7 && gfds[1] == -7) ? "ok" : "WRONG");
}

int
main(int argc, char *argv[])
{
  part1();
  part2();
  part3();
  exit(0);
}
