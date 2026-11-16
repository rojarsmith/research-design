// lostwake.c - Lab 05, exercise C: a ping-pong over two pipes, with a
// watchdog.
//
// "ping" writes one byte into pipe ab and waits for one byte on pipe ba;
// "pong" does the opposite. Each read that finds its pipe empty goes to
// sleep in piperead() until the other side's pipewrite() wakes it. If one
// wakeup is lost, both processes wait for each other forever. The watchdog
// then kills them, and this program reports which ones it had to kill.
//
// With the unmodified kernel it should always finish. With
// lostwake-a.patch or lostwake-b.patch it should get stuck.
//
// Install: copy to user/lostwake.c (lab05-lockstat.patch adds
//          $U/_lostwake to UPROGS), then make CPUS=3 qemu.
// Run:     lostwake                 (2000 rounds, watchdog after 100 ticks)
//          lostwake 5000 200

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int rounds = argc > 1 ? atoi(argv[1]) : 2000;
  int limit = argc > 2 ? atoi(argv[2]) : 100; // ticks; one tick is ~0.1 s
  int ab[2], ba[2];
  char c = 'x';

  if (pipe(ab) < 0 || pipe(ba) < 0) {
    fprintf(2, "lostwake: pipe failed\n");
    exit(1);
  }
  int t0 = uptime();

  int ping = fork();
  if (ping == 0) {
    close(ab[0]);
    close(ba[1]);
    for (int i = 0; i < rounds; i++) {
      if (write(ab[1], &c, 1) != 1)
        exit(2);
      if (read(ba[0], &c, 1) != 1)
        exit(3);
      if ((i + 1) % 500 == 0)
        printf("  ping: %d rounds\n", i + 1);
    }
    exit(0);
  }

  int pong = fork();
  if (pong == 0) {
    close(ab[1]);
    close(ba[0]);
    for (int i = 0; i < rounds; i++) {
      if (read(ab[0], &c, 1) != 1)
        exit(2);
      if (write(ba[1], &c, 1) != 1)
        exit(3);
    }
    exit(0);
  }

  close(ab[0]);
  close(ab[1]);
  close(ba[0]);
  close(ba[1]);

  int dog = fork();
  if (dog == 0) {
    pause(limit);
    printf("  watchdog: %d ticks are up, killing ping (pid %d) and pong "
           "(pid %d)\n",
           limit, ping, pong);
    kill(ping);
    kill(pong);
    exit(0);
  }

  // Collect ping and pong. Once both are gone, stop the watchdog.
  int left = 2, dog_alive = 1, ok = 0;
  int ping_status = 99, pong_status = 99;
  while (left > 0 || dog_alive) {
    int status;
    int pid = wait(&status);
    if (pid < 0)
      break;
    if (pid == ping || pid == pong) {
      if (pid == ping)
        ping_status = status;
      else
        pong_status = status;
      if (status == 0)
        ok++;
      if (--left == 0 && dog_alive)
        kill(dog); // a no-op if the watchdog has already exited
    } else if (pid == dog) {
      dog_alive = 0;
    }
  }
  int t1 = uptime();

  if (ok == 2)
    printf("lostwake: %d rounds finished in %d ticks\n", rounds, t1 - t0);
  else
    printf("lostwake: STUCK after %d ticks: ping exited %d, pong exited %d "
           "(-1 means killed by the watchdog). A wakeup was lost.\n",
           t1 - t0, ping_status, pong_status);
  exit(ok == 2 ? 0 : 1);
}
