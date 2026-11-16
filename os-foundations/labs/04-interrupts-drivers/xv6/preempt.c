// preempt.c - Stage 04, exercise C: two CPU-bound children on one hart.
// With timer preemption they take turns and finish together; without it
// the second one starts only when the first is done.
//
// Needs the intrstat() system call and kernel/intrstat.h from exercise B
// (the struct is shown at the top of intrcount.c); this program reads only
// its preempted field.
//
// Install: copy to user/preempt.c and add $U/_preempt to UPROGS.
// Run with one hart (make CPUS=1 qemu): preempt [ticks of work, default 20]

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/intrstat.h"
#include "user/user.h"

static volatile uint64 sink;

// Pure user-mode work: no system calls, so the only way off the CPU is
// a timer interrupt.
static uint64
spin(uint64 n)
{
  uint64 x = 1;
  for (uint64 i = 0; i < n; i++)
    x += i ^ (x >> 3);
  return x;
}

int
main(int argc, char *argv[])
{
  int work = argc > 1 ? atoi(argv[1]) : 20;
  uint64 per_tick = 0;
  int t0, i;

  // calibrate: how much spin() fits in 5 ticks, alone on the hart
  t0 = uptime();
  while (uptime() == t0)
    ;
  t0 = uptime();
  while (uptime() - t0 < 5) {
    sink += spin(100000);
    per_tick += 100000;
  }
  per_tick /= 5;
  printf("calibrated: %lu iterations per tick; each child does %d ticks of "
         "work\n",
         per_tick, work);

  int start = uptime();
  for (i = 0; i < 2; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      struct intrstat s;
      int begin = uptime();
      sink += spin(per_tick * work);
      int end = uptime();
      intrstat(&s);
      printf("  child %d: ran from tick %d to %d (%d ticks), preempted %lu "
             "times\n",
             i, begin - start, end - start, end - begin, s.preempted);
      exit(0);
    }
  }
  for (i = 0; i < 2; i++)
    wait(0);
  printf("both done after %d ticks\n", uptime() - start);
  exit(0);
}
