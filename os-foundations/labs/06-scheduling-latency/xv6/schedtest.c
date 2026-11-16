// schedtest.c - Lab 06, exercises A and B: CPU-bound processes and one
// interactive process share the CPU for a fixed time. Each CPU-bound
// process reports how much work it got done; the interactive one reports
// how often it managed to run and how long each wakeup waited for the CPU.
//
// Needs setprio() and schedinfo() from lab06-schedstat.patch.
//
// Install: copy to user/schedtest.c (the patch already adds $U/_schedtest
//          to UPROGS), then make CPUS=1 qemu.
// Run:     schedtest [nspin] [ticks] [mode]
//            nspin  CPU-bound processes, 0-3 (default 3)
//            ticks  how long the test runs; one tick is ~0.1 s (default 50)
//            mode   same    everyone at the default priority (default)
//                   inter   the interactive process asks for priority 5
//                   starve  the interactive process asks for 5, and the
//                           first CPU-bound process asks for 9
//          The round-robin scheduler ignores priorities, so on an
//          unmodified kernel all three modes should look the same.
//
// The children send their results to the parent through a pipe, and the
// parent prints them, so that output from several CPUs does not interleave.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/schedinfo.h"
#include "user/user.h"

struct result {
  int id;    // -1 for the interactive process
  int prio;
  int count; // units of work, or rounds of pause(1)
  struct schedinfo si;
};

static void
send(int fd, struct result *r)
{
  // At most four results of about 112 bytes fit in the 512-byte pipe
  // together, so each write is copied in while pipewrite() holds the
  // pipe lock, and results from different children never interleave.
  if (write(fd, r, sizeof(*r)) != sizeof(*r))
    exit(1);
  exit(0);
}

static void
spinner(int fd, int id, int end, int prio)
{
  volatile int x = 0;
  struct result r;

  memset(&r, 0, sizeof(r));
  if (prio >= 0)
    setprio(prio);
  while (uptime() < end) {
    for (int i = 0; i < 100000; i++) // one unit of work
      x++;
    r.count++;
  }
  r.id = id;
  r.prio = prio >= 0 ? prio : PRIO_DEFAULT;
  send(fd, &r);
}

static void
interactive(int fd, int end, int prio)
{
  volatile int x = 0;
  struct result r;

  memset(&r, 0, sizeof(r));
  if (prio >= 0)
    setprio(prio);
  while (uptime() < end) {
    pause(1); // sleep until the next tick: a frame-paced wakeup
    for (int i = 0; i < 1000; i++) // a little work per wakeup
      x++;
    r.count++;
  }
  if (schedinfo(&r.si) < 0)
    exit(1);
  r.id = -1;
  r.prio = r.si.prio;
  send(fd, &r);
}

static void
print_interactive(struct result *r)
{
  static const char *label[NLATHIST] = {
    "      < 10 us", " 10 - 100 us", "100 us - 1 ms", "   1 - 10 ms",
    "  10 - 50 ms", " 50 - 100 ms", "100 - 200 ms", "     > 200 ms",
  };
  struct schedinfo *si = &r->si;

  printf("  interactive (priority %d): %d rounds of pause(1)\n", r->prio,
         r->count);
  if (si->nwake == 0) {
    printf("    no wakeups measured\n");
    return;
  }
  printf("    %lu wakeups, mean wait for the CPU %lu us, max %lu us\n",
         si->nwake, si->latsum / si->nwake / 10, si->latmax / 10);
  for (int b = 0; b < NLATHIST; b++) {
    if (si->hist[b] == 0)
      continue;
    printf("    %s %lu ", label[b], si->hist[b]);
    int bar = (int)(si->hist[b] * 40 / si->nwake);
    for (int i = 0; i < bar; i++)
      printf("#");
    printf("\n");
  }
}

int
main(int argc, char *argv[])
{
  int nspin = argc > 1 ? atoi(argv[1]) : 3;
  int ticks = argc > 2 ? atoi(argv[2]) : 50;
  char *mode = argc > 3 ? argv[3] : "same";
  int iprio = -1, sprio = -1, fds[2];
  struct result res[4];

  if (strcmp(mode, "inter") == 0) {
    iprio = 5;
  } else if (strcmp(mode, "starve") == 0) {
    iprio = 5;
    sprio = 9;
  } else if (strcmp(mode, "same") != 0) {
    fprintf(2, "usage: schedtest [nspin] [ticks] [same|inter|starve]\n");
    exit(1);
  }
  if (nspin < 0 || nspin > 3 || ticks < 1) {
    fprintf(2, "schedtest: nspin 0-3, ticks >= 1\n");
    exit(1);
  }
  if (pipe(fds) < 0) {
    fprintf(2, "schedtest: pipe failed\n");
    exit(1);
  }

  int end = uptime() + ticks;
  printf("schedtest: %d cpu-bound + 1 interactive for %d ticks, mode %s\n",
         nspin, ticks, mode);
  if (fork() == 0)
    interactive(fds[1], end, iprio);
  for (int i = 0; i < nspin; i++)
    if (fork() == 0)
      spinner(fds[1], i, end, i == 0 ? sprio : -1);
  close(fds[1]);

  int n = 0;
  while (n < nspin + 1 && read(fds[0], &res[n], sizeof(res[n])) == sizeof(res[n]))
    n++;
  for (int i = 0; i < nspin + 1; i++)
    wait(0);

  for (int i = 0; i < n; i++)
    if (res[i].id < 0)
      print_interactive(&res[i]);
  for (int id = 0; id < nspin; id++)
    for (int i = 0; i < n; i++)
      if (res[i].id == id)
        printf("  cpu-bound %d (priority %d): %d units of work\n", id,
               res[i].prio, res[i].count);
  if (n != nspin + 1)
    printf("  (only %d of %d results arrived)\n", n, nspin + 1);
  exit(0);
}
