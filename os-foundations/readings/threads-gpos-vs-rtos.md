**English** | [繁體中文](threads-gpos-vs-rtos.zh-TW.md) | [Course home](../README.md)

# How threads work, and how RTOS threads differ from general-purpose OS threads

> **Researched 2026-11-20.** Technical statements were checked that day against the official pages listed under [Sources](#sources). Nothing new was run for this reading. Every measured number comes from this course's [lab 02](../labs/02-processes-threads/README.md) and [lab 06](../labs/06-scheduling-latency/README.md), or from the simulated kernel in `mcu-foundations` [lesson 7](../../mcu-foundations/lessons/06-rtos-tasks-and-queues.md) (Traditional Chinese only). Each lab's notes set the conditions and limits: the Linux numbers come from a virtual machine, and the simulated kernel's costs are assumptions. Paragraphs marked "inference" are judgements drawn from the sources, not facts that were looked up.

This reading answers two questions. How is a thread built at the machine level? And how do threads in an RTOS (FreeRTOS, Zephyr) differ from threads in a general-purpose OS (Linux, Windows)? It ties together lab 02 (context switching), lab 05 (synchronization), lab 06 (scheduling) and the RTOS lesson of the MCU course. To see each mechanism for yourself, do those labs; [section 8](#8-where-to-see-it-yourself) maps them.

## Summary

1. **A thread is a set of registers, a stack, and a record in the kernel.** The program counter (PC) says where it is executing. The stack pointer (SP) points at the thread's own stack, which holds its call chain and local variables. The kernel keeps a control block with the thread's state and priority: a task in Linux, a thread object in Windows, a TCB in FreeRTOS, a thread control block in Zephyr. Code, globals and the heap are shared with the other threads.
2. **Switching threads means saving registers, swapping stacks and returning.** Save the current registers for the old thread, point SP at the new thread's stack, reload the registers the new thread saved last time, and return. The CPU then continues wherever the new thread left off. Every OS does this; they differ only in what else has to be saved.
3. **RTOS threads and general-purpose OS threads share the mechanism but not the goal.** A general-purpose OS aims for throughput, fairness and isolation. An RTOS aims for a worst case you can predict. Every difference below follows from that:
   - **Scheduling:** an RTOS uses fixed-priority preemption. A higher-priority thread preempts a lower one as soon as it becomes ready (Zephyr's cooperative threads are the exception), and priorities change only when you change them or priority inheritance does. A general-purpose OS's normal scheduling class decides dynamically who runs first.
   - **Isolation:** a general-purpose OS thread belongs to a process, and the MMU isolates processes from each other. An RTOS usually has one address space for the whole firmware, and threads can reach all of memory by default.
   - **Resources:** a general-purpose OS creates thousands of threads at run time and backs their stacks with physical memory only when touched. An RTOS has few threads, often created and statically allocated at boot, each with a fixed-size block of RAM as its stack.
   - **Boundaries:** a user thread in a general-purpose OS must trap into the kernel. An RTOS thread usually runs privileged and calls the kernel as an ordinary function; the strict boundary is between interrupt service routines (ISRs) and threads.
4. **An RTOS does not make code faster.** It puts a computable bound on how late the highest-priority work can start. A general-purpose OS often does better on average but does not bound the worst case by default, as the lab 06 measurements show.

## 1. What a thread is made of

```
General-purpose OS: one process            RTOS: the whole firmware
  shared: code, globals, heap,               shared: code, globals, heap,
          file descriptors, signal handlers          peripheral registers, kernel objects
  thread A: registers, user stack, kernel stack     task A: registers, stack, TCB
  thread B: registers, user stack, kernel stack     task B: registers, stack, TCB
  other processes live in other address spaces      no other processes; ISRs share one interrupt stack
```

POSIX threads divide attributes into per-thread and process-wide ([pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html)):

| Per thread | Shared by all threads in a process |
| --- | --- |
| Registers, including PC and SP | Code, globals, heap |
| Stack: local variables and return addresses | Process ID, parent process ID, process group and session |
| Thread ID | Open file descriptors, record locks |
| Signal mask, `errno`, alternate signal stack | Signal dispositions |
| Real-time scheduling policy and priority; on Linux also CPU affinity and capabilities | User and group IDs, current directory, umask, timers, resource limits |
| Thread-local storage (TLS) | |

pthreads(7) lists the nice value as process-wide, but the NPTL section of the same page and [sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html) both say that on Linux the nice value is per thread. `errno` can be per thread because of TLS. Windows describes TLS as giving each thread its own data through one global index ([Thread Local Storage](https://learn.microsoft.com/en-us/windows/win32/procthread/thread-local-storage)).

**Why must each thread have its own stack?** The stack records where the thread is in its call chain: each frame's return address, saved registers and local variables. If two threads shared a stack, a call in one would overwrite the other's return address. The other side of this: once each thread has its own stack, several threads can run the same function at once without disturbing each other's locals. Only shared globals and the heap can collide, which is why locks exist (lab 05, and the TX race in MCU lesson 7).

**Processes and threads.** Windows puts it this way: a process provides the resources needed to run a program, and a thread is the entity within a process that is scheduled ([About Processes and Threads](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads)). Inside the Linux kernel both are tasks. A task created by `clone` with flags such as `CLONE_VM`, `CLONE_FILES`, `CLONE_SIGHAND` and `CLONE_THREAD` shares the address space and resources and becomes a thread of the same process ([clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html)); the `strace` in lab 02 exercise E shows those flags. Linux's NPTL is a 1:1 implementation: each thread maps to a kernel scheduling entity (pthreads(7)).

**Threads the kernel cannot see.** Windows fibers, glibc's `ucontext` and lab 02's own `uswitch` all use the same save-registers-and-swap-stacks switch, but user code decides when to switch. The kernel sees one thread, so it never preempts them, and if one blocks in a system call, everything on that kernel thread stops.

## 2. How a switch works

### Voluntary switch: one function call

The heart of a context switch is a few lines of assembly. As C pseudocode:

```c
struct tcb {
    void *sp;        /* stack top when switched out; FreeRTOS keeps it in the TCB's first field */
    int   priority;
    int   state;     /* RUNNING, READY, BLOCKED, ... */
};

/* Called like any function, so the compiler already handled caller-saved registers at the call site */
void switch_to(struct tcb *old, struct tcb *new)
{
    /* 1. push the callee-saved registers onto the current stack           */
    /* 2. old->sp = SP;     remember where the old thread stopped          */
    /* 3. SP = new->sp;     switch to the new thread's stack               */
    /* 4. pop the callee-saved registers the new thread pushed earlier     */
    /* 5. return;  the return address is on the new stack, so this returns */
    /*    to the point where the new thread last called switch_to          */
}
```

Three things to notice:

- **Saving only the callee-saved registers is enough.** To its caller, `switch_to` is an ordinary function. The calling convention already lets it clobber the caller-saved registers, and the compiler dealt with them before the call. That is why xv6's `swtch` on RISC-V saves just 14 registers: `ra`, `sp` and `s0`–`s11`. Lab 02's Windows x64 version saves `rsp`, `rbx`, `rbp`, `rdi`, `rsi`, `r12`–`r15` and `xmm6`–`xmm15`. When exercise G drops the XMM half, every integer sum is right and every double sum is wrong: leave out one class of callee-saved register and things break.
- **A new thread's "first return" is faked.** A new thread has never called `switch_to`, so creating it means forging a state that looks as if it just switched out: the return address is the entry function and SP is the top of the new stack. The first switch to it returns straight into the entry function. In xv6 that is `forkret`; in lab 02 it is `thread_entry`. Exercise B shows the child's `context.ra` is `forkret` in GDB.
- **There is nowhere to return to after the entry function.** pthreads wraps your function so that returning from it is the same as calling `pthread_exit`. FreeRTOS forbids a task from returning from its implementing function in any way; to end, a task deletes itself with `vTaskDelete(NULL)` ([FreeRTOS Kernel Book, chapter 4](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch04.md)).

### Preemption: the interrupt saves the rest first

When a thread does not give up the CPU, for example while it runs a long loop, only an interrupt can take control back. The interrupted point is not a call site, and any register may hold a live value, so saving happens in two parts. Interrupt entry saves the caller-saved registers (or all of them). Once the scheduler decides to switch, the switch routine above saves the rest.

**General-purpose OS**, using xv6 as the example. Linux and NT have the same structure with far more detail:

```
Thread A in user mode
  │  timer interrupt or system call
  ▼  trap: all user registers saved in A's trap frame, CPU enters kernel mode on A's kernel stack
A in the kernel: the scheduler decides to switch
  │  swtch: saves only the kernel code's callee-saved registers
  │  (xv6 first switches to the per-CPU scheduler thread, which then switches to B)
  │  if B belongs to another process, the page table is switched before returning to user mode
  ▼  return on B's kernel stack
B in the kernel: restore all user registers from B's trap frame
  ▼  back to thread B in user mode
```

**RTOS**, using FreeRTOS on a Cortex-M4F:

```
task A running: thread mode, SP is PSP, pointing at A's stack
  │  SysTick or some ISR decides to switch and sets PendSV pending
  ▼  hardware: pushes r0–r3, r12, lr, pc, xPSR (8 words) onto A's stack,
     enters handler mode and switches to MSP
PendSV (lowest priority: runs only after every other interrupt is done)
  │  r0 = PSP
  │  if A used the FPU: also push s16–s31
  │  push r4–r11 and r14 (the exception return value)
  │  store r0 in the first field of A's TCB
  │  call vTaskSwitchContext(): pick B, point pxCurrentTCB at B
  │  load B's stack top from the first field of B's TCB
  │  pop r4–r11 and r14; if B used the FPU, pop s16–s31
  │  PSP = B's stack top
  ▼  exception return: hardware pops the 8 words from B's stack; pc resumes where B was interrupted
task B running
```

The software steps match the instructions of `xPortPendSVHandler` in FreeRTOS's [`portable/GCC/ARM_CM4F/port.c`](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/portable/GCC/ARM_CM4F/port.c). The 8 words the hardware pushes follow the [Cortex-M4 Generic User Guide](https://support.arm.com/documentation/dui0553/latest/) as cited in MCU lesson 7. Zephyr does the same on Cortex-M ([Arm Cortex-M Developer Guide](https://docs.zephyrproject.org/latest/hardware/arch/arm_cortex_m.html)): PendSV sits at the lowest interrupt priority to use Cortex-M tail chaining and to stay out of the way of hardware interrupt latency. Software saves r4–r11 in the thread's structure in kernel memory, plus s16–s31 when the thread has an active floating-point context.

What each side has to handle during a switch:

| Item | General-purpose OS | RTOS (Cortex-M) |
| --- | --- | --- |
| Integer registers | All saved in the trap frame on kernel entry; the switch routine then saves the callee-saved ones | Hardware pushes 8 words; PendSV pushes r4–r11 |
| Floating-point and vector registers | Must be saved; wider vector registers mean more state | s16–s31 saved only for tasks that used the FPU. Zephyr's docs say that with lazy stacking enabled, ordinary interrupts pay no cost for FP context |
| Privilege | Back and forth between user mode and kernel mode | Usually privileged throughout; Zephyr user threads are the exception |
| Address space | Page table switched when moving to another process; not between threads of one process | No page table to switch; with an MPU, the incoming task's region settings are loaded (inference) |
| Stacks | Each thread has a user stack and a kernel stack | Each task has one stack (PSP); ISRs share one interrupt stack (MSP) |
| Picking the next one | Normal-class data structures (EEVDF), multiple CPUs, load balancing | Ready lists ordered by priority |

### Measured costs

What lab 02 measured (Windows on this machine; Linux in a virtual machine, so compare ratios within a column only):

| Operation | Windows | Linux (VM) |
| --- | ---: | ---: |
| Create a thread and wait for it | 85.2 µs | 73.9 µs |
| Create a process and wait for it | 20,717–43,594 µs (`CreateProcess`) | 554 µs (`fork` + `exec` + `wait`) |
| Handoff between two kernel threads, same logical CPU | 1,277 ns | 1,189 ns |
| Handoff between two kernel threads, unpinned | 3,264 ns | 18,803 ns |
| User-level switch | 27.1 ns (`SwitchToFiber`), 12.5 ns (`uswitch`) | 199.7 ns (`swapcontext`, which enters the kernel each time to change the signal mask) |

A kernel handoff costs about 1.2–1.3 µs, and the register switch itself is a small part of that: the same switch done at user level takes 12.5 ns. The rest is two system calls, the scheduler and the wake-up. Unpinned handoffs cost more because another CPU has to be woken.

This project has not yet measured an RTOS context switch on a board. The MCU course's simulated kernel assumes 4 µs per switch, only to compare designs. Real numbers need a GPIO pin and a logic analyzer on the board; see step 3 of lesson 7's on-board work.

## 3. Scheduling: who runs next

```
            selected
   Ready ──────────────► Running
    ▲   ◄──────────────   │
    │   preempted, slice   │  waits for a lock, I/O, sleep, a queue
    │   used up            ▼
    └──── event occurs ── Blocked
```

All three states exist in every system. FreeRTOS divides tasks into Running and Not Running, and splits Not Running into Ready, Blocked and Suspended ([FreeRTOS Kernel Book, chapter 4](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch04.md)). **Blocking is the most important thing a thread can do.** A waiting thread is off the ready list and uses no CPU, while a super loop has to check a flag on every pass. That is exactly why MCU lesson 7 introduces an RTOS.

| Item | Linux | Windows | FreeRTOS | Zephyr |
| --- | --- | --- | --- | --- |
| Priority range and direction | Normal policies all have static priority 0, adjusted with nice; `SCHED_FIFO` and `SCHED_RR` use 1 (low) to 99 (high) | 0 to 31, higher is more urgent; the real-time priority class has base priorities 16–31 | Count set by `configMAX_PRIORITIES`, higher is more urgent, idle is 0 | Lower is more urgent; negative is cooperative, non-negative is preemptible |
| Basic rule | Normal policies use EEVDF for fairness (replaced CFS in 6.6, transition completed in 6.12); a real-time thread always preempts a normal one | Time slices round-robin among ready threads at the highest level | Always runs the highest-priority task that can run | Picks the highest-priority ready thread at each reschedule point; a preemptible thread is preempted by a higher-priority one, a cooperative thread runs until it yields |
| Among equal priorities | `SCHED_RR` has a time quantum; `SCHED_FIFO` runs until it blocks, is preempted by a higher priority, or calls `sched_yield` | Round robin | Round robin by time slice; one time slice is one tick period | Optional time slicing; with `CONFIG_SCHED_DEADLINE`, the earlier deadline wins among equal static priorities |
| Does the system change your priority? | In normal policies the scheduler decides the order; real-time policies, no | Yes: threads with base priority 0–15 are boosted in the foreground, on input, or when a wait completes, and lose one level per time slice; 16–31 are never boosted | No; only mutex priority inheritance raises it temporarily | No; mutexes have priority inheritance |
| Ways not to be preempted | — | — | Turn preemption off, or enter a critical section | Cooperative threads (negative priority); only meta-IRQ threads can preempt them |
| Runaway protection | Real-time throttling: by default real-time threads get at most 0.95 s of each 1 s; 6.12 added SCHED_DEADLINE server infrastructure meant to replace and improve throttling | Learn warns to almost never use `REALTIME_PRIORITY_CLASS`, because it interrupts the system threads that handle mouse and keyboard input and background disk flushing | None: if a high-priority task never blocks, lower-priority tasks never run (inference) | Same (inference) |

Sources: [sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html), [sched-rt-group](https://docs.kernel.org/scheduler/sched-rt-group.html), [EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html), kernelnewbies [Linux 6.6](https://kernelnewbies.org/Linux_6.6) and [Linux 6.12](https://kernelnewbies.org/Linux_6.12), Learn [Scheduling Priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities) and [Priority Boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts), [FreeRTOS Kernel Book, chapter 4](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch04.md), Zephyr [Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html) and [Scheduling](https://docs.zephyrproject.org/latest/kernel/services/scheduling/index.html).

The biggest difference is the fourth row. A general-purpose OS adjusts who runs first on your behalf: Linux's normal policies aim for fairness, and Windows boosts a thread that just finished waiting. An RTOS makes no such adjustment. If you get the priorities wrong, the system runs them wrong.

Two examples measured in lab 06:

- **Windows:** with 16 spinning priority-8 threads filling the CPUs, a 60 Hz timer thread also at priority 8 missed 77 and 18 periods in two runs. One level higher (9), every wake-up ran within 0.9 ms.
- **Linux (VM):** under load, `SCHED_FIFO` kept the median wake-up delay near 22 µs, but every run still had a maximum of 4–8 ms. The lab suspects the VM's host but did not trace it.

In other words, raising priority on a general-purpose OS improves the median, but the worst case is still hard to pin down.

Zephyr also lets you choose the ready-queue implementation. A simple linked list suits systems with very few runnable threads (the docs say about 3). A red/black tree suits about 20 or more. The multi-queue, one list per priority, needs more RAM and cannot be used with deadline scheduling. A general-purpose OS usually does not hand that choice to the application.

## 4. Side-by-side comparison

| Aspect | General-purpose OS (Linux, Windows) | RTOS (FreeRTOS, Zephyr) |
| --- | --- | --- |
| Name | Thread, belonging to a process | FreeRTOS says task, Zephyr says thread; there are no processes |
| Address space and isolation | One address space per process, isolated by the MMU; threads of one process share it | Usually one address space for the whole system. Zephyr threads run in supervisor mode by default, with access to privileged instructions, the whole address space and peripherals. Isolation comes from an MPU: an unprivileged FreeRTOS-MPU task can reach only its own stack and up to three user-defined regions; a Zephyr user thread on an MPU system can reach only its own stack and, by default, no kernel objects other than its own thread object |
| Calling the kernel | System call: trap into kernel mode | A privileged task calls the kernel as a function; a Zephyr user thread makes system calls through the SVC exception |
| When threads are created | Dynamically at run time; lab 02 measured 74–85 µs | Can be dynamic, but often created at boot and statically allocated (`xTaskCreateStatic`, `K_THREAD_DEFINE`), and rarely changed afterwards |
| Stack | The user stack lives in virtual memory: a range is reserved and physical pages arrive on first touch (reserve and commit in lab 03); there is also a kernel stack | One fixed-size block of RAM per task, sized at creation. FreeRTOS counts in words, so 128 on a 32-bit core means 512 bytes |
| Stack overflow | The end of the stack usually has a guard page, so overflow becomes a page fault and the program is terminated; lab 02's `uthread_win64.c` sets up its own guard page too | Without an MPU, overflow silently overwrites neighbouring memory such as another task's TCB or stack, and the symptom may show up far away. Catch it with the RTOS's overflow check, stack high-water marks or an MPU guard region (MCU lesson 7) |
| Count | Hundreds to thousands | A handful to a few dozen |
| Scheduling goal | Throughput, fairness, interactive response; adjusted dynamically | Fixed-priority preemption; worst case can be analysed |
| Interrupts versus threads | The interrupt handler does the minimum and defers the rest, for example to softirqs and workqueues on Linux or DPCs on NT (lab 04) | The ISR only puts an event in a queue and a task does the work. ISRs may only use non-blocking APIs (FreeRTOS `FromISR`, Zephyr `K_NO_WAIT`), and after waking a higher-priority task they request a switch (`portYIELD_FROM_ISR`); otherwise the task waits for the next tick |
| Interrupts the kernel never delays | Not generally offered to applications | FreeRTOS interrupts above `configMAX_SYSCALL_INTERRUPT_PRIORITY` and Zephyr's zero-latency interrupts are never delayed by kernel masking, at the price of not calling any kernel API |
| Synchronization objects | futex, mutex, condition variable (lab 05) | queue, semaphore, mutex. FreeRTOS and Zephyr mutexes have priority inheritance; FreeRTOS binary semaphores do not |
| Time base | Timer tick or tickless. Windows' ordinary waitable timer still lands anywhere from 0 to about 15.3 ms on an idle machine (lab 06) | The tick sets the resolution of time slices and timeouts; in FreeRTOS one time slice is one tick period |

Sources beyond the previous section: Zephyr [User Mode Overview](https://docs.zephyrproject.org/latest/kernel/usermode/overview.html), FreeRTOS [MPU Support](https://www.freertos.org/Security/04-FreeRTOS-MPU-memory-protection-unit) and [xTaskCreateRestricted](https://freertos.org/Documentation/02-Kernel/04-API-references/13-FreeRTOS-MPU-specific/01-xTaskCreateRestricted), Zephyr [Arm Cortex-M Developer Guide](https://docs.zephyrproject.org/latest/hardware/arch/arm_cortex_m.html). For ISRs, mutexes and stack units, see the sources cited in MCU lesson 7.

## 5. Can a general-purpose OS act as an RTOS? (inference)

- Since Linux 6.12, mainline includes real-time preemption (PREEMPT_RT), and sched(7) says it can be enabled without patches since 6.12. With `SCHED_FIFO` or `SCHED_DEADLINE`, Linux can reach very low latency.
- Its worst case is still harder to analyse than an RTOS's: page faults, memory reclaim, drivers, multicore caches and virtualization can all sit on the latency path. Lab 06 still measured 4–8 ms tails with `SCHED_FIFO` inside a VM.
- So a common split is to give hard-deadline control to an RTOS on an MCU, or on a Cortex-M co-processor inside the SoC, and leave networking, files and UI to Linux. For the product-level trade-off, see study 19 of the HMI research, [RTOS vs Linux](../../hmi-studio-iot-ide-spinoff-research/research/19-rtos-vs-linux.zh-TW.md) (Traditional Chinese).

## 6. Common misconceptions

- **"An RTOS is faster."** No. 25 ms of computation is still 25 ms under an RTOS; the RTOS only decides who gets the CPU first. In MCU lesson 7's model, the RTOS version switches 4,830 times per 10 s against the super loop's 1, and what cut latency by an order of magnitude was shortening the critical section.
- **"Highest priority means real time."** On Windows, `REALTIME_PRIORITY_CLASS` starves the system threads that handle input. On Linux, default real-time throttling reserves 5% for normal threads. On an RTOS, priority inversion can make the highest-priority task wait for a whole medium-priority computation (MCU lesson 7, case D).
- **"An RTOS task is a small process."** It is not. Without an MPU there is no isolation between tasks, and one stray pointer can corrupt another task's stack or kernel data.
- **"More threads means faster."** Only when there are CPUs to run them in parallel or waits to overlap. Every thread needs a stack and a control block, and every handoff is a switch. MCU lesson 7 suggests deciding the number of tasks by the events they wait on and their deadlines, not by wrapping every function in a task.
- **"User-level threads are not real threads."** They switch exactly the same way; only the scheduler differs. Fibers, `ucontext` and Go's goroutines are switched by code in user space. The difference is that the kernel cannot see them, so it does not preempt them individually or place them on different CPUs. The runtime has to do that itself; Go's runtime, for example, manages several kernel threads of its own.

## 7. Try these first

1. Two threads run the same function at once. Do their local variables interfere? Their globals?
2. Why does a context switch only need to save the callee-saved registers? What about when a thread is preempted?
3. On Linux, what does a switch between two threads of one process skip, compared with a switch to a thread of another process?
4. On FreeRTOS, a priority-3 task enters an infinite loop that never blocks. What happens to the priority-1 and priority-2 tasks and the idle task? On Linux, what happens with the same thread under `SCHED_OTHER`? Under `SCHED_FIFO`?
5. What is the result of a thread stack overflow on Linux, and on an RTOS without an MPU?
6. Why does a Cortex-M RTOS put PendSV at the lowest priority?

### Answers

1. Locals live on separate stacks and do not interfere. Globals and the heap are shared and do; protect them with a lock or give them to one owning thread.
2. On a voluntary switch, the switch routine is called as a function, so the compiler has already handled the caller-saved registers. On preemption the interrupted point is not a call site, so interrupt entry saves the rest first: a general-purpose OS traps into a trap frame, and Cortex-M hardware pushes 8 words.
3. It skips switching the page table, so the TLB is not disturbed by an address-space change, and both threads share the same file descriptors and other resources. The kernel stack, registers and scheduler work are the same either way.
4. FreeRTOS: the scheduler always runs the highest-priority task that can run, so the priority-1 and priority-2 tasks and the idle task never run. Work that depends on the idle task stops too, such as freeing the memory of deleted tasks, and a watchdog would eventually reset the system because nobody feeds it. Linux `SCHED_OTHER`: EEVDF still gives the other threads CPU time. Linux `SCHED_FIFO`: lower-priority real-time threads on the same CPU starve, while normal threads still get about 5% of each second because of the default real-time throttling.
5. Linux: the end of the stack usually has a guard page, so overflow becomes a page fault and the program is terminated, which at least leaves a trail. An RTOS without an MPU: the overflow silently overwrites neighbouring memory, and the symptom may appear much later in a different task.
6. So that a switch happens only after every other interrupt has been handled, to use tail chaining and cut the cost of exception entry and exit, and to avoid lengthening hardware interrupt latency. Those are the two reasons Zephyr's Cortex-M guide gives.

## 8. Where to see it yourself

| Mechanism | Lab |
| --- | --- |
| What `swtch` saves, and how a new thread starts in `forkret` | [Lab 02](../labs/02-processes-threads/README.md) exercises B and C |
| Write your own user-level thread, then save too few registers on purpose | Lab 02 exercises D and G |
| Clone flags for threads and processes; creation and switch costs | Lab 02 exercises E and F |
| Locks, futexes and lost wake-ups | [Lab 05](../labs/05-synchronization/README.md) |
| Priority, `SCHED_FIFO` and wake-up latency | [Lab 06](../labs/06-scheduling-latency/README.md) |
| RTOS tasks, queues, priority inversion and the ISR boundary | `mcu-foundations` [lesson 7](../../mcu-foundations/lessons/06-rtos-tasks-and-queues.md) and its [simulated kernel](../../mcu-foundations/labs/06-rtos-model/rtos_sim.c) (Traditional Chinese) |
| Whether a product should use an RTOS or Linux | [HMI research, study 19](../../hmi-studio-iot-ide-spinoff-research/research/19-rtos-vs-linux.zh-TW.md) (Traditional Chinese) |

## Sources

Checked 2026-11-20.

**Linux and POSIX**
- [pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html): per-thread and process-wide attributes; NPTL as a 1:1 implementation.
- [sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html) (man-pages 6.19): policies, real-time priorities 1–99, when a `SCHED_FIFO` thread gives up the CPU, the throttling `/proc` files and defaults, and real-time preemption without patches since 6.12. Note that this page still says the default scheduler is CFS and does not mention EEVDF.
- [sched-rt-group](https://docs.kernel.org/scheduler/sched-rt-group.html): throttling defaults of 1,000,000 µs and 950,000 µs, and the limit that preserves bandwidth for the fair dl_server.
- [EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html); kernelnewbies [Linux 6.6](https://kernelnewbies.org/Linux_6.6) (EEVDF replaces CFS) and [Linux 6.12](https://kernelnewbies.org/Linux_6.12) (EEVDF transition completed, PREEMPT_RT merged, SCHED_DEADLINE server infrastructure, sched_ext).
- [clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html).

**Windows**
- Learn: [Scheduling Priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities), [Priority Boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts), [Context Switches](https://learn.microsoft.com/en-us/windows/win32/procthread/context-switches), [Thread Local Storage](https://learn.microsoft.com/en-us/windows/win32/procthread/thread-local-storage), [About Processes and Threads](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads). The Context Switches page does not give the length of a time slice.

**FreeRTOS**
- [FreeRTOS Kernel Book, chapter 4](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch04.md): task states, TCB and stack, stack depth in words, tasks must not return, `configMAX_PRIORITIES`, the tick and time slices. The page is long and only its first part was checked word for word; the Ready and Suspended sub-states and the idle task's priority come from a summary of the later part, and match what MCU lesson 7 cites.
- [`portable/GCC/ARM_CM4F/port.c`](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/portable/GCC/ARM_CM4F/port.c): `xPortPendSVHandler`, `vPortSVCHandler`, `prvPortStartFirstTask`.
- [MPU Support](https://www.freertos.org/Security/04-FreeRTOS-MPU-memory-protection-unit) and [xTaskCreateRestricted](https://freertos.org/Documentation/02-Kernel/04-API-references/13-FreeRTOS-MPU-specific/01-xTaskCreateRestricted): an unprivileged task can access only its own stack and up to three user-defined regions, and can only be created with `xTaskCreateRestricted()`. These two pages were found through search results and not read in full.

**Zephyr**
- [Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html), [Scheduling](https://docs.zephyrproject.org/latest/kernel/services/scheduling/index.html), [User Mode Overview](https://docs.zephyrproject.org/latest/kernel/usermode/overview.html), [Arm Cortex-M Developer Guide](https://docs.zephyrproject.org/latest/hardware/arch/arm_cortex_m.html).

**Arm**
- [Cortex-M4 Devices Generic User Guide](https://support.arm.com/documentation/dui0553/latest/): could not be read by automated fetch this time (the page returned only its title). The statement that hardware pushes 8 words on exception entry follows MCU lesson 7, which cites the same document; Zephyr's Cortex-M guide also says the switched-out thread's caller-saved context and return address sit in the exception stack frame.
