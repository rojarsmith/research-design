# Lab 02: Processes, threads and context switches

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 02](../../SYLLABUS.md#02-processes-threads-and-context-switches)

> **Draft for review, 2026-11-15, xv6 and Linux parts run on 2026-11-16. What was and was not executed:**
> - **xv6 parts (exercises A–D): run on 2026-11-16** in an Ubuntu 24.04 container under podman on Windows 11, whose podman machine runs on WSL 2. That is not the WSL 2 Ubuntu distribution the course tells you to use. The container had QEMU 8.2.2, riscv64-linux-gnu-gcc 13.3.0 and gdb-multiarch 15.1, and [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) master at commit `fa2ddcb` (2026-11-05). An expect script typed into the xv6 console and GDB for exercise B, and a batch GDB script drove exercise C. Exercise D was checked with a port of `uthread_win64.c` written only for that check; it is not in this folder. Output: [`xv6/example-output.txt`](xv6/example-output.txt).
> - **Linux part (exercise E): run on 2026-11-16** in the same container: the podman machine's kernel 6.6.87.2-microsoft-standard-WSL2 with 16 online CPUs, glibc 2.39, gcc 13.3.0, and strace 6.8, which worked without extra container capabilities. Treat its timings as VM numbers. Output: [`linux/example-output.txt`](linux/example-output.txt).
> - **Windows parts (exercises F and G): built and run** on the machine this lab was written on, on 2026-11-15, with gcc 16.2 from WinLibs. The output is in [`windows/example-output.txt`](windows/example-output.txt), and the Windows numbers below come from it.
> - **Not run:** MIT's `xv6-labs-2025` repository and its `util` assignments, and the lock questions at the end of exercise C, which are answered by reading the code.

## Goal

There are three ways to get a second flow of control: a new process, a new kernel thread, and a user-level thread that your own code switches to. Build each one, measure what each costs, and be able to say exactly which registers a context switch saves, where it saves them, and why it can skip the rest. The xv6 kernel's `swtch`, your own user-level switch for xv6, and the Windows x64 switch in this folder all answer that question for a different ABI.

```
process A's kernel thread            this CPU's scheduler thread            process B's kernel thread
yield(), sleep() or kexit()
  sched()
    swtch(&A->context, &c->context) ─► returns inside scheduler()
                                       release A's lock, pick B
                                       swtch(&c->context, &B->context) ─► returns inside sched(),
                                                                          or in forkret() the first time
```

## Files

| File | What it is | Status |
| --- | --- | --- |
| [`xv6/pipeline.c`](xv6/pipeline.c) | `cat README \| grep the \| wc` built from `fork`, `exec`, `pipe`, `dup` and `wait`, with a `-leak` switch | run in xv6 (container) |
| [`xv6/example-output.txt`](xv6/example-output.txt) | recorded xv6 output: the pipeline, the `fork()` and `swtch()` traces, and the user-level thread checks | recorded 2026-11-16 |
| [`linux/pipeline.c`](linux/pipeline.c) | the same pipeline on Linux, for `strace -f` | built and run on Linux (container) |
| [`linux/procthread.c`](linux/procthread.c) | Linux counterpart of `nt_procthread.c`: `fork`, `fork` + `exec`, `posix_spawn`, `pthread_create`, a semaphore ping-pong, `swapcontext` | built and run on Linux (container) |
| [`linux/example-output.txt`](linux/example-output.txt) | recorded output of both Linux programs, with the `strace` runs | recorded 2026-11-16 |
| [`windows/nt_procthread.c`](windows/nt_procthread.c) | process vs thread identity, `CreateProcess` vs `CreateThread` cost, an event ping-pong between two threads, `SwitchToFiber` | built and run here |
| [`windows/uthread_win64.c`](windows/uthread_win64.c), [`windows/uswitch_win64.S`](windows/uswitch_win64.S) | user-level threads with a hand-written x64 context switch: a thread table, the switch in assembly, a yield | built and run here |
| [`windows/example-output.txt`](windows/example-output.txt) | recorded output, including the run with `-DSKIP_XMM` and the disassembly of `worker` | recorded 2026-11-15 |

## Setup

- The Lab 00 workbench. Use `CPUS=1` for exercises B and C, so that the breakpoints fire on one hart.
- MIT's matching lab is `util` (`git checkout util` in `xv6-labs-2025`). Its 2025 assignments are `sleep` (built on the `pause()` system call), `sixfive`, `memdump`, `find`, and `find -exec`, which uses `fork`, `exec` and `wait` ([util lab](https://pdos.csail.mit.edu/6.1810/2025/labs/util.html)). Do them as warm-ups; this lab assumes you can write a small xv6 user program.
- **Linux:** WSL 2 Ubuntu with `strace`, as in Lab 01.
- **Windows:** any x86-64 MinGW-w64 gcc. `objdump` from the same toolchain is used in exercise G.

## Read first

- The xv6 book: chapter 1 (processes, file descriptors, pipes) and the scheduling chapter (chapter 7).
- `user/sh.c`: `runcmd()`, especially the `PIPE` case, and `parsepipe()`.
- `kernel/proc.c`: `allocproc`, `kfork`, `scheduler`, `sched`, `yield`, `forkret`, `kexit`, `kwait`.
- `kernel/swtch.S` and `struct context` in `kernel/proc.h`.
- The integer register table of the [RISC-V calling convention](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc): `s0`–`s11` and `sp` are preserved across calls, while `ra`, `t0`–`t6` and `a0`–`a7` are not. This is the calling convention from `c-compiler-foundations`, now used by the kernel.

## Exercises

Write each prediction in `NOTES.md` before you run anything.

### A. A pipeline from four system calls (xv6)

Copy [`xv6/pipeline.c`](xv6/pipeline.c) to `user/`, add `$U/_pipeline` to `UPROGS`, and boot.

**Predict:** Will `pipeline` print the same count as `cat README | grep the | wc` typed into the xv6 shell? In which order will the three `pid N exited` lines appear? What will `pipeline -leak` do, and which process will be stuck, waiting for what?

Run both, then answer from the code:

- Each child closes all four pipe ends after `dup`, and so does the parent. Find the line in `sh.c` that does the same.
- `sh.c` builds `a | b | c` differently: `parsepipe()` is right-recursive, so the shell forks a child for `a` and a child that is itself a pipeline `b | c`. Draw the process tree each version creates.
- With `-leak`, press Ctrl-p to see every process and its state. xv6's shell has no job control, so quit QEMU with Ctrl-a, then x.

<details>
<summary>Observed on 2026-11-16. Read this only after you have predicted and run it.</summary>

- `pipeline` printed `3 33 197`, the same as `cat README | grep the | wc` typed into the shell.
- In five runs, with one hart and with three, the `pid N exited` lines came in the order cat, grep, wc. The count line landed anywhere among them. In one run with three harts, wc's count and a `pid N exited` line were mixed character by character (`3 p3i3d  1129 ex7i t`), because xv6's `printf` makes one `write` per character.
- `pipeline -leak` printed `pid 4 exited with status 0` for `cat` and then nothing more. Ctrl-p listed `3 sleep  pipeline`, `5 sleep  grep` and `6 sleep  wc`, besides `init` and `sh`, and the same list again a few seconds later.

</details>

### B. Trace `fork()` (xv6)

```
(gdb) b kfork
(gdb) c                      the first stop is init starting sh, before you type anything
(gdb) c                      now type "pipeline" in the xv6 console: this stop is sh forking for it
(gdb) c                      this stop is pipeline's first fork
(gdb) p cpus[0].proc->name   p itself is not set yet at the breakpoint
(gdb) p cpus[0].proc->pid
(gdb) next                   (repeat; stop after allocproc() returns)
(gdb) p np->pid
(gdb) p/x np->context        ra and sp of the child's kernel thread
(gdb) p/x np->kstack
(gdb) p p->sz / 4096
```

**Predict, before stepping on:**

1. `allocproc()` sets the child's `context.ra` and `context.sp`. Which function will the child's kernel thread run first, and on which stack?
2. How many pages will `uvmcopy()` copy? Master's `uvmcopy` copies every mapped page below `p->sz` and skips unmapped ones, so `p->sz / 4096` is an upper bound. Stage 03's copy-on-write fork brings this number down. Record it now so you can compare.
3. After `*(np->trapframe) = *(p->trapframe)` and `np->trapframe->a0 = 0`, where will the child resume in user space, and what will `fork()` return there?

Check prediction 1 with `disable 1`, `b forkret`, `c`, and `p cpus[0].proc->pid` (with one hart, `cpus[0]` is the only CPU). Without the `disable`, the next stop is the parent's second `fork`, because the parent keeps running until it blocks. Check prediction 3 in `user/pipeline.asm`: find the `ecall` in the `fork` stub and the instruction after it.

<details>
<summary>Observed. Read this only after you have predicted and run it.</summary>

- The three stops were `init` (pid 1), `sh` (pid 2) and `pipeline` (pid 3). At the breakpoint `p p->name` printed `value has been optimized out`.
- After three `next`s (line 271, the `uvmcopy` call), `np->pid` was 4, and `np->context` had `ra = 0x80001912`, which `info symbol` named `forkret`, and `sp = 0x3fffff8000`, which is `np->kstack` (`0x3fffff7000`) plus 4096. All the `s` registers were 0.
- `p->sz / 4096` was 4.
- With breakpoint 1 still enabled, `c` stopped in `kfork` again, for pid 3. After `disable 1`, it stopped in `forkret` with `cpus[0].proc->pid` 4, the name still `pipeline` because `exec` had not run yet, and `$sp` `0x3fffff8000`.
- In `user/pipeline.asm` the `fork` stub's `ecall` is at `0x49e` and the `ret` after it at `0x4a2`.

</details>

### C. Trace `swtch()` (xv6)

```
(gdb) b swtch
(gdb) c
(gdb) bt
(gdb) p/x $a0
(gdb) p/x $a1
(gdb) p &cpus[0].context
(gdb) p/x $sp
```

Compare `$a0` and `$a1` with `&cpus[0].context` to see which way this switch goes: from a process to the scheduler, or from the scheduler to a process. **Predict** where `swtch`'s final `ret` will land and on which stack, then step to it:

```
(gdb) si                     (repeat until the ret: 28 times from the first instruction)
(gdb) p/x $ra
(gdb) info symbol $ra
(gdb) si
(gdb) bt
(gdb) p/x $sp
```

Catch both directions and record `info symbol $ra` for each. Expect `scheduler`, `sched`, or `forkret` for a process that has never run. Then answer: which stack does `scheduler()` run on? Hint: `main()` calls `scheduler()` and never returns, and `main()` runs on the stack that `entry.S` set up.

Also find which lock is held across the switch. `yield()` acquires `p->lock` before `sched()`, and something releases it on the other side. Find the `release` in `scheduler()` and the one in `forkret()`.

<details>
<summary>Observed in the first four stops after boot. Read this only after you have predicted and run it.</summary>

- Stop 1, scheduler to process: `$a0` was `0x80012350`, which is `&cpus[0].context`, and `$a1` was `0x800127a8`, the first process's context. `bt` showed `swtch` called from `scheduler`, called from `main`. 28 `si` reached the `ret`. `$ra` was `forkret`, and `$sp` changed from `0x8000b1a0` to `0x3fffffe000`, the kernel stack of pid 1.
- Stop 2, process to scheduler: pid 1 was sleeping in `virtio_disk_rw()`, inside `fsinit()` called from `forkret()`. `$a0` and `$a1` were the other way round. The `ret` went to `scheduler+130`, and `$sp` became `0x8000b1a0` again.
- Stop 3, scheduler to process again: this time the `ret` went to `sched+112`, back into the `sleep()` that stop 2 left. Stop 4 was like stop 2.
- `scheduler()`'s `$sp`, `0x8000b1a0`, lies in the first 4096 bytes of `stack0` (`0x8000a220` to `0x8000b220`): the stack that `entry.S` set up for hart 0.
- `bt` right after the switch into `forkret` showed a made-up caller, `myproc () at kernel/proc.c:89`. Nothing called `forkret`; `swtch`'s `ret` jumped there.

</details>

### D. User-level threads for xv6

Port [`windows/uthread_win64.c`](windows/uthread_win64.c) to xv6 as `user/uthread.c`, and write the switch for RISC-V as `user/uthread_switch.S`:

- **The switch:** `thread_switch(struct context *old, struct context *new)` saves and restores the same 14 registers as `kernel/swtch.S`: `ra`, `sp`, `s0`–`s11`. Use your own `struct context` in `uthread.c`; do not include the kernel's.
- **A new thread's context:** set `ra` to `thread_entry` and `sp` to the top of the thread's stack. The stack can be a static `char` array declared with `__attribute__((aligned(16)))`, so that `sp` is 16-byte aligned as the RISC-V psABI requires. On RISC-V, `ret` jumps to `ra`, so nothing has to be planted on the stack. On x64, `uswitch_win64.S` has to put `thread_entry`'s address on the new stack for `ret` to pop. Compare `thread_create()` in the Windows file.
- **Remove the Windows parts:** the Windows and C library headers (use `kernel/types.h`, `kernel/stat.h` and `user/user.h`), `VirtualAlloc`, `VirtualProtect`, the guard page, `QueryPerformanceCounter` and `setvbuf`. Replace `int64_t` with `uint64` or `long`. xv6's `printf` exists, and `exit(0)` ends `main`.
- **Start with integer sums only.** xv6's `printf` has no `%f` in any case. Later, as an experiment, add the `double` sum back and compare it as an integer. If xv6 kills the program, record `scause` and explain it. Hint: read about the `FS` field of `sstatus` in the RISC-V privileged spec, then grep xv6 for it.
- **The Makefile:** user programs are linked from one object file by the `_%` rule. Add a rule for `$U/_uthread` that copies that rule's recipe and links `$U/uthread_switch.o` as well, plus a rule that builds `$U/uthread_switch.o` from the `.S` with `$(CC) $(CFLAGS) -c`. Recipe lines must start with a tab. Add `$U/_uthread` to `UPROGS`, and rebuild with `make qemu` or `make fs.img`; plain `make` builds only the kernel.

**Predict, then run:** the output of the first part should interleave as `thread_a round 0`, `thread_b round 0`, `thread_c round 0`, `thread_a round 1` and so on, as it did on Windows (exercise F). **Break it:** save only `ra` and `sp` in `thread_switch`. Predict what breaks and why: xv6 compiles user code with `-O`, and the loop variables of `worker` live somewhere. Read `user/uthread.asm` to find out where, then run it.

<details>
<summary>Observed with a reference port (not in this folder). Read this only after you have predicted and run your own.</summary>

- The first part interleaved exactly as predicted, with 16 switches, and every integer sum was right. Two threads of 1,000,000 rounds each also gave the right sums.
- In that port's `user/uthread.asm`, `worker` kept `i` in `s1`, `isum` in `s3`, `w` in `s2` and the address of `current` in `s4`.
- With only `ra` and `sp` saved, no sum was printed at all. After `thread_c round 0`, xv6 printed `usertrap(): unexpected scause 0xf pid=3` with `sepc=0x252 stval=0x800e022e40611b1` and killed the program. `0x252` was in `main`'s own loop, the inlined `current->state = RUNNABLE`. `main` expected `s3` to hold `&current`, but it resumed with whatever a worker had left in `s3`, and the store went to a garbage address.
- With the `double` sums added back, xv6 killed the program before the first line of output: `scause 0x2` (illegal instruction), `sepc=0x396`, `stval=0xa422`. `0xa422` is the encoding of `fsd fs0,8(sp)` in `worker`'s prologue. xv6 never sets the `FS` field of `sstatus`; grep finds no use of it.

</details>

### E. The same questions on Linux (WSL 2)

```sh
cd linux
gcc -O2 -Wall -o pipeline pipeline.c
gcc -O2 -Wall -pthread -o procthread procthread.c
./pipeline
cat pipeline.c | grep pipe | wc -l
strace -f -e trace=clone,clone3,execve,pipe,pipe2,dup2,wait4 -o pipeline.trace ./pipeline
./procthread
strace -f -e trace=clone,clone3,execve,wait4 ./procthread identity
strace -c -f ./procthread switch > /dev/null
```

`pipeline` reads `pipeline.c` from the current directory, so run it inside `linux/`.

**Predict, then check:**

1. Which system call appears in the trace where the C code calls `fork()`? fork(2) says glibc's wrapper has invoked clone(2) since glibc 2.3.3 ([fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html)).
2. Which clone flags does `pthread_create` pass that `fork` does not? Look for `CLONE_VM`, `CLONE_FILES`, `CLONE_SIGHAND` and `CLONE_THREAD` ([clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html)). Recent glibc may call `clone3`, which strace prints with a structure argument.
3. Is `tid` equal to `pid` for the main thread? For the created thread? For the child process?
4. Does `swapcontext` enter the kernel? Count with `strace -c`. Hint: a `ucontext_t` includes the signal mask ([getcontext(3)](https://man7.org/linux/man-pages/man3/getcontext.3.html)). Compare its cost with `SwitchToFiber` and `uswitch` below.
5. Compare your numbers with the Linux column of the table in exercise F, which was recorded in the container described at the top. WSL 2 runs in a VM too, so treat both as relative.

<details>
<summary>Observed in the container. Read this only after you have predicted and run it.</summary>

- `./pipeline` printed 12, the same as `cat pipeline.c | grep pipe | wc -l`. `./pipeline --leak` hung after `cat` exited and was stopped by `timeout`.
1. `fork()` appeared as `clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, ...)`. The trace also shows `execvp` searching `PATH`: `execve("/usr/local/sbin/cat", ...)` failed with `ENOENT`, and so on, until `/usr/bin/cat`.
2. `pthread_create` called `clone3` with `CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID`, `exit_signal=0` and its own stack.
3. The main thread had tid = pid (35), the created thread the same pid and a new tid (36), and the child process a new pid with tid = pid (37).
4. Yes. `strace -c -f ./procthread switch` counted 4,000,010 `rt_sigprocmask` calls, one for each of the 4,000,000 `swapcontext` calls, and 861,958 `futex` calls from the semaphore ping-pong. That run took several minutes, because every traced system call is slow.

</details>

### F. Processes, threads and switches on Windows (run here)

```powershell
cd windows
gcc -O2 -Wall -o nt_procthread.exe nt_procthread.c
.\nt_procthread.exe
gcc -O2 -Wall -o uthread_win64.exe uthread_win64.c uswitch_win64.S
.\uthread_win64.exe
```

**Predict, before running:** order these from cheapest to most expensive, and guess each within a factor of ten: create and wait for a process; create and join a thread; hand control to another kernel thread through an event; switch fibers; switch with `uswitch`.

Observed on 2026-11-15 (Windows build 26300, Intel Core i7-11700 with 16 logical processors, hypervisor present; [full output](windows/example-output.txt)). Two runs were made that day. The recorded run is first, and the earlier run is in brackets.

The Linux column was recorded on 2026-11-16 in the container described at the top of this lab, which saw the same i7-11700 with 16 logical CPUs ([full output](linux/example-output.txt)). The first run of `./procthread` is first, and the second run is in brackets. It is a VM, so compare ratios within the column rather than values across columns.

| What | Windows, observed | Linux, observed in the container |
| --- | ---: | ---: |
| create a process and wait for it (`CreateProcess` of this program with `--child`) | 43,594 µs [20,717 µs] | `fork` + `exec` + `wait`: 554 µs [613 µs] |
| create a thread and wait for it | 85.2 µs [82.8 µs] | `pthread_create` + `join`: 73.9 µs [76.2 µs] |
| hand-off between two kernel threads, same logical CPU | 1,277 ns [1,301 ns] | 1,189 ns [1,322 ns] |
| hand-off between two kernel threads, unpinned | 3,264 ns [3,280 ns] | 18,803 ns [19,316 ns] |
| `SwitchToFiber` | 27.1 ns [28.8 ns] | `swapcontext`: 199.7 ns [218.3 ns] |
| `uswitch` round robin, scheduler loop included | 12.5 ns [12.1 ns] | not measured |

On Linux, `fork` + `_exit` + `wait` alone cost 254.5 µs [201.9 µs] and `posix_spawn` + `wait` 437.0 µs [487.3 µs].

What these runs show, and what they do not:

- **Process creation is expensive and noisy here.** Two runs on the same day differed by a factor of two. Learn says `CreateProcess` creates a new process together with its primary thread, and returns before the new process has finished initializing ([CreateProcessW](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw)). This program also waits for the child to exit. Where the milliseconds go was not measured. Propose candidates (loading the image, DLL initialization, security software, the console) and test one, for example with a WPR trace (stage 06) or Sysinternals Process Monitor.
- **A kernel hand-off costs about 50 times a fiber switch and 100 times `uswitch`** (1,277 ns against 27.1 and 12.5 ns). The kernel path is a wait on one event and a set of another, each a system call, plus the scheduler. The user-level paths are a function call.
- **Unpinned hand-offs cost 2.5 times more than pinned ones.** Explain why a wake-up on another logical CPU could take longer than a switch on the same one, then design a test.
- **`SwitchToFiber` costs twice `uswitch`.** Learn says the only state kept for a fiber is its stack, the registers normally preserved across a function call, and its fiber data, and that fiber local storage is switched along with the fiber ([Fibers](https://learn.microsoft.com/en-us/windows/win32/procthread/fibers)). Read the warning at the top of `uthread_win64.c`: what else must Windows update so that exceptions still work on a fiber's stack?
- **In the Linux VM, an unpinned hand-off cost about 16 times a pinned one** (18,803 against 1,189 ns), far more than the 2.5 times on Windows. `swapcontext` cost about 7 times `SwitchToFiber`, and exercise E question 4 shows the system call that explains part of it. Propose why the unpinned case is so slow in a VM before you compare it with your own numbers.

The identity part shows the split the build plan describes. The main thread and the created thread report the same pid with different tids, and the child process has a new pid and a new tid. In Learn's terms, a process provides the resources needed to run a program, and a thread is the entity within a process that gets scheduled ([processes and threads](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads)).

### G. What a switch must save, and what breaks if it does not (run here)

`uswitch_win64.S` saves `rsp`, `rbx`, `rbp`, `rdi`, `rsi`, `r12`–`r15` and `xmm6`–`xmm15`, the registers that Learn's [x64 calling convention](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention) lists as nonvolatile. Build it again without the XMM registers:

```powershell
gcc -O2 -Wall -DSKIP_XMM -o uthread_noxmm.exe uthread_win64.c uswitch_win64.S
.\uthread_noxmm.exe
objdump -d -M intel --no-show-raw-insn uthread_win64.exe
```

**Predict** which of the two sums will break, then run it. Observed here: every integer sum stayed correct, and every double sum was wrong. For example, thread 0's double sum was 10.25 where 0.75 was expected, and thread 1's was 0.00 where 3.75 was expected. The program exited with status 1. The disassembly of `worker` (gcc 16.2, `-O2`, recorded in the example output) shows why:

```
movaps XMMWORD PTR [rsp+0x20],xmm6     worker's prologue saves xmm6 because it will use it
pxor   xmm6,xmm6                       dsum = 0.0 lives in xmm6
xor    edi,edi                         isum = 0 lives in rdi
call   1400017b0 <thread_schedule>     the yield, inlined down to this call
addsd  xmm6,xmm0                       dsum += ...
add    rdi,rax                         isum += ...
```

Across the call that switches threads, the compiler keeps `dsum` in `xmm6`, the constant 0.25 in `xmm7`, `isum` in `rdi`, `i` in `rbx` and `w` in `rsi`, all nonvolatile registers. Without the XMM part of the switch, every thread resumes with whichever `xmm6` the previous thread left.

Now compare the three switches:

| | xv6 `swtch` (RISC-V) | `uswitch` (Windows x64) |
| --- | --- | --- |
| Saved integer registers | `ra`, `sp`, `s0`–`s11` | `rsp`, `rbx`, `rbp`, `rdi`, `rsi`, `r12`–`r15` |
| Saved FP and vector registers | none | `xmm6`–`xmm15` |
| Where the resume address is | `ra`, saved in `struct context` | on the old stack, found through the saved `rsp` |
| Where the state goes | `p->context` or `cpu->context` | `struct uctx` in the thread table |
| Page table | not touched: all kernel threads share the kernel page table, and the user table changes in `uservec`/`userret` | not touched: one process |

## Linux / NT comparison

- **Linux:** a thread is a task created by `clone` with flags such as `CLONE_VM` and `CLONE_THREAD`, and a process is a task created without them ([clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html)). Exercise E shows the flags in the trace.
- **NT:** a process provides the resources (address space, handles, security context), and a thread is the unit that gets scheduled ([Learn](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads)). Exercise F shows the ids.
- **No `fork` on NT.** `CreateProcess` starts a new program from an executable file. `fork` copies the calling process, and `exec` then replaces its image, so the pair has a cheap step and an expensive one. Compare your `fork` + `exec` number with `fork` alone and with `CreateProcess`.
- **User-level switching:** NT has fibers, and glibc has `ucontext`. Neither one is preemptive. Exercise E question 4 asks whether `swapcontext` stays in user mode.

## Checkpoints

Answer these in `NOTES.md`.

1. List what `swtch` saves and where. The psABI says `ra` is not preserved across calls, yet `swtch` saves it. Why? Why can `swtch` skip `t0`–`t6`, `a0`–`a7` and every floating-point register?
2. Explain why a switch from one process to another in xv6 passes through the scheduler thread. Use what you found in exercise C: which stack the scheduler runs on, which lock is held across `swtch`, and what `kexit()` needs from a stack that is not its own.
3. Explain what your user-level switch does not have to save, and why: caller-saved registers, the page table, the privilege mode, `sepc` and `sstatus`, the trap frame. Then show the evidence from exercise G for what a switch must save.
4. Compare `fork` + `exec` with `CreateProcess`: what each one copies or builds, the `uvmcopy` page count from exercise B, and the costs from exercises E and F.
5. From your `strace` output, give the clone flags of a thread and of a process. Map them onto NT's split between process and thread.
6. Explain why `pipeline -leak` hangs, naming the file descriptor that keeps each reader waiting.

## Common failure modes

- **The pipeline hangs.** A write end of a pipe is still open somewhere: in the parent, or in a child that did not close the ends it does not use. A reader sees end of file only when every write end is closed.
- **`exec cat failed`.** xv6 programs live in `/`. `exec("cat", ...)` is relative to the current directory, so run `pipeline` from `/` or change the paths to `/cat`.
- **Output arrives in a strange order.** The parent and the children all write to the console. The order of the `pid N exited` lines depends on the scheduler.
- **`b swtch` fires on every timer tick.** That is how often xv6 switches. Use `CPUS=1`, and `c` until you catch the direction you want.
- **`p p->name` prints `value has been optimized out` in `kfork`.** At the breakpoint `p` is not assigned yet, and the kernel is built with `-O`. Use `cpus[0].proc` with one hart, or `next` first.
- **`b forkret` does not fire; `kfork` stops again.** The parent keeps running until it blocks, so its next `fork` comes first. `disable` the `kfork` breakpoint.
- **A user thread is killed with `scause 0x2`.** It executed a floating-point instruction, and xv6 leaves the `FS` field of `sstatus` off.
- **A user thread crashes as soon as it starts.** Check that `sp` points at the top of its stack, not the bottom (the stack grows down), that it is 16-byte aligned, and that `ra` holds `thread_entry`.
- **A thread function returns and the program crashes.** It has nowhere to return to. Always enter through `thread_entry`, which marks the thread `FREE` and switches away.
- **`make: *** missing separator`.** A Makefile recipe line starts with spaces instead of a tab.
- **`strace -f` prints too much.** Filter with `-e trace=...` and write to a file with `-o`.
- **`strace -c -f ./procthread switch` takes minutes.** It makes almost five million system calls, and each traced one is slow.
- **Windows: `uthread_win64.exe` dies with no message.** An exception raised on a uthread stack cannot be dispatched (see the warning in the source). Keep the thread bodies to plain C.
- **Windows: the `SKIP_XMM` build does not break for you.** Your compiler kept the sums somewhere else. Check the disassembly of `worker` before you draw conclusions.
- **Windows: timings jump between runs.** Report the spread over several runs, not one number.

## Where this shows up later

- Stage 03 replaces the full copy in `uvmcopy` with copy-on-write and counts the pages again.
- Stage 05 returns to the lock held across `swtch`, and to `sleep`/`wakeup`.
- Stage 06 replaces the round-robin loop in `scheduler()` and measures wake-up latency, the cost you saw in the event ping-pong.
- Stage 08's event loop is the alternative to many threads; stage 09's server must never block on a client.

## Next

Stage 03 of the [build plan](../../SYLLABUS.md#03-virtual-memory) covers virtual memory: page tables, copy-on-write `fork`, and shared and private mappings.
