# Lab 04: Interrupts, timers and device drivers

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 04](../../SYLLABUS.md#04-interrupts-timers-and-device-drivers)

> **Draft for review, 2026-11-16. What was and was not executed:**
> - **xv6 parts (exercises A–C): built and run** in an Ubuntu 24.04 container under podman on Windows 11 (the podman machine is a WSL 2 VM; this is not WSL 2 Ubuntu itself, and its timings are VM numbers), against xv6-riscv commit `fa2ddcb7`, the same pinned commit as [Lab 03](../03-virtual-memory/README.md#the-pinned-xv6-commit). [`xv6/run-xv6.sh`](xv6/run-xv6.sh) applied the reference solution (not shipped) to a copy of the tree and drove QEMU and GDB with expect. Output: [`xv6/example-output.txt`](xv6/example-output.txt).
> - **Linux kernel module (exercise D): not executed.** `tickdev.c` has not been compiled or loaded anywhere. The container shares the WSL 2 kernel and has no kernel headers, and no Linux VM was available. Only the reader, `tickread.c`, was compiled, to check that it builds. Everything marked *expected* in exercise D comes from reading the kernel's documentation and headers.
> - **Linux user-space counterpart (exercise E): built and run** in the same container. Output: [`linux/example-output.txt`](linux/example-output.txt).
> - **Windows observation (exercise F): built and run** on the Windows 11 host, without administrator rights, with gcc 16.2 from WinLibs. Output: [`windows/example-output.txt`](windows/example-output.txt). No driver was built or loaded, and nothing here needs `TESTSIGNING`.

## Goal

Follow one device interrupt and one timer interrupt from the hardware to the code that handles them, and be able to say, in xv6, Linux and NT terms, what that code may and may not do. In xv6 you watch a typed character travel from the PLIC through `devintr()` into the UART driver, count interrupts by source, and take away the timer's power to preempt. On Linux you write a character device whose `read()` sleeps until a kernel timer wakes it, the shape of the DRM event stream in stage 12. On Windows you can only watch: timer resolution, timer wake-ups, and the interrupt and DPC rates the system reports.

```
UART receives a byte ──► PLIC: source 10 pending; enabled for this hart's S mode; priority 1 > threshold 0
                    ──► the hart sets sip.SEIP; the trap is taken as soon as interrupts are enabled
from user mode:       uservec (trampoline) ──► usertrap()
from supervisor mode: kernelvec            ──► kerneltrap()
both:                 devintr(): scause = 0x8000000000000009 (interrupt, cause 9)
                        irq = plic_claim()        reads the claim register: 10
                        uartintr()                acknowledges, drains the receive FIFO
                          consoleintr(c)          edits the line, wakes consoleread() on '\n'
                        plic_complete(10)         the PLIC may deliver source 10 again

timer: time >= stimecmp ──► sip.STIP ──► scause = 0x8000000000000005 (interrupt, cause 5)
                        devintr() ──► clockintr(): hart 0 does ticks++ and wakeup(&ticks);
                                       every hart writes stimecmp = time + 1000000
                        returns 2 ──► usertrap() or kerneltrap() calls yield(): the preemption
```

## Files

| File | What it is | Status |
| --- | --- | --- |
| [`xv6/intrcount.c`](xv6/intrcount.c) | counts interrupts around one `write()`, one `read()` of a typed line, and `pause(10)` | built and run here |
| [`xv6/preempt.c`](xv6/preempt.c) | two CPU-bound children on one hart: when each ran, and how often it was preempted | built and run here |
| [`xv6/uart.gdb`](xv6/uart.gdb), [`xv6/timer.gdb`](xv6/timer.gdb) | GDB scripts for one received character and for the preempting timer interrupt | run here, in batch mode |
| [`xv6/run-xv6.sh`](xv6/run-xv6.sh) | applies your patch to a copy of the tree and runs all of the above, plus the break-it run; uses Lab 03's expect drivers | run here, with the reference solution |
| [`linux/tickdev.c`](linux/tickdev.c), [`linux/tickdev.h`](linux/tickdev.h) | the module: a misc character device and a periodic kernel timer that wakes blocked readers | not compiled here |
| [`linux/tickread.c`](linux/tickread.c) | the reader: sequence number, timestamp, interval, missed ticks and wake-up latency for each `read()` | compiled here, not run |
| [`linux/Makefile`](linux/Makefile), [`linux/run-module.sh`](linux/run-module.sh) | kbuild for the module and the reader, and a script that loads, exercises and unloads it in a VM | not run here |
| [`linux/timerfd_ticks.c`](linux/timerfd_ticks.c), [`linux/run-linux.sh`](linux/run-linux.sh) | the same blocked-reader pattern from user space with a `timerfd`, runnable without root | built and run here |
| [`windows/nt_timer_res.c`](windows/nt_timer_res.c) | timer facts the APIs report, `Sleep(1)` with and without `timeBeginPeriod`, periodic waitable timers | built and run here |
| [`windows/nt_intr_rates.c`](windows/nt_intr_rates.c) | interrupt and DPC rates from the Processor performance counters, per CPU | built and run here |
| `*/example-output.txt` | recorded output | recorded 2026-11-16 |

Reference solutions (not shipped): the counts and the preemption runs recorded here came from a reference patch for exercise B, which is kept out of the repository. Write your own. To run `run-xv6.sh` on it, save it as a patch against the pinned commit, `0001-intrstat.patch`, in `xv6/solution/`, which git ignores, or point `SOLUTION_DIR` at it. Without it, the script still runs exercise A on the stock tree and skips the other steps with a message.

## Setup

- The Lab 00 workbench at the pinned commit (see [Lab 03](../03-virtual-memory/README.md#the-pinned-xv6-commit)), with one hart: `make CPUS=1 qemu`. Make your patch against a plain tree, not on top of your Lab 03 changes.
- MIT's matching labs are `traps` (backtraces and a timer-driven `sigalarm`) and `net` (an E1000 network driver). Both are good extra practice. This lab does not need them.
- To run every xv6 part without typing, as was done here: `sudo apt-get install expect patch`, then `SOLUTION_DIR=<your patch> sh xv6/run-xv6.sh ~/xv6-riscv` (the default is `xv6/solution/`). It builds in `/tmp/lab04-xv6/`.
- **Linux module:** a Linux VM (QEMU or another hypervisor) with the headers for its running kernel, for example `sudo apt-get install build-essential linux-headers-$(uname -r)` on Ubuntu. Not WSL 2: its kernel ships no module headers, and the course does not plan a module there. A bug in a module can bring down the whole kernel, so use a VM you can throw away.
- **Linux user space:** WSL 2 Ubuntu or the VM, then `sh linux/run-linux.sh`.
- **Windows:** any x86-64 MinGW-w64 gcc. Nothing needs administrator rights. **Never turn on `TESTSIGNING` or turn off Secure Boot on the main desktop for this lab.** NT driver work is an optional side quest, in a Windows VM only.

## Read first

- The xv6 book, chapter 5 (interrupts and device drivers), and the timer-interrupt part of the scheduling chapter.
- `kernel/start.c`: `w_mideleg(0xffff)`, `w_sie(... SIE_SEIE | SIE_STIE)`, and `timerinit()`, which enables `stimecmp` (the Sstc extension) and asks for the first timer interrupt.
- `kernel/plic.c` with `PLIC*` in `kernel/memlayout.h`; `kernel/uart.c`; `kernel/console.c`.
- `kernel/trap.c`: `devintr()`, `clockintr()`, and the `which_dev == 2` lines in `usertrap()` and `kerneltrap()`. `kernel/kernelvec.S`.

## Exercises

Write each prediction in `NOTES.md` before you run anything.

### A. Follow a received character (xv6, GDB, run here)

[`uart.gdb`](xv6/uart.gdb) stops in `consoleintr()` when you type a character, prints the trap registers, the PLIC's pending word and the UART's line status, then catches the matching `plic_complete()`. By hand: `make CPUS=1 qemu-gdb` in one terminal, `gdb-multiarch` in the same directory in another, `source <path>/uart.gdb`, then type one character in the xv6 console.

**Predict:** Which trap path will the backtrace show, `usertrap()` or `kerneltrap()`? What is `scause`? Which mode was the hart in (`sstatus.SPP`)? Will the PLIC still show source 10 pending while `consoleintr()` runs?

Observed here, typing `echo x` (the first character is `e`, 101):

```
#0  consoleintr (c=101) at kernel/console.c:148
#1  0x0000000080000a1a in uartintr () at kernel/uart.c:164
#2  0x000000008000259c in devintr () at kernel/trap.c:211
#3  0x000000008000273a in kerneltrap () at kernel/trap.c:158
#4  0x00000000800056a8 in kernelvec () at kernel/kernelvec.S:38
scause  0x8000000000000009  (interrupt bit set, cause 9: supervisor external)
sstatus 0x0000000200000120  SPP=1, so the trap came from supervisor mode
sepc    0x000000008000437e
fileread + 16 in section .text
this hart's current process: 0x800128f8
PLIC pending word 0x0c001000: 0x00000400 (bit 10 is the UART)
UART LSR 0x10000005: 0x61 (bit 0: more input waiting, bit 5: transmitter idle)
#0  plic_complete (irq=irq@entry=10) at kernel/plic.c:45
#1  0x00000000800025ca in devintr () at kernel/trap.c:223
```

- **The interrupt arrived while the hart was running kernel code with interrupts on.** Here `sepc` is in `fileread()`: the shell's process was in its `read()` system call, and `kerneltrap()` ran on that process's kernel stack, although the character had nothing to do with that process. Other runs that day caught it in `pop_off()` and in `usertrap()`, right after interrupts came back on. The run without `SOLUTION_DIR`, at the end of the example output, caught it in `scheduler()` with no current process at all: the hart was idle.
- **`scause` has the top bit set.** That marks an interrupt rather than an exception. Cause 9 is a supervisor external interrupt, which on this machine means "ask the PLIC".
- **Source 10 was still pending during `consoleintr()`.** The UART still held the rest of `echo x` (LSR bit 0), so its interrupt line stayed high. `devintr()` had claimed source 10 and completes it only after `uartintr()` returns. The comment in `devintr()` states the rule: each device may raise at most one interrupt at a time until it is completed.
- `devintr()` keeps the claimed number in a register (GDB says `irq` is optimized out), so the script reads it where it is handed back: `plic_complete (irq=10)`.
- **Do not read RHR, ISR or the PLIC claim register from GDB.** Reading them changes the device's state. LSR and the pending word are safe to read.

**Your turn (not run here):** with `CPUS=3`, break on `plic_claim`, `finish`, and look for a claim that returns 0. `plicinithart()` enables the UART on every hart. Explain what `devintr()` does with irq 0, and why that is needed.

### B. Count interrupts by source (xv6, run here)

Add the counters yourself. Create `kernel/intrstat.h` with the struct shown at the top of [`intrcount.c`](xv6/intrcount.c). Count in `devintr()` (UART, virtio, timer), in `uartintr()` (characters received) and in `usertrap()` (timer interrupts that arrived in user mode). Add a `preempted` field to `struct proc`, count each timer yield in `usertrap()` and `kerneltrap()`, and reset it in `freeproc()`. Several harts take interrupts at once, so count with atomic adds. Then add `intrstat()`, which copies the counts out, with `ticks` read under `tickslock` and `preempted` from the calling process. Copy [`intrcount.c`](xv6/intrcount.c) and [`preempt.c`](xv6/preempt.c) to `user/` and add both to `UPROGS`. The counts below came from the reference solution (not shipped).

**Predict** each count before running `intrcount` with one hart. How many UART interrupts for one `write()` of 64 bytes? For a typed line of 10 characters? How many timer interrupts during `pause(10)`?

Observed here (`hello xv6` and Enter were typed by the expect script in one burst):

| Action | UART | virtio | timer (in user mode) | characters received | ticks |
| --- | ---: | ---: | ---: | ---: | ---: |
| `write()` of 64 bytes | 64 | 0 | 0 (0) | 0 | 0 |
| `read()` of a typed line | 2 | 0 | 7 (0) | 10 | 7 |
| `pause(10)` | 0 | 0 | 10 (0) | 0 | 10 |

- **One interrupt per byte written.** `uartwrite()` puts one byte in the transmit holding register, sets `tx_busy` and sleeps. The UART's transmit interrupt clears `tx_busy` and wakes it for the next byte. The kernel's own `printk` does not work this way: `consputc()` calls `uartputc_sync()`, which polls the line status with interrupts off.
- **Ten characters in two interrupts.** The receive FIFO holds several bytes, and `uartintr()` reads until `LSR_RX_READY` clears. Interrupt counts measure work batches, not bytes.
- **One timer interrupt per tick, none in user mode.** With one hart, every timer interrupt is hart 0's, and hart 0 is the only one that advances `ticks`. The process was asleep in the kernel the whole time. **Predict, then try:** with `CPUS=3`, how many timer interrupts per tick?

**Break it (not run here):** in `uartinit()`, drop `IER_TX_ENABLE`. Predict what the shell prints at boot, and where `uartwrite()` waits.

### C. The timer interrupt that forces preemption (xv6, run here)

`preempt` calibrates a busy loop, then forks two children that each spin for about 20 ticks of work without any system call, and prints when each one ran. **Predict** both children's start and end ticks with one hart, and how often each will be preempted.

Observed here:

```
$ preempt
calibrated: 65840000 iterations per tick; each child does 20 ticks of work
  child 0: ran from tick 0 to 34 (34 ticks), preempted 17 times
  child 1: ran from tick 1 to 34 (33 ticks), preempted 17 times
both done after 34 ticks
```

They overlap: each runs for one tick, takes the timer interrupt in user mode, and `usertrap()` calls `yield()`. "20 ticks of work" turned out to be about 17 ticks of CPU, because the calibration loop also paid for its `uptime()` calls. 17 preemptions each fit that: one per tick of CPU it got.

[`timer.gdb`](xv6/timer.gdb) stops in that `yield()` for the `preempt` process, then in the next `clockintr()`. Observed here:

```
#0  yield () at kernel/proc.c:503
#1  0x000000008000270c in usertrap () at kernel/trap.c:94
#2  0x0000003ffffff09c in ?? ()
process preempt, pid 3, interrupted at user pc 0x476:
uptime + 6 in section .text of /tmp/l4/intr/user/_preempt
scause  0x8000000000000005  (interrupt bit set, cause 5: supervisor timer)
=== the next timer interrupt, inside clockintr() ===
#0  clockintr () at kernel/trap.c:179
#1  0x00000000800025e6 in devintr () at kernel/trap.c:229
#2  0x0000000080002628 in usertrap () at kernel/trap.c:75
time 5370211
sip 0x20 before clockintr() runs (bit 5, STIP: timer interrupt pending)
sip 0x0 after clockintr() wrote the next deadline into stimecmp
```

- Frame 2, `0x3ffffff09c`, is in the trampoline page: `uservec`'s jump into `usertrap()`. GDB has no symbols at that address (Lab 01).
- This time the breakpoint fired while the parent was still calibrating (it had been preempted twice so far), and the interrupt had arrived in user mode inside the `uptime()` stub. Another run caught a child inside `main`, where the compiler had inlined `spin()`.
- `STIP` stays set until `stimecmp` moves past `time`. Writing the next deadline is what clears the request, as the comment in `clockintr()` says. That comment also says 1000000 is about a tenth of a second.

**Break it (run here):** remove the `yield()` and the `preempted++` from the timer case in `usertrap()`. `run-xv6.sh` does this when your patch has the reference solution's line `    yield(); // lab 04: the preemption`; otherwise it says so and skips. **Predict** first. Observed:

```
  child 0: ran from tick 0 to 17 (17 ticks), preempted 0 times
  child 1: ran from tick 17 to 35 (18 ticks), preempted 0 times
```

Child 1 did not start until child 0 finished. Nothing else could take the CPU from a process that makes no system calls. The timer interrupts still arrived and `ticks` still advanced; only the `yield()` was missing. `kerneltrap()` still yields on a timer interrupt, so a process can still be preempted while it is in the kernel with interrupts on. xv6's kernel code is preemptible too.

### D. A character device that blocks until the next tick (Linux VM, not executed)

Read [`tickdev.c`](linux/tickdev.c). Then, in a Linux VM:

```sh
cd linux
make
sudo insmod tickdev.ko period_ms=100
./tickread -n 10
sudo rmmod tickdev
```

or `sh run-module.sh`, which also runs two readers at once, a slow reader, non-blocking reads, and shows a blocked reader's state. Its design, and why each piece is there:

- **A misc device** (`misc_register()`) is a character device with major number 10 (`MISC_MAJOR` in [major.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/major.h)). Registering it creates `/sys/class/misc/tickdev`, and udev (or devtmpfs) makes `/dev/tickdev` with the `mode` the driver gives. This is the device model the build plan points to: a device, a class, attributes in sysfs, and a node in `/dev` made from a uevent ([driver model](https://docs.kernel.org/driver-api/driver-model/overview.html)).
- **A periodic `timer_list`.** `tick_fn()` updates a sequence number and a `ktime_get_ns()` timestamp under a spinlock, wakes the wait queue, and re-arms itself from the previous deadline so that the period does not drift. It runs from the timer softirq, which the [kernel hacking guide](https://docs.kernel.org/kernel-hacking/hacking.html) describes as a software interrupt. There it may not call anything that can sleep.
- **A wait queue.** `tick_read()` sleeps in `wait_event_interruptible()` until the sequence number differs from the last one this open file returned. A signal ends the wait with `-ERESTARTSYS`. With `O_NONBLOCK` it returns `-EAGAIN` instead, and `poll()` reports readiness, so stage 08's `epoll` loop can use it.
- **`copy_to_user()` outside the spinlock.** The hacking guide says these functions may sleep implicitly and must not be called with a spinlock held or with interrupts disabled.
- **Unloading.** `fops.owner = THIS_MODULE` keeps the module loaded while any file is open. `tick_exit()` stops the timer with `timer_shutdown_sync()` on Linux 6.2 and newer, and with `del_timer_sync()` before that. Checked on 2026-11-16: v6.1's `include/linux/timer.h` has no `timer_shutdown_sync()`, v6.2's has it, and master's no longer has `del_timer_sync()` or `from_timer()`. The module uses neither of the latter.

*Expected, from that source (not observed):* `dmesg` shows `tickdev: /dev/tickdev, minor N, one tick every 100 ms (J jiffies at HZ=H)`. `tickread` prints sequence numbers that go up by one with `dt` near 100,000 µs, rounded to whole jiffies, and a wake-up latency that a VM makes larger and noisier. Two readers print the same sequence numbers and timestamps. A reader that sleeps 250 ms per read sees gaps of 2 or 3 and a non-zero "missed" column, because each `read()` returns the newest tick. With `-N`, `read()` returns `EAGAIN` between ticks. `ps` shows a blocked reader in state `S`. `rmmod` fails with "in use" while a reader has the device open.

**Break it (VM only, not run here):** add `msleep(1)` to `tick_fn()`. Predict what the kernel log says on the next tick, and why the timer callback, not the reader, gets the blame.

### E. The same pattern from user space (run here)

`timerfd_ticks` does in user space what `tickread` does against the module: a blocked `read()` that a periodic timer wakes, with a count of the ticks since the last read ([timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)). A second thread looks at the blocked thread through `/proc`. Observed here:

```
  [observer] main thread 1284 during read(): state 'S', wchan 'do_wait_intr_irq'
     seq          wake ns      dt us  missed   latency us
       1 1996792204392544          -       0         53.1
       2 1996792304380934    99988.4       0         41.5
       ...
      10 1996793104375973    99973.8       0         36.5
softirqs on all CPUs during the run: TIMER +53, HRTIMER +0

=== timerfd_ticks -n 5 -s 250 (a slow reader) ===
       1 1996793206197892          -       0         50.4
       3 1996793456708635   250510.7       1      50561.1
       6 1996793707096642   250388.0       2        949.1
```

- The blocked thread is in state `S` (interruptible sleep). Its wait channel is the kernel function it sleeps in, the same kind of wait as `wait_event_interruptible()` in the module.
- A timerfd read returns the number of expirations, so a slow reader can count what it missed, as `tickread` can from `seq`. For the slow reader, "latency" is measured from the newest deadline it was told about, so it includes the reader's own sleep.
- The rootless container masks `/proc/interrupts` (it is `/dev/null` there), so the program reads `/proc/softirqs`. Those are machine-wide counts for the whole WSL 2 VM, not this program's. In your own VM, compare the `LOC` line of `/proc/interrupts` before and after.

### F. Interrupts, DPCs and timers on Windows (observation only, run here)

```powershell
cd windows
gcc -O2 -Wall -o build\nt_timer_res.exe nt_timer_res.c -lwinmm
build\nt_timer_res.exe
gcc -O2 -Wall -o build\nt_intr_rates.exe nt_intr_rates.c -lpdh -lwinmm
build\nt_intr_rates.exe
```

Observed on 2026-11-16 (Windows build 26300, Intel Core i7-11700, hypervisor present, the podman WSL 2 VM running; [full output](windows/example-output.txt), two runs of the first program):

| Measurement | Observed |
| --- | --- |
| `GetSystemTimeAdjustment` time increment | 156250 × 100 ns = 15.625 ms |
| `timeGetDevCaps` | `wPeriodMin` 1 ms, `wPeriodMax` 1,000,000 ms |
| `Sleep(1)`, median of 100 | 15.36 ms [15.42], with `timeBeginPeriod(1)`: 1.53 ms [1.53], after `timeEndPeriod`: 15.29 ms [15.36] |
| waitable timer, 10 ms period, 200 waits | 308 periods passed [308]; median interval 15.37 ms [15.33] |
| the same with `CREATE_WAITABLE_TIMER_HIGH_RESOLUTION` | 205 periods passed [205]; median interval 10.21 ms [10.21] |
| `Processor(_Total)`: Interrupts/sec, quiet | 116,808 |
| DPCs Queued/sec, quiet | 49,666 |
| % Interrupt Time, % DPC Time, quiet | 0.5 %, 0.8 % |
| the same while one thread loops on `Sleep(1)` at 1 ms resolution | 113,062 interrupts/s, 47,320 DPCs/s |

What these runs show, and what they do not:

- **A default wait lasts about 15.6 ms.** `Sleep(1)` took a whole timer interval, and a 10 ms periodic timer fired about every 15.4 ms. [timeBeginPeriod](https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod) says that since Windows 10, version 2004, the call affects only the processes that make it. It also says Windows 11 may ignore it for a window-owning process that is occluded, minimized or otherwise invisible or inaudible. Learn defines the `GetSystemTimeAdjustment` increment as the interval at which the system adds its time adjustment to the clock. It does not call that value the timer resolution, so treat the match with 15.6 ms as an observation.
- **A waitable timer does not say how many periods you missed.** 200 wake-ups covered 308 periods, so 108 expirations produced no separate wake-up. Unlike `tickdev`'s `seq` or a timerfd's count, the waiter cannot tell. [CreateWaitableTimerExW](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw) documents the high-resolution flag (Windows 10, version 1803 and later) "for time-critical situations when short expiration delays on the order of a few milliseconds are unacceptable". With it, the intervals were close to 10 ms, but 200 waits still took 205 periods.
- **About 117,000 interrupts a second on a quiet desktop,** spread unevenly over the 16 logical processors. The counter names come from `PdhAddEnglishCounterW`, which takes language-neutral paths ([Learn](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhaddenglishcounterw)). The 1 ms `Sleep` loop made no visible difference against that background. Which devices, and the hypervisor's share, cannot be told apart from these counters. A WPR trace with CPU and DPC/ISR events would show them, but it needs administrator rights, so it was not done here.

**IRQL and DPCs, the NT side of top and bottom halves.** NT code runs at an IRQL. Learn's [Managing Hardware Priorities](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/managing-hardware-priorities) lists them from low to high:

| IRQL | Masked on that processor | Driver routines that run there |
| --- | --- | --- |
| `PASSIVE_LEVEL` | nothing | `DriverEntry`, `AddDevice`, `Unload`, most dispatch routines, driver threads, work items |
| `APC_LEVEL` | `APC_LEVEL` interrupts | some dispatch routines |
| `DISPATCH_LEVEL` | `DISPATCH_LEVEL` and `APC_LEVEL` interrupts | `StartIo`, `DpcForIsr`, `CustomDpc`, `CustomTimerDpc`, `IoTimer` |
| DIRQL | interrupts at or below the device's level | the ISR (`InterruptService`), `SynchCritSection` |

- An ISR must finish quickly, so it queues a DPC that finishes the work later at a lower IRQL ([DPC objects](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-dpc-objects)). DPCs are also how kernel timers run driver code, as `CustomTimerDpc`. That makes `tickdev`'s `timer_list` callback the Linux counterpart of a timer DPC.
- At `DISPATCH_LEVEL` or above, waiting on a dispatcher object for a nonzero interval is a fatal error. Above `APC_LEVEL`, a page fault is fatal, and paged pool may not be touched. The only routines that may wait are at `PASSIVE_LEVEL` in a nonarbitrary thread context.
- Devices sit in device stacks: a PDO from the bus driver, exactly one function driver, and optional filter drivers ([device stacks](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/device-nodes-and-device-stacks)). New drivers use KMDF or UMDF rather than WDM ([WDF](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/)). Loading a test-signed driver needs `TESTSIGNING`, which can mean turning off Secure Boot ([Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option)). That belongs in a Windows VM, never on the main desktop.

## Linux / NT comparison

| | xv6 | Linux | NT |
| --- | --- | --- | --- |
| First code to run | `devintr()` with interrupts off on that hart, on the current kernel stack | the hard interrupt handler | the ISR at DIRQL |
| Deferred work | none: `uartintr()` does it all, and `wakeup()` makes the sleeper runnable | softirqs (the timer softirq runs `timer_list` callbacks), threaded handlers, workqueues | DPCs at `DISPATCH_LEVEL`, then work items at `PASSIVE_LEVEL` |
| A periodic kernel timer | `clockintr()` from each hart's `stimecmp` | `timer_list` (softirq) or hrtimer | a timer object with a DPC |
| Where a reader sleeps | `sleep()` on a channel, for example `&cons.r` | a wait queue, `wait_event_interruptible()` | a dispatcher object, at `PASSIVE_LEVEL` |

- Linux's threaded interrupt handlers split the work explicitly. [genericirq](https://docs.kernel.org/core-api/genericirq.html) says the primary handler of `request_threaded_irq()` is still called in hard interrupt context and returns `IRQ_WAKE_THREAD` to wake the handler thread that runs `thread_fn`.
- None of the three lets interrupt-level code sleep, for related reasons. In xv6 there is no process of its own to put to sleep: `devintr()` runs on whatever kernel stack was current, possibly the scheduler's, and `sched()` panics if more than one lock is held. In Linux, the hacking guide allows sleeping only in user context with no spinlocks held and interrupts enabled. In NT, the IRQL rules above make waiting at `DISPATCH_LEVEL` and above a fatal error.

## Checkpoints

Answer these in `NOTES.md`.

1. **For xv6, Linux and NT, name what may and may not sleep in interrupt context, and why.** Use your own evidence: the backtrace from exercise A (which stack, which process), the comments in `tickdev.c`, and the IRQL table.
2. **Show your module's blocked reader waking on every tick:** `tickread` output from a Linux VM with consecutive sequence numbers, plus the two-reader and slow-reader runs. Until you have a VM, show `timerfd_ticks` and say that the module part is still open.
3. Draw the path of one received character with the values you saw: PLIC pending bit, `scause`, `sstatus.SPP`, `sepc`, the claimed irq, and the moment it is completed.
4. Explain preemption in xv6: which interrupt, which function yields, and what changed in `preempt`'s output without it. Why could `kerneltrap()` still preempt a process?
5. Explain why `Sleep(1)` took about 15 ms on Windows, what `timeBeginPeriod(1)` changed, and for whom.
6. A slow reader of `tickdev` sees gaps in `seq`, while a waiter on a waitable timer cannot tell what it missed. Which design does a DRM vblank event (sequence number and timestamp) follow, and why does a compositor need it? Stages 12 and 13 come back to this.

## Common failure modes

- **`b uartintr` stops at boot before you type anything.** Transmit interrupts also go through `uartintr()`. Break on `consoleintr()`, which only received characters reach.
- **GDB says `irq = <optimized out>`.** Read the value at `plic_complete(irq)`, or `finish` out of `plic_claim()`.
- **`$stimecmp` prints void or "Value can't be converted to integer".** QEMU 8.2's gdbstub did not offer that CSR here. Read `$sip` and `$time` instead.
- **`preempt` shows no overlap even with the yield in place.** You ran with three harts, so the children ran in parallel. Use `make CPUS=1 qemu`.
- **`patch` fails in `run-xv6.sh`.** Your patch was made against another commit, or on top of your Lab 03 changes. Diff against a clean tree at the pinned commit.
- **`run-xv6.sh` prints `SKIPPED: ... needs 0001-intrstat.patch`.** The patch is not in `SOLUTION_DIR`, or has another name.
- **Module: `insmod: ERROR: could not insert module tickdev.ko: Invalid module format`.** It was built against other headers than the running kernel's. Check `modinfo tickdev.ko` (vermagic) against `uname -r`.
- **Module: `make` cannot find `/lib/modules/.../build`.** The kernel headers are not installed, or you are on WSL 2.
- **Module: `rmmod: ERROR: Module tickdev is in use`.** A reader still has `/dev/tickdev` open.
- **Module: a `BUG: scheduling while atomic` splat.** Something in `tick_fn()` slept.
- **Windows: `undefined reference to timeBeginPeriod` or `Pdh...`.** Link with `-lwinmm` and `-lpdh`.
- **Windows: `Sleep(1)` is already 1 ms without `timeBeginPeriod`.** Before Windows 10, version 2004, any process's request raised the resolution for everyone. Check your build and what else is running.

## Where this shows up later

- Stage 05: `acquire()` disables interrupts on its own hart (`push_off`) because an interrupt handler on the same hart might want the same lock.
- Stage 06: timer-driven preemption, run queues, and wake-up latency measured with `timerfd`.
- Stage 08: `poll()` on `tickdev` or a `timerfd` inside one `epoll` loop.
- Stages 12 and 13: DRM page-flip and vblank events carry a sequence number and a timestamp, like `tickdev`'s.
- Stage 16: udev and sysfs again, for input devices and seats.

## Next

Stage 05 of the [build plan](../../SYLLABUS.md#05-synchronization) covers synchronization: lock contention, `sleep` and `wakeup`, and futexes.
