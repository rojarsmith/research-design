// pipeline.c - Stage 02, exercise A: run "cat README | grep the | wc"
// with fork, exec, pipe and wait, the way user/sh.c would.
//
// Compiled and run on 2026-11-16 against xv6-riscv master (commit fa2ddcb)
// in an Ubuntu 24.04 container; output in xv6/example-output.txt.
//
// Install: copy to user/pipeline.c, add $U/_pipeline to UPROGS, make qemu.
// Run:     pipeline          (compare with: cat README | grep the | wc)
//          pipeline -leak    (the parent keeps every pipe end open)

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Start argv[0] with standard input from `in` and standard output to `out`
// (-1 keeps the inherited one). The child closes every pipe end in fds,
// so that only the ends it dup'ed onto 0 and 1 stay open.
static int
spawn(char **argv, int in, int out, int *fds, int nfds)
{
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    if (in >= 0) {
      close(0);
      dup(in);
    }
    if (out >= 0) {
      close(1);
      dup(out);
    }
    for (int i = 0; i < nfds; i++)
      close(fds[i]);
    exec(argv[0], argv);
    fprintf(2, "exec %s failed\n", argv[0]);
    exit(1);
  }
  return pid;
}

int
main(int argc, char *argv[])
{
  int a[2], b[2], fds[4];
  char *cat[] = { "cat", "README", 0 };
  char *grep[] = { "grep", "the", 0 };
  char *wc[] = { "wc", 0 };
  int leak = argc > 1 && strcmp(argv[1], "-leak") == 0;

  if (pipe(a) < 0 || pipe(b) < 0) {
    fprintf(2, "pipe failed\n");
    exit(1);
  }
  fds[0] = a[0];
  fds[1] = a[1];
  fds[2] = b[0];
  fds[3] = b[1];

  int p1 = spawn(cat, -1, a[1], fds, 4);
  int p2 = spawn(grep, a[0], b[1], fds, 4);
  int p3 = spawn(wc, b[0], -1, fds, 4);
  fprintf(2, "spawned cat %d, grep %d, wc %d\n", p1, p2, p3);

  if (leak)
    fprintf(2, "-leak: the parent keeps every pipe end open\n");
  else
    for (int i = 0; i < 4; i++)
      close(fds[i]);

  for (int i = 0; i < 3; i++) {
    int status;
    int pid = wait(&status);
    fprintf(2, "pid %d exited with status %d\n", pid, status);
  }
  exit(0);
}
