# Lab 05: Synchronization

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 05](../../SYLLABUS.md#05-synchronization)

> **Draft for review, 2026-11-16. What was and was not executed:**
> - **xv6 parts (exercises A–D): built and run** on 2026-11-16 in an Ubuntu 24.04 container under podman on Windows 11 (a rootful podman machine on WSL 2), with QEMU 8.2.2 and riscv64-linux-gnu-gcc 13.3.0, on xv6-riscv commit [`fa2ddcb7`](https://github.com/mit-pdos/xv6-riscv/commit/fa2ddcb7ca87a77af579380a6eafd5aff163b44f) (2026-11-05). The output is in [`xv6/example-output.txt`](xv6/example-output.txt). The GDB steps in exercise C were not run.
> - **Linux parts (exercises E–G): built and run** in the same container: kernel 6.6.87.2-microsoft-standard-WSL2, 16 vCPUs, gcc 13.3.0. These are numbers from a VM. The ThreadSanitizer run in exercise G failed in that container (see the exercise), so that step is not executed. Output: [`linux/example-output.txt`](linux/example-output.txt).
> - **Windows parts (exercises H–J): built and run** natively on the same machine with gcc 16.2 from WinLibs. Output: [`windows/example-output.txt`](windows/example-output.txt).
> - **Machine:** Windows 11 Pro build 26300, Intel Core i7-11700 (16 logical processors), hypervisor present, Balanced power plan. Other work was running on the machine at the same time; each Windows log records the total CPU load just before and after the run (13–88 %). Every number below is specific to this machine and these runs. Run the programs yourself and keep your own numbers.

## Goal

Measure a contended lock, make the contention go away, and know exactly why each lock in this lab is correct or broken. In xv6 you count how often CPUs spin on the page allocator's lock, split the lock per CPU, read `acquire()` and `push_off()` and break them on purpose, and run one `sleep`/`wakeup` pair that loses wakeups and write a second one. On Linux you race a mutex built on `futex(2)` against `pthread_mutex`. On Windows you run the same test with SRW locks, critical sections and a `WaitOnAddress` mutex, and you show why a lock shared by two processes needs something other than `WaitOnAddress`.

```
                  fast path, no waiter              slow path, must wait                       who can wake you
xv6 spinlock      amoswap.w.aq on lk->locked        keep spinning, interrupts off              (nobody sleeps)
xv6 sleep()       -                                 sleep_prepare(chan); release(lk); sleep()  wakeup(chan) in the kernel
Linux futex       atomic compare-and-swap           FUTEX_WAIT(addr, expected value)           FUTEX_WAKE(addr); FUTEX_*_PRIVATE: this process only
WaitOnAddress     interlocked compare-exchange      WaitOnAddress(addr, &expected, size, ms)   WakeByAddress*: "another thread in the same process"
named mutex       none: every acquire is a call     WaitForSingleObject(mutex)                 ReleaseMutex, from any process with a handle
```

## Files

| File | What it is | Status |
| --- | --- | --- |
| [`xv6/lab05-lockstat.patch`](xv6/lab05-lockstat.patch) | per-lock counters in `acquire()` and a `lockstat()` system call; adds `kmemstress` and `lostwake` to `UPROGS` | built and run here |
| [`xv6/kmemstress.c`](xv6/kmemstress.c) | processes that `sbrk` a page and give it back as fast as they can, then print the lock counters | built and run here |
| [`xv6/pushoff-nointr.patch`](xv6/pushoff-nointr.patch), [`xv6/pushoff-nonest.patch`](xv6/pushoff-nonest.patch) | two ways to break `push_off()`/`pop_off()` | built and run here |
| [`xv6/lostwake.c`](xv6/lostwake.c) | a pipe ping-pong with a watchdog that reports a lost wakeup | built and run here |
| [`xv6/lostwake-a.patch`](xv6/lostwake-a.patch), [`xv6/lostwake-window.patch`](xv6/lostwake-window.patch) | a broken `sleep`/`wakeup` pair for pipes to run and read, and a control with the same race window and the correct pair | built and run here |
| [`xv6/run.sh`](xv6/run.sh), [`xv6/xv6run.exp`](xv6/xv6run.exp) | build every variant from a clean tree and run it without a person at the console | run here |
| reference solutions (not shipped) | the per-CPU allocator of exercise B and the second broken pair of exercise D | the recorded output for those two came from reference patches kept outside this repository |
| [`linux/futex_mutex.c`](linux/futex_mutex.c) | a futex mutex against `pthread_mutex`, an adaptive mutex and a spinlock under contention | built and run here |
| [`linux/lostwake.c`](linux/lostwake.c) | lost wakeups with a condition variable and with a raw futex | built and run here |
| [`linux/lockorder.c`](linux/lockorder.c) | an AB-BA lock order, run apart, run together, and under ThreadSanitizer | built and run here; the ThreadSanitizer step failed |
| [`linux/run.sh`](linux/run.sh) | builds and runs the Linux part | run here |
| [`windows/lockbench.c`](windows/lockbench.c) | the same contention test with SRW, two critical sections, a `WaitOnAddress` mutex and a spinlock | built and run here |
| [`windows/lostwake.c`](windows/lostwake.c) | lost wakeups with a condition variable and with `WaitOnAddress` | built and run here |
| [`windows/crossproc.c`](windows/crossproc.c) | `WaitOnAddress` across two processes, a named event, a named mutex and a benaphore | built and run here |
| [`xv6/example-output.txt`](xv6/example-output.txt), [`linux/example-output.txt`](linux/example-output.txt), [`windows/example-output.txt`](windows/example-output.txt) | recorded output | recorded 2026-11-16 |

## Setup

- **xv6.** Use a clean [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) checkout at commit `fa2ddcb7ca87a77af579380a6eafd5aff163b44f`; the patches were made against it. Apply one patch set per fresh tree, for example `patch -p1 < lab05-lockstat.patch` (or `git apply`), and copy the user programs into `user/`. [`xv6/run.sh`](xv6/run.sh) does all of this for every variant in a temporary directory and leaves your checkout untouched:

  ```sh
  sh xv6/run.sh ~/xv6-riscv                     # every variant
  sh xv6/run.sh ~/xv6-riscv base lostwake-a     # some of them
  SOLUTION_DIR=~/lab05-mine sh xv6/run.sh ~/xv6-riscv percpu lostwake-b
  ```

  The `percpu` and `lostwake-b` variants run your own work from exercises B and D. Apply `lab05-lockstat.patch` to a clean tree and commit it, make your change, save it with `git diff > ~/lab05-mine/kalloc-percpu.patch` (or `lostwake-b.patch`), and point `SOLUTION_DIR` at that directory. Without it, the script says it skipped them.

  It needs MIT's tool packages from Lab 00 plus `expect` and `patch`. MIT's matching lab is [`lock`](https://pdos.csail.mit.edu/6.1810/2025/labs/lock.html) (`git checkout lock` in `xv6-labs-2025`). Its tree comes from 2025 and has its own statistics code and the older `sleep(chan, lk)`; this lab's patches do not apply to it.
- **The container used here.** The runs recorded below used an image with those packages and the pinned tree at `/opt/xv6-riscv`, started from Git Bash like this, with the lab folder mounted read-only:

  ```sh
  MSYS_NO_PATHCONV=1 podman run --rm -v "C:/path/to/research-design:/work:ro" oslab:24.04 \
    sh /work/os-foundations/labs/05-synchronization/xv6/run.sh /opt/xv6-riscv
  ```

  The Linux part used the same image with `--cap-add SYS_PTRACE`, so that `strace` works.
- **Linux:** gcc, and `strace` for exercise E. WSL 2 Ubuntu from Lab 00 works too; its numbers are VM numbers as well.
- **Windows:** any x86-64 MinGW-w64 gcc. `WaitOnAddress` is in `API-MS-Win-Core-Synch-l1-2-0.dll`, so link with `-lsynchronization` ([Learn](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress)).

## Read first

- The xv6 book: chapter 6 (locking) and the sleep and wakeup sections of chapter 7.
- `kernel/spinlock.c` and `kernel/spinlock.h`; `kernel/kalloc.c`; `sleep_prepare()`, `sleep()` and `wakeup()` in `kernel/proc.c`; `piperead()` and `pipewrite()` in `kernel/pipe.c`.
- **The pinned tree's sleep API is new.** Since commit [`68337b6`](https://github.com/mit-pdos/xv6-riscv/commit/68337b6fea42dfd6c6215e9e0f269d989438f349) ("more modular API and implementation for sleep/wakeup", 2026-08-11), a caller registers with `sleep_prepare(chan)` while it still holds its own lock, releases that lock, then calls `sleep()` with no arguments. `wakeup(chan)` clears `p->chan` of every registered process, asleep or not, and `sleep()` does not sleep if `p->chan` is already clear. Trees from before that date, including MIT's 2025 lab trees and the xv6 book of that year, have `sleep(chan, lk)` instead. Exercise D covers both.
- [futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html), and Ulrich Drepper's paper "Futexes Are Tricky". `linux/futex_mutex.c` follows the lock path of the paper's third mutex; its unlock uses an exchange where the paper uses a decrement.
- Learn: [WaitOnAddress](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress), [SRW locks](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks), [critical sections](https://learn.microsoft.com/en-us/windows/win32/sync/critical-section-objects), [condition variables](https://learn.microsoft.com/en-us/windows/win32/sync/condition-variables).

## Exercises

Write each prediction in `NOTES.md` before you run anything.

### A. Measure contention on the page allocator (xv6)

`lab05-lockstat.patch` adds two counters to `struct spinlock`. `acquire()` counts the calls that got the lock (`#acquire()`) and the `amoswap` attempts that found it already held (`#spin`), and updates both while it holds the lock, so no atomics are needed. `initlock()` records every lock that lives in the kernel's static data (below `end`); pipe locks live in `kalloc()`ed pages and are left out, because they are freed. `lockstat(1)` zeroes the counters and `lockstat(0)` prints them, summed over all locks with the same name.

`kmemstress` forks processes that each call `sbrk(4096)` and `sbrk(-4096)` 20,000 times. The first maps a new page through `uvmalloc()`, which calls `kalloc()`, and the second unmaps it, which calls `kfree()`. Both take `kmem.lock`.

```sh
sh xv6/run.sh ~/xv6-riscv base           # or by hand, in the xv6 shell:
$ kmemstress                             # with make CPUS=3 qemu
$ kmemstress 1 60000                     # the same 60,000 rounds in one process
```

**Predict:** with three CPUs, how many of the 120,000 `kmem` acquisitions will find the lock held? With one CPU? Which other lock will show spins, and why?

Observed in three boots of the same kernel, with `kmemstress` run twice in each boot. The [recorded output](xv6/example-output.txt) is the third boot:

| Run | `kmem` `#acquire()` | `kmem` `#spin`, boots 1 / 2 / 3 | Ticks (~0.1 s each) |
| --- | ---: | --- | --- |
| CPUS=3, 3 processes | 120,061 | 99,831, 56,969 / 54,094, 45,433 / 58,225, 43,481 | 17, 15 / 15, 14 / 15, 15 |
| CPUS=3, 1 process × 60,000 | 120,021 | 0 / 0 / 0 | 36 / 33 / 34 |
| CPUS=1, 3 processes | 120,061 | 0 / 0 / 0 | 35 / 32 / 33 |

What this shows, and what it does not:

- **Contention needs two CPUs in the lock at once.** One process on three CPUs, or three processes on one CPU, made the same 120,000 acquisitions with no spins. On one CPU a process holding `kmem.lock` cannot be preempted, because `acquire()` turned interrupts off.
- **`#spin` counts loop iterations, not waiting time.** One spin is one `amoswap`, whose cost under QEMU depends on how QEMU emulates atomics on the host. Compare spins only between runs on the same setup.
- **The `uart` lock** showed 5,000–70,000 spins for about 165 acquisitions in every multi-CPU run, and none with one CPU. `uartwrite()` holds `tx_lock` while it writes a byte to the UART's register, and `uartintr()` on another CPU waits for it. Find the two functions in `kernel/uart.c` and explain why few acquisitions can mean many spins.
- **`proc`** (64 locks) had about 250,000 acquisitions per run. Each `scheduler()` pass acquires every process's lock. Exercise B of Lab 06 changes that loop.

### B. Split the lock per CPU (xv6)

Replace the single free list with one list and one lock per CPU, as MIT's lock lab asks:

- `kfree()` puts the page on the list of the CPU it runs on.
- `kalloc()` takes from its own list, and when that is empty it steals a page from another CPU's list.
- `cpuid()` is only meaningful while interrupts are off, so wrap the use of it in `push_off()`/`pop_off()`.
- Never hold two `kmem` locks at once. A CPU that holds its own lock while it takes another CPU's lock can deadlock against that CPU doing the same.
- Name every per-CPU lock `"kmem"`, so that `lockstat` sums them into one line you can compare with exercise A.

Write it yourself; no solution ships with this lab. The numbers below came from a reference implementation that steals one page at a time and is not shipped. Run yours the same way with `SOLUTION_DIR=... sh xv6/run.sh ~/xv6-riscv percpu`.

Observed with the reference implementation:

| Run | `kmem` locks | `kmem` `#acquire()` | `kmem` `#spin`, boots 1 / 2 / 3 | Ticks |
| --- | ---: | --- | --- | --- |
| CPUS=3, 3 processes | 8 | 120,068 to 120,126 | 0 in all six runs | 15, 15 / 15, 15 / 15, 16 |
| CPUS=3, 1 process × 60,000 | 8 | 120,028 | 0 / 0 / 0 | 36 / 33 / 43 |

The spins went from 43,000–100,000 to 0, but the runs took 14–17 ticks with one lock and 15–16 with eight. The tick count is coarse (0.1 s), and most of each round is spent outside the lock: `kalloc()` and `kfree()` each fill the page with junk and `uvmalloc()` zeroes it, 12 KiB of stores per round. **Explain** why removing all contention did not make this workload measurably faster here, and design a change to `kmemstress` that would make the difference visible. The second part of MIT's 2025 lab, a read-write spinlock with writer priority (`rwlktest`), is a good extension; do it in MIT's tree.

### C. Read `acquire()` and explain `push_off()` (xv6)

In the pinned tree, `acquire()` is `push_off()`, a `holding()` check that panics with `"acquire"`, a loop on `__atomic_exchange_n(&lk->locked, 1, __ATOMIC_ACQUIRE)`, and a store of `mycpu()` in `lk->cpu`. `release()` clears `lk->cpu`, stores 0 with `__atomic_store_n(..., __ATOMIC_RELEASE)`, then calls `pop_off()`. `push_off()` reads and clears `sstatus.SIE` in one `csrrc` instruction (`rc_sstatus`), saves the old state in `mycpu()->intena` if this is the outermost `push_off()`, and increments `mycpu()->noff`. `pop_off()` panics if interrupts are on, decrements `noff`, and turns interrupts back on only when `noff` reaches 0 and `intena` says they were on before.

Answer from the source, then check:

1. Find `acquire` in `kernel/kernel.asm`. The comment in `spinlock.c` says the exchange becomes `amoswap.w.aq` and the release store becomes `fence rw,w` followed by `sw`. Is that what your compiler emitted?
2. Why must interrupts be off while a CPU holds a spinlock that an interrupt handler also takes? Name such a lock in `trap.c`.
3. Why does `push_off()` disable interrupts *before* it calls `mycpu()`? (The comment says why.)
4. Why a counter and a saved flag, rather than `intr_off()` in `acquire()` and `intr_on()` in `release()`? Find a place where xv6 holds two spinlocks at once.
5. `sched()` panics unless `mycpu()->noff == 1`. Why exactly one?

With GDB and `CPUS=1` (not run here): `b wakeup`, `c`, then `p cpus[0].noff` and `p cpus[0].intena` inside it, and `p/x $sstatus` before and after a `push_off()`.

**Break it, twice.** Predict what happens, then apply each patch by hand or run `sh xv6/run.sh ~/xv6-riscv pushoff-nointr pushoff-nonest`:

- [`pushoff-nointr.patch`](xv6/pushoff-nointr.patch): `push_off()` reads `sstatus` but no longer clears `SIE`.
- [`pushoff-nonest.patch`](xv6/pushoff-nonest.patch): `pop_off()` turns interrupts back on at every release, even with an outer lock still held.

Then delete the `intr_get()` check in `pop_off()` as well and run both again (the `pushoff-nointr-nocheck` and `pushoff-nonest-nocheck` variants do that with `sed`). Observed, in two sets of runs with `kmemstress` as the first command:

| Patch | CPUS=1, set 1 / set 2 | CPUS=3, set 1 / set 2 |
| --- | --- | --- |
| `pushoff-nointr` | `panic: pop_off - interruptible` / the same | the same / the same |
| `pushoff-nonest` | `ppanic: pop_off - interruptible` / the same | the same / the same |
| `pushoff-nointr`, check removed | `ppanic: acquire` / the same | `panic: sched interruptible` / `panic: acquire` |
| `pushoff-nonest`, check removed | `panic: acquire` / `ppanic: acquire` | `ppanic: acquire` / the same |

No run reached the shell prompt; every panic came during boot. xv6 checks the invariant in three places (`pop_off`, `acquire`'s `holding()` test, `sched`), so the broken kernel panics instead of hanging. For each `panic: acquire`, describe the interrupt that arrived while its CPU held the lock it then tried to take. The doubled `p` in `ppanic` was not investigated. A likely reading is that a second panic, raised by an interrupt that arrived while the first panic was printing, wrote its message after the first one's `p`. Check that with GDB if you want to know for sure.

### D. Lose a wakeup, then explain why the real pair cannot (xv6)

[`xv6/lostwake.c`](xv6/lostwake.c) plays ping-pong between two processes through two pipes for 2,000 rounds. Every read that finds its pipe empty sleeps in `piperead()` until the other side's `pipewrite()` calls `wakeup(&pi->nread)`. If one wakeup is lost, both processes wait for each other forever. A watchdog process kills them after 100 ticks, and their exit status of -1 shows that it had to.

The patches add `race_window()` to `pipe.c`, a 100 µs spin between "this reader has decided to sleep" and "this reader is asleep", so that the race happens often enough to see:

- **Pair A** ([`lostwake-a.patch`](xv6/lostwake-a.patch)): the reader is correct, but `pipewrite()` calls `wakeup_asleep()`, which wakes only processes already `SLEEPING`. A reader between `sleep_prepare()` and `sleep()` keeps `p->chan` set and goes to sleep anyway. That check is exactly what `wakeup()` did in older trees.
- **Pair B, yours to write:** keep `wakeup()` correct, and make `piperead()` release the pipe lock *before* it calls `sleep_prepare()`, with the same `race_window()` between the two. A writer that runs in between finds no one registered. Save it as `lostwake-b.patch` for `run.sh`, as in the setup. The pair B results below came from a reference version that is not shipped.
- **Control** ([`lostwake-window.patch`](xv6/lostwake-window.patch)): the same 100 µs window, with the correct pair.

Read `lostwake-a.patch` first. **Predict** which of the three get stuck, then run `SOLUTION_DIR=... sh xv6/run.sh ~/xv6-riscv window lostwake-a lostwake-b`. Observed with `CPUS=3` in three boots of each kernel, three runs of `lostwake` per boot except for the first row:

| Kernel | Boots 1 / 2 / 3 |
| --- | --- |
| `lab05-lockstat.patch` only (one run per boot) | finished in 6 / 5 / 5 ticks |
| + control window | finished in 11, 10, 10 / 10, 9, 10 / 10, 10, 10 ticks |
| + pair A | stuck in all nine runs; one got past round 1,000 and one past round 500 before it stuck, the other seven stuck before round 500 |
| + pair B (reference, not shipped) | stuck in all nine runs, each before round 500 |

Now explain both correct designs:

- **The pinned tree:** the reader calls `sleep_prepare()` while it holds `pi->lock`. A writer needs `pi->lock` to add data, so it cannot run between the reader's check and its registration. After the reader releases `pi->lock`, the writer may run at any time, but its `wakeup()` clears the reader's `p->chan` under `p->lock`, and `sleep()` checks `p->chan` under `p->lock` before it sleeps. Pair A broke the second half, pair B the first half.
- **Older trees, `sleep(chan, lk)`:** `sleep()` acquires `p->lock` *before* it releases `lk`, then sets `p->chan` and `SLEEPING`. `wakeup()` needs `p->lock` to look at the process, so it sees either a process that has not yet decided to sleep (and will see the new data under `lk`) or one that is already `SLEEPING`. If your tree has this API, write the broken pair there: a copy of `sleep()` that releases `lk` before it acquires `p->lock`, used only by `piperead()`, with the same window. Predict and check that it gets stuck.

Both designs follow the same rule as a futex and `WaitOnAddress` in exercises F and I: the check "should I still sleep?" and the act of going to sleep must be atomic with respect to the waker.

### E. A futex mutex against `pthread_mutex` (Linux)

[`linux/futex_mutex.c`](linux/futex_mutex.c) runs the same loop in every thread: lock, a short critical section on shared data (about a fifth of each iteration), unlock, then private work. Its futex mutex has three states: 0 free, 1 held, 2 held with possible waiters. `lock` tries a compare-and-swap from 0 to 1. If that fails it sets the word to 2 and calls `FUTEX_WAIT(addr, 2)`, which sleeps only if the word is still 2 when the kernel looks. `unlock` exchanges the word to 0 and calls `FUTEX_WAKE(addr, 1)` only if the old value was 2 ([futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)).

```sh
sh linux/run.sh
```

If you want to build the mutex yourself, write your own `lock` and `unlock` from futex(2) before you read `futex_lock()` in the file, then swap yours in and compare.

**Predict:** at 16 threads, which lock is fastest, where does each one spend its time (`sys` vs `user`), and how many `FUTEX_WAKE` calls find nobody to wake?

Observed in the container (16 vCPUs), second run, 0.25 s per cell; the first run is in the [output file](linux/example-output.txt):

| Lock, 16 threads | Mops/s | sys ms | user ms | voluntary switches | `FUTEX_WAIT` / 1k ops | `FUTEX_WAKE` / 1k ops |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `pthread_mutex` (default) | 4.04 | 2,756 | 372 | 103,040 | | |
| `PTHREAD_MUTEX_ADAPTIVE_NP` | 4.58 | 1,432 | 1,772 | 102,044 | | |
| futex mutex | 4.71 | 3,213 | 401 | 80,333 | 398 | 483 |
| futex mutex, 50 spins first | 4.03 | 371 | 3,129 | 37,453 | 199 | 231 |
| spinlock | 5.39 | 3 | 3,934 | 3 | | |
| one thread, any lock | 6.26–6.54 | 0 | about 250 | 2 | 0 | 0 |

- **No contention, no system call.** With one thread every lock made 2 voluntary context switches in the whole cell and the futex mutex made no futex calls.
- **The sleeping locks spend contention in the kernel**: 2.8–3.2 s of system time against about 0.4 s of user time for the default and futex mutexes. Spinning first (adaptive, futex + spin) moves that time into user mode. The pure spinlock never sleeps, and its slowest thread did less than half the work of its fastest (min/max 0.48).
- **More `FUTEX_WAKE` than `FUTEX_WAIT` calls.** State 2 means "maybe waiters": a thread that was woken sets the word back to 2 when it takes the lock, because it cannot know whether others still wait, so its own unlock makes a wake call even when nobody is left to wake.
- **`strace -c` with 8 threads for 0.5 s** counted 19,228 futex calls (5,930 errors) for `pthread_mutex` and 20,792 (7,654 errors) for the futex mutex. The errors are mostly `FUTEX_WAIT` returning `EAGAIN` because the word had already changed. Explain why that error is the lost-wakeup protection working.
- **Splitting the lock**, one lock per thread as in exercise B, raised 16 threads from 4.10 to 63.41 Mops/s with `pthread_mutex` and from 4.77 to 70.41 with the futex mutex.
- Throughput peaked at two threads for most locks and fell from there, even though the critical section is only a fifth of the loop. **Explain** what moves between cores on every acquisition, whoever wins.

### F. Lost wakeups with a condition variable and a futex (Linux)

[`linux/lostwake.c`](linux/lostwake.c) is the Linux twin of [`windows/lostwake.c`](windows/lostwake.c) in exercise I. Two threads pass a turn back and forth 20,000 times. Every wait has a 50 ms timeout, and a wait that times out while the turn already belongs to the waiter counts as a lost wakeup. A mode stops after 20 of them. Modes 1 and 4 are broken; 2 and 3 are not:

1. `pthread_cond`, with the turn tested *before* taking the mutex.
2. `pthread_cond`, with the turn tested while holding the mutex.
3. `FUTEX_WAIT` with the value just seen.
4. A futex where the notifier calls `FUTEX_WAKE` *before* it stores the new turn.

Observed (second run; the race window is a busy spin between the test and the wait):

| Mode | 5 µs window: rounds, lost | No window: rounds, lost |
| --- | --- | --- |
| 1. test outside the mutex | 146 rounds, 20 lost, stopped | 1,025 rounds, 20 lost, stopped |
| 2. test under the mutex | 20,000 rounds, 0 lost | 20,000 rounds, 0 lost |
| 3. futex with the value seen | 20,000 rounds, 0 lost | 20,000 rounds, 0 lost |
| 4. wake before publish | 70 rounds, 20 lost, stopped | 699 rounds, 20 lost, stopped |

Mode 3 is the same rule as xv6's `sleep()` in exercise D. futex(2) says that `FUTEX_WAIT`'s load of the word, its comparison with the expected value and the start of the sleep happen atomically with respect to other futex operations on the same word. So a store and wake that come between your test and your call make the call return at once. Mode 4 shows what the comparison cannot protect against: a wake sent before the change it announces.

### G. Lock order and lockdep (Linux)

[`linux/lockorder.c`](linux/lockorder.c) takes lock A then B in one thread and B then A in another.

- Run apart, nothing went wrong: `thread 1: took A then B`, `thread 2: took B then A`.
- Run together (`./lockorder together`), each thread held its first lock, waited 1 s for the second with `pthread_mutex_timedlock`, and gave up: both printed `deadlock`.
- **Not executed:** under ThreadSanitizer (`-fsanitize=thread`, gcc 13.3) the run aborted with `FATAL: ThreadSanitizer: unexpected memory mapping`. The container's kernel has `vm.mmap_rnd_bits = 32`, and gcc 13's ThreadSanitizer does not cope with that much address-space randomization. Turning randomization off with `setarch -R` was refused (`Function not implemented`) by the container's seccomp profile. On a Linux machine where you can lower `vm.mmap_rnd_bits` to 28, or run `setarch -R`, expect ThreadSanitizer to report a `lock-order-inversion (potential deadlock)` for the first, harmless run.

That report is the point: [lockdep](https://docs.kernel.org/locking/lockdep-design.html) and ThreadSanitizer both record the order in which locks are taken and report a cycle the first time the second order appears, even if the two threads never actually met. In xv6 terms, `wait_lock` must be acquired before any `p->lock` (the comment above `wait_lock` in `proc.c`). Write down the deadlock that a `kwait()` taking `p->lock` and then `wait_lock` would create, and the lockdep report you would expect for it in a Linux kernel built with `CONFIG_PROVE_LOCKING`.

### H. The same contention test on Windows (run here)

```powershell
cd windows
gcc -O2 -Wall -o lockbench.exe lockbench.c -lsynchronization
.\lockbench.exe
```

[`windows/lockbench.c`](windows/lockbench.c) runs the workload from exercise E with six locks. Its `WaitOnAddress` mutex is the futex mutex with `WaitOnAddress(addr, &two, 4, INFINITE)` and `WakeByAddressSingle`.

**Predict** the order of the six locks at 16 threads before you look.

Observed, two runs, 0.25 s per cell ([full output](windows/example-output.txt)):

| Lock, 16 threads | Mops/s, run 1 | Mops/s, run 2 | kernel ms / user ms, run 1 |
| --- | ---: | ---: | --- |
| `SRWLOCK` | 5.39 | 5.36 | 125 / 4,078 |
| `CRITICAL_SECTION` | 2.36 | 2.16 | 312 / 3,438 |
| critical section, spin count 4000 | 2.73 | 2.63 | 141 / 3,547 |
| `WaitOnAddress` mutex | 4.37 | 4.43 | 250 / 3,125 |
| `WaitOnAddress` mutex, 50 spins first | 3.66 | 3.78 | 109 / 3,359 |
| spinlock | 3.05 | 2.76 | 125 / 3,172 |
| one thread, any lock | 5.76–6.67 | 5.89–6.45 | |

- **Splitting the lock** per thread raised `SRWLOCK` from 5.40 to 77.64 Mops/s (run 2: 5.49 to 73.08) and the `WaitOnAddress` mutex from 4.35 to 78.34 (run 2: 4.39 to 78.88).
- **Fairness:** with 32 threads, SRW's least busy thread finished 0 iterations in both runs (min/max 0.00), and so did the spinlock's and the spin-count critical section's in run 2. Learn says SRW locks "are neither fair nor FIFO" ([SRW locks](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks)).
- **Kernel time stayed low** on Windows for every lock (at most 625 ms), while Linux's sleeping mutexes in exercise E spent most of their CPU time in the kernel. The Learn pages cited here do not say how `WaitOnAddress` or SRW locks wait, and `GetProcessTimes` does not say how it samples. Treat this as a question rather than a result. A WPR trace (Lab 06) with sampled stacks would show where the CPU time goes.
- `waits/1k` above 1,000 for the `WaitOnAddress` mutex means more `WaitOnAddress` calls than lock operations. Many of those calls returned at once, because the word was no longer 2. Learn also allows spurious returns (the note in the Remarks of the [WaitOnAddress page](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress)), which is why the loop re-checks.

### I. Lost wakeups on Windows (run here)

```powershell
gcc -O2 -Wall -o lostwake.exe lostwake.c -lsynchronization
.\lostwake.exe
.\lostwake.exe 20000 0
```

The four modes match exercise F, with `SleepConditionVariableSRW` and `WaitOnAddress`. Observed:

| Mode | 5 µs window: rounds, lost, time | No window: rounds, lost, time |
| --- | --- | --- |
| 1. condition variable, test outside the lock | 30, 20, stopped after 1,303 ms | 73, 20, stopped after 1,343 ms |
| 2. condition variable, test under the lock | 20,000, 0, 446 ms | 20,000, 0, 24 ms |
| 3. `WaitOnAddress` with the value seen | 20,000, 0, 107 ms | 20,000, 0, 11 ms |
| 4. `WaitOnAddress`, wake before publish | 24, 20, stopped after 1,238 ms | 38, 20, stopped after 1,221 ms |

The broken modes lost a wakeup even without any extra window, after a few dozen rounds. A bug like this would show up in production as a thread that hangs until something unrelated wakes it.

### J. Why a cross-process lock needs something other than `WaitOnAddress` (run here)

```powershell
gcc -O2 -Wall -o crossproc.exe crossproc.c -lsynchronization
.\crossproc.exe
```

Two processes map the same page-file-backed section (a named file mapping). Atomic instructions on that page work across processes; only the waking is in question. Learn says a waiter returns when "another thread in the same process" calls `WakeByAddressSingle` or `WakeByAddressAll`, or when the timeout elapses ([WaitOnAddress](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress)). Observed, two runs:

| Part 1: one waiter, one waker | Run 1 | Run 2 |
| --- | --- | --- |
| a. same process, `WaitOnAddress` / `WakeByAddressAll` | noticed 0.031 ms after the wake | 0.014 ms |
| b. two processes, `WaitOnAddress` / `WakeByAddressAll` | noticed 1,787.8 ms after the wake, when its 2 s timeout ran out | 1,798.7 ms |
| c. two processes, a named auto-reset event | woken 0.018 ms after `SetEvent` | 0.026 ms |

| Part 2: two processes, one counter, 1 s | Ops, parent + child, run 1 | Run 2 | Waits, run 1 / 2 | Waits ended by the 20 ms timeout |
| --- | --- | --- | --- | --- |
| `WaitOnAddress` mutex in the shared page | 2,849,646 + 2,385,180 | 2,488,573 + 2,584,878 | 163 / 124 | 33 / 33 |
| named mutex (`CreateMutexW`) | 131,577 + 131,618 | 124,558 + 124,581 | not counted | |
| benaphore: interlocked count plus a named semaphore | 399,078 + 400,104 | 289,199 + 289,666 | 234,416 / 235,051 | |

Every counter was correct, so all three gave mutual exclusion. The difference is waiting:

- The `WaitOnAddress` mutex looks fast, but in both runs 33 of its waits in one second ended only because the 20 ms timeout ran out, which is 0.66 s spent asleep while the lock was free. The other process's `WakeByAddressSingle` never reached them. With `INFINITE`, as in exercise H, a waiter in the other process would sleep until something in its own process happened to wake it, which in this program is never.
- The named mutex works everywhere, because it is a kernel dispatcher object, but every acquire and every release is a system call: about 250,000 iterations per second for the two processes together, about 4 µs each.
- The benaphore keeps the uncontended path in user mode (an interlocked increment in the shared page) and uses a named semaphore only when it must wait. It is the cross-process equivalent of the futex mutex, with a kernel object standing in for the futex.

Linux needs no such split. futex(2) describes futexes shared between processes through shared memory, and `FUTEX_PRIVATE_FLAG` as the way to tell the kernel that a futex is not shared ([futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)). This lab's futex mutex uses the private operations; a cross-process version was not tested here.

## Linux / NT comparison

- **User-space fast path.** A futex keeps uncontended locking in user space and enters the kernel only to wait or wake ([futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)). `WaitOnAddress` is the same idea inside one process. SRW locks and critical sections also stay in user mode when uncontended. Exercises E and H show the one-thread rows with no system calls (Linux) or almost no kernel time (Windows).
- **Scope.** `FUTEX_PRIVATE_FLAG` limits a futex to one process; without it, a futex in shared memory works across processes. `WaitOnAddress` and SRW locks work within one process only ([WaitOnAddress](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress), [SRW locks](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks)). Across processes, Windows uses named or duplicated kernel objects: mutexes, events, semaphores.
- **Kernel spinlocks.** An xv6 spinlock disables interrupts on its own CPU. An NT kernel spin lock must be held briefly and live in nonpaged memory ([spin locks](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-spin-locks)), and waiting on dispatcher objects is not allowed at `DISPATCH_LEVEL` or above ([dispatcher objects](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-kernel-dispatcher-objects)). Compare with xv6's `sched()` refusing to switch while `noff != 1`.
- **Read-mostly data.** Linux offers RCU for data that is read far more often than written ([RCU](https://docs.kernel.org/RCU/whatisRCU.html)); neither xv6 nor this lab uses it.

## Checkpoints

Answer these in `NOTES.md`.

1. Show contention before and after with numbers: the `kmem` line of `lockstat` from exercises A and B, with `CPUS` and the round counts. Then the 16-thread single-lock and per-thread rows from exercise E or H. Say what the numbers do not show, for example why the xv6 tick counts did not change.
2. Explain `push_off()`: why interrupts go off before `mycpu()`, why a counter, why `intena` is saved only by the outermost call, and which panic in exercise C's table each check produced.
3. For each broken pair in exercise D, draw the interleaving that loses the wakeup, step by step, with the lock each process holds at each step. Then show why the same interleaving is harmless with the real `sleep_prepare()`/`sleep()`/`wakeup()`, and with the older `sleep(chan, lk)`.
4. Describe a deadlock that lockdep would report: two locks, the two orders, the code path for each, and what the report would say. Use exercise G or an xv6 example.
5. Explain why a lock shared by two Windows processes needs something other than `WaitOnAddress`, with the evidence from exercise J, and name two designs that work.
6. Mode 3 of exercises F and I cannot lose a wakeup, and mode 4 can. State the one rule that separates them, and find the same rule in xv6's `sleep()`.

## Common failure modes

- **`patch` fails or applies with offsets.** The patches were made against commit `fa2ddcb7`. On another tree, check `git log -1` and read the rejected hunks; MIT's 2025 `lock` branch has a different `spinlock.c` and the older `sleep(chan, lk)`.
- **The lab 05 patches and lab 06's together.** They are meant for separate fresh trees and both use system call number 23.
- **`lockstat` prints nothing for pipes.** Pipe locks are in `kalloc()`ed memory and are deliberately not registered.
- **`kmemstress` shows no spins.** Check `CPUS`: one CPU cannot contend. Check that you ran three processes.
- **Your per-CPU allocator deadlocks or panics `acquire`.** You held one CPU's `kmem` lock while taking another's, or called `cpuid()` with interrupts on and moved CPUs in between.
- **`lostwake` never gets stuck with a broken pair.** Use `CPUS=3`, and keep `race_window()`; without it the window is a few instructions wide and the race is rare in QEMU.
- **`lostwake` reports STUCK on a correct kernel.** The watchdog limit is 100 ticks (about 10 s). Under a very slow emulator, raise it: `lostwake 2000 300`.
- **The expect script times out at boot.** The kernel panicked before the shell started, as in exercise C; the log shows the panic. The script quits QEMU on a timeout, so nothing keeps running.
- **Linux: ThreadSanitizer says `unexpected memory mapping`.** See exercise G: lower `vm.mmap_rnd_bits` to 28 on a machine you control, or run it with `setarch -R` outside a restrictive container.
- **Linux: `strace` fails with `Operation not permitted` in a container.** Add `--cap-add SYS_PTRACE`.
- **Windows: `undefined reference to WaitOnAddress`.** Link with `-lsynchronization`.
- **Timings jump between runs.** Other programs, the power plan and, for the container, the host all show up in these numbers. Report the spread over several runs, as the tables here do.

## Where this shows up later

- Lab 06 measures how long a woken thread waits for a CPU, the step after the wakeup this lab protects.
- Stage 08 passes file descriptors and shared memory between processes; a futex in shared memory, or a Windows named object, is how those processes would synchronize.
- Stage 14's fences answer the same question as `sleep`/`wakeup` for the GPU: how a consumer waits for a producer without missing the signal.

## Next

[Lab 06](../06-scheduling-latency/README.md) replaces xv6's round-robin scheduler and measures how late a 60 Hz wakeup runs on Linux and Windows under load.
