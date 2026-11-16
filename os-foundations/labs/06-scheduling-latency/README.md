# Lab 06: Scheduling and latency

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 06](../../SYLLABUS.md#06-scheduling-and-latency)

> **Draft for review, 2026-11-16. What was and was not executed:**
> - **xv6 parts (exercises A and B): built and run** on 2026-11-16 in an Ubuntu 24.04 container under podman on Windows 11 (a rootful podman machine on WSL 2), with QEMU 8.2.2 and riscv64-linux-gnu-gcc 13.3.0, on xv6-riscv commit [`fa2ddcb7`](https://github.com/mit-pdos/xv6-riscv/commit/fa2ddcb7ca87a77af579380a6eafd5aff163b44f) (2026-11-05). The priority-scheduler results came from a reference implementation that is not shipped. Output: [`xv6/example-output.txt`](xv6/example-output.txt). The lottery variant (exercise C) was not written or run.
> - **Linux part (exercise D): built and run** in the same container, three times: kernel 6.6.87.2-microsoft-standard-WSL2, 16 vCPUs, gcc 13.3.0. These are numbers from a VM whose vCPUs the Windows host schedules. `SCHED_FIFO` worked with `--cap-add SYS_NICE` and was refused with `EPERM` without it. Output: [`linux/example-output.txt`](linux/example-output.txt).
> - **Windows part (exercise E): built and run** natively, twice, with gcc 16.2 from WinLibs. Output: [`windows/example-output.txt`](windows/example-output.txt).
> - **WPR/WPA recording (exercise F): not executed.** `wpr -start` needs an elevated prompt, which was not used, and Windows Performance Analyzer is not installed on this machine. Only `wpr -profiles`, which needs no elevation, was run, to check the profile names.
> - **Machine:** Windows 11 Pro build 26300, Intel Core i7-11700 (16 logical processors), hypervisor present, Balanced power plan. Other work was running on the machine during every run; each Windows log records the total CPU load just before and after the run (24–48 % around the latency runs). All numbers are specific to this machine and these runs.

## Goal

A compositor wakes up once per frame, composes, and must hand the result to the display before the next vblank. If it wakes up late, the frame misses vblank and the previous one is shown again. This lab measures how late a periodic wakeup really runs, and what decides it: the scheduler's policy, the thread's priority, the timer's resolution, and the machine's other load. You write a priority scheduler for xv6 and watch an interactive process stop waiting behind CPU-bound ones. Then you measure a 60 Hz wakeup on Linux with `timerfd` under the default policy and `SCHED_FIFO`, and on Windows with a high-resolution waitable timer at several thread priorities, and you explain the tail of each histogram.

```
vblank              vblank              vblank              (60 Hz: 16.67 ms apart)
  |------- frame ------|------- frame ------|
       ^ timer due
       |-- wakeup latency --|-- compose --|-- submit --|     on time: shown at the next vblank
       ^ timer due
       |------- wakeup latency (the tail) -------|-- compose --|   late: misses it, one frame dropped
```

## Files

| File | What it is | Status |
| --- | --- | --- |
| [`xv6/lab06-schedstat.patch`](xv6/lab06-schedstat.patch) | per-process wakeup-latency statistics, a priority field, and the `setprio()` and `schedinfo()` system calls; the scheduler stays round-robin | built and run here |
| [`xv6/schedtest.c`](xv6/schedtest.c) | three CPU-bound processes and one interactive process for a fixed time, with a wakeup-latency histogram | built and run here |
| [`xv6/run.sh`](xv6/run.sh) | builds the round-robin kernel, and your priority kernel if you point `SOLUTION_DIR` at it, and runs `schedtest` without a person at the console | run here |
| reference solution (not shipped) | the priority scheduler of exercise B | the recorded `prio` output came from a reference patch kept outside this repository |
| [`linux/latency.c`](linux/latency.c), [`linux/run.sh`](linux/run.sh) | a 60 Hz `timerfd` wakeup, idle and under load, `SCHED_OTHER` and `SCHED_FIFO`, with a text histogram | built and run here |
| [`windows/latency.c`](windows/latency.c) | a 60 Hz waitable-timer wakeup in seven rows: idle with two timer kinds, and under load at five thread settings, with a text histogram | built and run here |
| [`xv6/example-output.txt`](xv6/example-output.txt), [`linux/example-output.txt`](linux/example-output.txt), [`windows/example-output.txt`](windows/example-output.txt) | recorded output | recorded 2026-11-16 |

## Setup

- **xv6.** A clean [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) checkout at commit `fa2ddcb7ca87a77af579380a6eafd5aff163b44f`, as in Lab 05. Apply `lab06-schedstat.patch` to a fresh tree, not on top of the Lab 05 patches: both use system call number 23. Copy `schedtest.c` to `user/`. `xv6/run.sh` does this in a temporary directory and uses Lab 05's `xv6run.exp`:

  ```sh
  sh xv6/run.sh ~/xv6-riscv rr                                   # the round-robin kernel
  SOLUTION_DIR=~/lab06-mine sh xv6/run.sh ~/xv6-riscv rr prio    # and your scheduler
  ```

  For the `prio` variant, apply `lab06-schedstat.patch` to a clean tree and commit it, write your scheduler, and save it with `git diff > ~/lab06-mine/priority-sched.patch`.
- **Linux:** gcc, with `sh linux/run.sh` to build and run every row. For `SCHED_FIFO` you need `CAP_SYS_NICE` or a non-zero `RLIMIT_RTPRIO` ([sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html)): `sudo`, `ulimit -r`, or in a container `--cap-add SYS_NICE`. The runs here used `podman run --cap-add SYS_NICE` with the image described in Lab 05.
- **Windows:** any x86-64 MinGW-w64 gcc. For exercise F, an elevated prompt for `wpr.exe` (it ships with Windows; `wpr -profiles` lists `CPU` and `GeneralProfile`) and Windows Performance Analyzer from the [Windows Performance Toolkit](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/).

## Read first

- The xv6 book, chapter 7 (scheduling). `scheduler()`, `sched()`, `yield()` and `wakeup()` in `kernel/proc.c`; `clockintr()` and the two `yield()` calls in `kernel/trap.c`. The timer interrupt comes every 1,000,000 cycles of the 10 MHz `time` counter, about 0.1 s. Only hart 0's interrupt advances `ticks`.
- [sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html): `SCHED_OTHER` and `SCHED_FIFO`, the privilege rules, and real-time throttling (`sched_rt_runtime_us`). Linux began moving its fair scheduler from CFS to [EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html) in 6.6. [timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html).
- Learn: [scheduling priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities) (the base-priority table), [priority boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts), [CreateWaitableTimerExW](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw) (`CREATE_WAITABLE_TIMER_HIGH_RESOLUTION`, Windows 10 1803 and later).

## Exercises

Write each prediction in `NOTES.md` before you run anything.

### A. Measure the round-robin scheduler (xv6)

`lab06-schedstat.patch` leaves `scheduler()` round-robin and adds measurement:

- `wakeup()` stamps `p->readytime = r_time()` when it moves a process from `SLEEPING` to `RUNNABLE`.
- Just before `scheduler()` switches to a process, `lat_account()` adds `r_time() - p->readytime` to that process's statistics: count, sum, maximum and an eight-bucket histogram from below 10 µs to above 200 ms. `r_time()` reads the `time` CSR, which QEMU's `virt` machine runs at 10 MHz.
- `setprio(n)` stores a priority from 0 to 9 (default 1) that this scheduler ignores, and `schedinfo()` copies the statistics out.

`schedtest 3 50 same` runs three CPU-bound processes and one interactive one for 50 ticks. The interactive process loops on `pause(1)`, a sleep until the next tick, which stands in for a frame-paced wakeup. `inter` asks for priority 5 for the interactive process, and `starve` additionally asks for 9 for the first CPU-bound process.

**Predict,** for `CPUS=1`: how many of its roughly 50 wakeups does the interactive process get to run, and how long does each wait for the CPU? Hint: work out which process the scheduler's loop reaches first after a timer tick.

Observed (`sh xv6/run.sh ~/xv6-riscv rr`, two runs; the [recorded output](xv6/example-output.txt) is the second):

| `CPUS`, mode | Interactive rounds in 50 ticks | Mean wait for the CPU | Max | CPU-bound work units, second run |
| --- | --- | --- | --- | --- |
| 1, `same` | 17 / 17 | 194.9 / 195.1 ms | 200.8 / 201.2 ms | 6,864 / 7,005 / 6,658 |
| 1, `inter` | 17 / 17 | 194.9 / 195.1 ms | 200.9 / 201.3 ms | 7,025 / 6,931 / 6,518 |
| 1, `starve` | 17 / 17 | 194.8 / 195.1 ms | 200.9 / 201.2 ms | 7,159 / 7,300 / 6,864 |
| 3, `same` | 50 / 50 | 105 / 120 µs | 497 / 643 µs | 20,132 / 19,336 / 19,888 |
| 3, `inter` | 49 / 49 | 115 / 145 µs | 305 / 497 µs | 19,232 / 18,599 / 18,802 |

On one CPU, 16 of the 17 wakeups waited more than 200 ms, in both runs: about two full ticks behind the other CPU-bound processes. Explain the 17, and why the priorities changed nothing. With three CPUs the interactive process ran within a millisecond, because some hart's timer tick came soon and found it runnable. **Explain** why that would stop being true with six CPU-bound processes.

### B. A priority scheduler (xv6)

Replace the loop in `scheduler()`:

- Run the `RUNNABLE` process with the highest `p->prio`. Among processes at that level, take turns.
- Keep each `p->lock` held only while you look at that process, as the original does, and re-check `RUNNABLE` under the lock before you run it: another CPU may have taken it.
- Keep the `intr_on(); intr_off();` at the top of the loop and the `wfi` when nothing is runnable.
- Call `lat_account(p)` just before `swtch`, so the statistics keep working.

Write it yourself; no solution ships with this lab. The `prio` rows below came from a reference implementation that is not shipped: two passes over `proc[]`, the first to find the highest runnable priority and the second to run the next process at that level after the one this CPU ran last.

Observed, `CPUS=1`, two runs (`SOLUTION_DIR=... sh xv6/run.sh ~/xv6-riscv prio`):

| Mode | Interactive rounds | Mean wait | Max | CPU-bound work units, second run |
| --- | --- | --- | --- | --- |
| `same` | 17 / 17 | 194.9 / 195.2 ms | 200.9 / 202.2 ms | 6,977 / 7,367 / 6,727 |
| `inter` (interactive at 5) | 50 / 50 | 209 / 246 µs | 382 / 628 µs | 7,052 / 7,310 / 7,057 |
| `starve` (one CPU-bound at 9) | 1 / 1 | 4,914 / 4,920 ms | the same | 18,696 / 0 / 0 |

- **Latency.** At priority 5 the interactive process ran at every tick, about 0.2 ms after its wakeup, against about 195 ms under round robin. The CPU-bound processes lost almost nothing, because the interactive one needs very little CPU.
- **Starvation.** A CPU-bound process at priority 9 never sleeps, so nothing below it ran for the whole test. The interactive process waited 4.9 s for its first and only round, in both runs. Add aging (raise a process's priority the longer it waits, or lower it as it uses CPU) and run `starve` again.
- **A bug worth finding.** A first version of the reference kept one round-robin cursor for all levels. Under `inter`, the interactive process's run moved the cursor, so the search at priority 1 always started just after it and found the same CPU-bound process: 20,338 work units for that one and 0 for the other two (recorded in the output file). Explain why one cursor per level fixes it.
- With `CPUS=3`, `inter` gave 50 rounds in both runs (mean wait 182–190 µs). `same` gave 50 and 48 rounds, and in the second run one wakeup waited 101 ms. With equal priorities, three CPUs usually hide the problem, but not always.
- xv6 preempts only at timer ticks. A wakeup from a pipe write does not preempt the CPU-bound process; the woken process waits for the next tick even at a higher priority. Linux and NT preempt at the wakeup (exercises D and E). Where would you add that to xv6?

### C. Optional: a lottery scheduler (xv6, not done here)

Give each process tickets instead of a priority (`settickets(n)`, default 10). At each scheduling decision, draw a random number below the total tickets of the runnable processes and run the process whose range contains it. xv6 has no random number generator; a small linear congruential generator in `proc.c` is enough. Then run two CPU-bound processes with 10 and 30 tickets and check that their work units come out near 1:3, and run `schedtest` to see what lottery scheduling does, and does not do, for the interactive process. Not written or run for this draft.

### D. A 60 Hz wakeup on Linux, `SCHED_OTHER` vs `SCHED_FIFO`

[`linux/latency.c`](linux/latency.c) arms a `timerfd` on `CLOCK_MONOTONIC` with an absolute first expiry and a period of 1/60 s, reads it 300 times (5 s), and measures how far each wakeup came after its due time. `read()` returns the number of expirations since the last read, so a value above one means whole periods were missed. Load threads spin at the default policy, one per online CPU.

```sh
sh linux/run.sh            # idle; 16 load threads; 16 load threads with SCHED_FIFO 50
```

**Predict** the median and the worst wakeup for each row, and how many wakeups come later than 4 ms.

Observed in three runs in the container (16 vCPUs, `SCHED_FIFO` priority 50):

| Row | Median | p99 | Max | Later than 4 ms (of 300) |
| --- | --- | --- | --- | --- |
| `SCHED_OTHER`, idle | 41 / 110 / 98 µs | 84 / 337 / 257 µs | 0.81 / 1.12 / 0.28 ms | 0 / 0 / 0 |
| `SCHED_OTHER`, 16 load threads | 3,026 / 20 / 22 µs | 13.9 / 2.4 / 4.9 ms | 14.7 / 3.8 / 7.4 ms | 127 / 0 / 5 |
| `SCHED_FIFO`, 16 load threads | 22 / 24 / 23 µs | 4.0 / 2.7 / 3.4 ms | 4.4 / 7.6 / 6.2 ms | 3 / 1 / 2 |

The histogram of the first loaded `SCHED_OTHER` run, which had one whole missed period:

```
       < 50 us     86  ##################################################
  50 - 100 us       8  #####
 100 - 200 us       1  #
 200 - 500 us       5  ###
0.5 - 1 ms         13  ########
  1 - 2 ms         20  ############
  2 - 4 ms         40  ########################
  4 - 8 ms         72  ##########################################
8 - 16.7 ms        55  ################################
    > 16.7 ms       0
```

What these runs show, and what they do not:

- **`SCHED_FIFO` moved the bulk to about 20 µs every time and cut the late wakeups to 1–3 per run**, but it did not remove the tail: 4–8 ms in every run. sched(7) says a `SCHED_FIFO` thread "will always immediately preempt" a `SCHED_OTHER` one, so the guest scheduler should not be the cause. A likely one is the host: the VM's vCPUs share the physical processors with the Windows host's own work, and this machine was busy. That was not traced, so it remains a hypothesis. The hardware validation repeats this on a Linux PC.
- **`SCHED_OTHER` under load varied from a 3 ms median to a 20 µs median** between runs a few minutes apart. Report the spread, never one run. EEVDF runs the eligible task with the earliest virtual deadline, and a task whose deadline is earlier can preempt the running one ([EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html)). Why the woken thread got the CPU at once in two runs and waited milliseconds in the first was not determined.
- **The idle rows were slower in the middle than the loaded `SCHED_FIFO` rows** (a 98–110 µs median in two of the three runs). An idle vCPU has halted, and waking it costs more than preempting a busy one. On real hardware, CPU idle states (C-states) do the same.
- **Real-time throttling** was at its default (`sched_rt_runtime_us` 950,000 of 1,000,000), so a runaway `SCHED_FIFO` thread could not have taken more than 95 % of a CPU.
- Without `--cap-add SYS_NICE`, the container's `CapEff` lacked `CAP_SYS_NICE`, `ulimit -r` was 0, and `sched_setscheduler` returned `EPERM`: the program printed `SCHED_FIFO priority 50 refused: Operation not permitted`. podman reported itself as rootful (`podman info` Rootless: false), so in a rootless setup, check again.

### E. A 60 Hz wakeup on Windows at different priorities (run here)

```powershell
cd windows
gcc -O2 -Wall -o latency.exe latency.c
.\latency.exe          # all seven rows, 4 s each, about 30 s in total
.\latency.exe 5 7      # one row (here row 7) for 5 s, for a WPR trace
```

[`windows/latency.c`](windows/latency.c) re-arms a high-resolution waitable timer every period for the time left until the next due time on a fixed 1/60 s grid, waits on it, and records how late it woke. The load is one spinning thread per logical processor at `THREAD_PRIORITY_NORMAL` in the same `NORMAL_PRIORITY_CLASS` process, so every load thread has base priority 8. The measuring thread gets 8 (`NORMAL`), 9 (`ABOVE_NORMAL`), 10 (`HIGHEST`) or 15 (`TIME_CRITICAL`), according to Learn's [base-priority table](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities).

**Predict** each row before you run it. Will one priority level above the load be enough?

Observed, two runs of 240 wakeups per row:

| Row | Median | p99 | Max | Later than 4 ms (of 240) | Whole periods missed |
| --- | --- | --- | --- | --- | --- |
| 1. idle, high-resolution timer | 336 / 328 µs | 0.63 / 0.66 ms | 1.15 / 1.28 ms | 0 / 0 | 0 / 0 |
| 2. idle, ordinary waitable timer | 7.77 / 7.65 ms | 15.1 / 15.2 ms | 15.2 / 15.3 ms | 180 / 178 | 0 / 0 |
| 3. load, `NORMAL` (base 8) | 13.1 / 9.2 ms | 35.3 / 20.9 ms | 36.1 / 22.5 ms | 224 / 209 | 77 / 18 |
| 4. load, `NORMAL`, boosts disabled | 13.0 / 34.2 ms | 35.2 / 91.2 ms | 50.0 / 97.2 ms | 216 / 222 | 75 / 346 |
| 5. load, `ABOVE_NORMAL` (9) | 276 / 303 µs | 0.51 / 0.52 ms | 0.52 / 0.88 ms | 0 / 0 | 0 / 0 |
| 6. load, `HIGHEST` (10) | 275 / 271 µs | 0.51 / 0.52 ms | 0.52 / 0.52 ms | 0 / 0 | 0 / 0 |
| 7. load, `TIME_CRITICAL` (15) | 262 / 288 µs | 0.52 / 0.52 ms | 0.89 / 0.53 ms | 0 / 0 | 0 / 0 |

Row 3 of the first run, as the program printed it:

```
       < 50 us      0
  50 - 100 us       0
 100 - 200 us       1  #
 200 - 500 us       1  #
0.5 - 1 ms          1  #
  1 - 2 ms          1  #
  2 - 4 ms         12  ######
  4 - 8 ms         39  ##################
8 - 16.7 ms       113  ##################################################
    > 16.7 ms      72  ################################
```

What these runs show, and what they do not:

- **One level above the load was enough.** At base priority 9 or higher, every wakeup ran within 0.9 ms, with or without load, because a ready thread of higher priority preempts the running one ([scheduling priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)). At 8, the measuring thread competed with 16 equal threads for 16 logical processors and lost most frames.
- **The boost did not reliably save it.** Learn says a thread whose wait is satisfied gets a priority boost ([priority boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts)). Turning boosts off with `SetThreadPriorityBoost` made no difference in the first run and made things much worse in the second. Why a boosted thread at 8 still waited milliseconds behind threads at 8 was not determined; the trace in exercise F answers it (look at `NewInPri` and `NewPriDecr`).
- **The ordinary waitable timer** spread the wakeups from 0 to about 15.3 ms, even on an idle machine. That fits expirations processed only at a clock tick of about 15.6 ms, a common Windows default; the tick interval itself was not measured here. One wakeup in the second run came 5.6 µs *before* its due time. Explain how that can happen.
- **The high-resolution timer's floor** was a median of about 0.3 ms, idle or not. This run does not show where those 0.3 ms go.

### F. Record one run with WPR and read it in WPA (not executed here)

These steps follow Learn's [WPR command-line options](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/wpr-command-line-options) and [CPU analysis](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/cpu-analysis) pages. They were not run: `wpr -start` needs an elevated prompt, and WPA is not installed on the machine this lab was written on.

1. In an elevated prompt: `wpr -start CPU -filemode`. WPA's CPU Usage (Precise) graph is built from context-switch events; check with `wpr -profiledetails CPU` that the profile records them (not checked here).
2. In a normal prompt: `latency.exe 5 3` (row 3, `NORMAL` under load), then `latency.exe 5 5` (row 5). Note the tid each one prints.
3. Elevated again: `wpr -stop lab06.etl "lab 06: 60 Hz timer under load"`.
4. Open `lab06.etl` in WPA. Add **Computation > CPU Usage (Precise)** and choose the **Timeline by Process, Thread** preset. Find `latency.exe` and the measuring thread by `NewThreadId`.
5. Add the columns `ReadyTime`, `Ready (µs)` (switch-in time minus ready time), `NewInPri`, `NewPriDecr` (the boost), `ReadyingProcess`, `Readying ThreadId` and `OldThreadId`.
6. For a late wakeup in row 3, check: when was the thread readied relative to its due time? With what priority did it switch in? Which thread did it replace, and how long did it wait in the ready state? Then compare with a wakeup from row 5.

Record what you find in `NOTES.md`, then answer the open questions from exercise E: why the boost did not help reliably, and where the high-resolution timer's 0.3 ms go.

## Linux / NT comparison

- **Priorities.** NT schedules threads by priority, 0 to 31. Threads at the same level take round-robin turns, and a higher level preempts a lower one ([scheduling priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)). Threads with a base priority of 0–15 get dynamic boosts, including when a wait completes ([priority boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts)). Linux keeps fair scheduling (`SCHED_OTHER`, moving from CFS to EEVDF since 6.6) apart from the real-time policies, and a `SCHED_FIFO` thread always preempts `SCHED_OTHER` ones ([sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html)).
- **Privilege.** On Linux, `SCHED_FIFO` needs `CAP_SYS_NICE` or `RLIMIT_RTPRIO` (exercise D). On Windows, any process may raise its own threads within its class, as exercise E did without elevation. Learn warns against `REALTIME_PRIORITY_CLASS`, which interrupts the system threads that handle input and disk flushing.
- **Throttling.** Linux reserves 5 % of each second for non-real-time work by default (`sched_rt_runtime_us`). The two Learn pages cited here describe no such reserve for NT.
- **Other policies.** [`SCHED_DEADLINE`](https://docs.kernel.org/scheduler/sched-deadline.html) is EDF with a constant bandwidth server: a task asks for a runtime per period, which is how a compositor's per-frame work could be expressed. [`sched_ext`](https://docs.kernel.org/scheduler/sched-ext.html) lets BPF programs define a scheduler and has no stable ABI.
- **xv6** has one policy, one priority, and preemption only at its 100 ms tick. Its tail is the tick (exercise A).

## Checkpoints

Answer these in `NOTES.md`.

1. Produce a latency histogram from your own runs, at least for xv6 round-robin vs priority, Linux `SCHED_OTHER` vs `SCHED_FIFO` under load, and Windows `NORMAL` vs `ABOVE_NORMAL` under load.
2. Explain the tail of each. Name the cause for each part of it: the timer's resolution (xv6's 100 ms tick, Windows' ordinary timer), waiting behind equal-priority CPU-bound threads, preemption only at ticks (xv6), wake-up from an idle CPU or vCPU, and in a VM the host scheduling the vCPU. Say which causes you showed and which you only suspect.
3. Connect it to the display. At 60 Hz a frame lasts 16.7 ms. Suppose a compositor wakes 4 ms before vblank and needs 2 ms to compose and submit, so it has 2 ms of slack. From your histograms, how many frames per second would it drop in each configuration? Here, exercise E's row 3 had 231–236 wakeups later than 2 ms out of 240, so nearly every frame; row 5 had none. Exercise D's loaded `SCHED_OTHER` rows had 5–167 out of 300, the `SCHED_FIFO` rows 4–9. A late compositor misses vblank and the previous frame stays on screen, so scheduling is part of display quality.
4. Your xv6 priority scheduler can starve. Show it with `schedtest 3 50 starve`, then show your fix.
5. Explain why the WSL 2 or container numbers can only be compared with each other, and what the hardware validation on the Linux PC will measure instead (the stage 13 flip timestamps).

## Common failure modes

- **`patch` reports a conflict in `syscall.h`.** You applied `lab06-schedstat.patch` on top of the Lab 05 patches. Use a fresh tree.
- **`schedtest` prints `(only N of 4 results arrived)`.** A child failed: `schedinfo()` returned -1, or `setprio()` is missing from your kernel.
- **All three `schedtest` modes look the same on your priority kernel.** Your scheduler ignores `p->prio`, or `setprio()` stores it somewhere the scheduler does not read.
- **Your scheduler panics with `sched locks` or hangs at boot.** You returned from `scheduler()`'s loop while holding a `p->lock`, or held two of them at once.
- **One CPU-bound process gets all the work at equal priority.** Your round-robin cursor moves when a process at another level runs (exercise B).
- **Linux: `SCHED_FIFO priority 50 refused: Operation not permitted`.** Add `CAP_SYS_NICE` (`sudo`, `--cap-add SYS_NICE`) or raise `ulimit -r`.
- **Linux: the machine stops responding during the `SCHED_FIFO` run.** A real-time thread that never sleeps can starve everything else on its CPU apart from the throttling reserve. This program sleeps between wakeups; your own variants should too.
- **Windows: row 1 looks like row 2.** `CreateWaitableTimerExW` ignored the high-resolution flag (older than Windows 10 1803), or the flag was not passed.
- **Windows: `wpr -start` fails with an access error.** Run it from an elevated prompt.
- **Numbers jump between runs.** Other programs, the power plan, thermal limits and, in a VM, the host all show up in these numbers. Report the spread over several runs, as the tables here do.

## Where this shows up later

- Stage 08's event loop sleeps in `epoll_wait` until a `timerfd` fires: the same wakeup measured here.
- Stage 09's server ticks a simulated 60 Hz vblank with `timerfd` and must keep its cadence while clients misbehave.
- Stage 13 replaces the timer with real page-flip events, and the hardware validation measures flip timing with the scheduling choices from this lab.

## Next

Stage 07 of the [build plan](../../SYLLABUS.md#07-file-systems) extends the xv6 file system and adds file-backed `mmap`.
