// intrcount.c - Stage 04, exercise B: count interrupts by source around
// three actions: one write() of 64 bytes, one read() of a typed line, and
// pause(10).
//
// Needs the intrstat() system call you add in exercise B,
//   int intrstat(struct intrstat *);
// and kernel/intrstat.h, shared by the kernel and this program. All counts
// are since boot and cover every hart, except preempted:
//
//   struct intrstat {
//     uint64 uart;       // UART interrupts claimed from the PLIC (irq 10)
//     uint64 virtio;     // virtio disk interrupts claimed from the PLIC (irq 1)
//     uint64 timer;      // timer interrupts
//     uint64 timer_user; // timer interrupts that arrived in user mode
//     uint64 rx_chars;   // characters uartintr() handed to consoleintr()
//     uint64 ticks;      // the kernel's ticks counter
//     uint64 preempted;  // timer yields of the calling process
//   };
//
// Install: copy to user/intrcount.c and add $U/_intrcount to UPROGS.
// Run with one hart (make CPUS=1 qemu): intrcount

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/intrstat.h"
#include "user/user.h"

static void
snap(struct intrstat *s)
{
  if (intrstat(s) < 0) {
    printf("intrcount: intrstat failed\n");
    exit(1);
  }
}

static void
row(char *what, struct intrstat *a, struct intrstat *b)
{
  printf("  %s: uart %lu, virtio %lu, timer %lu (in user mode %lu), "
         "chars received %lu, ticks %lu\n",
         what, b->uart - a->uart, b->virtio - a->virtio, b->timer - a->timer,
         b->timer_user - a->timer_user, b->rx_chars - a->rx_chars,
         b->ticks - a->ticks);
}

int
main(int argc, char *argv[])
{
  struct intrstat a, b;
  char line[64], buf[128];
  int n;

  printf("interrupts since boot: ");
  snap(&a);
  printf("uart %lu, virtio %lu, timer %lu, ticks %lu\n", a.uart, a.virtio,
         a.timer, a.ticks);

  memset(line, '-', sizeof(line) - 1);
  line[sizeof(line) - 1] = '\n';
  snap(&a);
  write(1, line, sizeof(line));
  snap(&b);
  row("1. write() of 64 bytes ", &a, &b);

  printf("2. type a line and press Enter: ");
  pause(2); // let the prompt's last interrupts arrive first
  snap(&a);
  n = read(0, buf, sizeof(buf));
  snap(&b);
  row("   read() of a typed line", &a, &b);
  printf("   read() returned %d bytes\n", n);

  snap(&a);
  pause(10);
  snap(&b);
  row("3. pause(10)            ", &a, &b);
  exit(0);
}
