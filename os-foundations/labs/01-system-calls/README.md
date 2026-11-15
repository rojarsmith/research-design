# Lab 01: System calls, crossing the boundary

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 01](../../SYLLABUS.md#01-system-calls-crossing-the-boundary)

> **Draft for review, 2026-11-15. What was and was not executed:**
> - **xv6 parts (exercises A–D): not executed.** The machine this lab was written on has no RISC-V toolchain or QEMU. Everything marked *expected* comes from reading [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) master on 2026-11-15.
> - **Linux part (exercise E): not executed.** That machine has no Linux distribution or Linux toolchain, so `linux/boundary.c` has not been compiled either.
> - **Windows parts (exercises F and G): built and run** on that machine on 2026-11-15 with gcc 16.2 and gdb 18.1 from WinLibs. The full output is in [`windows/example-output.txt`](windows/example-output.txt), and the numbers quoted below come from it.

## Goal

Follow one system call from user code into the kernel and back, instruction by instruction, in xv6. Then find the same boundary on Linux with `strace` and on Windows in ntdll's stubs and a debugger. Finally, hand each kernel a pointer it must reject, and explain each rejection in that kernel's own terms.

```
user    write() in usys.S:  li a7, SYS_write ; ecall
  hw    sepc <- pc, scause <- 8, sstatus.SPP <- 0 (from user), mode <- S, pc <- stvec
kernel  uservec   (trampoline.S)  save 31 user registers in TRAPFRAME, load kernel sp, tp, satp
        usertrap() (trap.c)       stvec <- kernelvec, epc += 4, intr_on(), syscall()
        syscall()  (syscall.c)    num = a7 ; a0 = syscalls[num]()
        sys_write -> filewrite -> consolewrite -> either_copyin -> copyin
        prepare_return() (trap.c) stvec <- uservec, fill trapframe->kernel_*, SPP <- 0, sepc <- epc
        userret   (trampoline.S)  satp <- user page table, restore registers, sret
user    the ret after ecall, with the result in a0
```

Names follow xv6-riscv master as read on 2026-11-15, which has `kfork`, `kexit`, `kexec`, `prepare_return`, `printk` and the `pause` system call. MIT's 2025 syscall lab already refers to `kfork()`. Older trees and older editions of the book use `fork`, `exit`, `exec`, `usertrapret`, `printf` and `sleep`. Before you start, run `grep -n "prepare_return\|usertrapret" kernel/*.c` to see which names your tree uses.

## Files

| File | What it is | Status |
| --- | --- | --- |
| [`xv6/badptr.c`](xv6/badptr.c) | xv6 user program that passes four bad addresses to `write`, `pipe` and `fstat`, then loads from a kernel address itself | not compiled here |
| [`xv6/trapinfotest.c`](xv6/trapinfotest.c) | test for the `trapinfo()` system call you add in exercise C | not compiled here |
| [`linux/boundary.c`](linux/boundary.c) | one `write` three ways, bad pointers, and the cost of a call that stays in user mode vs one that enters the kernel | not compiled here |
| [`windows/nt_boundary.c`](windows/nt_boundary.c) | the same questions against Win32 and ntdll | built and run here |
| [`windows/win32_write.c`](windows/win32_write.c), [`find_nt_routine.gdb`](windows/find_nt_routine.gdb), [`user_mode_calls.gdb`](windows/user_mode_calls.gdb) | a two-call Win32 program and two GDB scripts that show which ntdll routine it reaches | built and run here |
| [`windows/example-output.txt`](windows/example-output.txt) | recorded output of the Windows programs | recorded 2026-11-15 |

## Setup

- The Lab 00 workbench, with one hart while you learn: `make CPUS=1 qemu-gdb`.
- For MIT's ready-made exercises, use the lab repository and its `syscall` branch, as MIT's lab page does:

  ```sh
  git clone git://g.csail.mit.edu/xv6-labs-2025
  cd xv6-labs-2025
  git checkout syscall
  make clean
  ```

  Exercises A–D here work in that tree or in a plain xv6-riscv clone. MIT's [syscall lab](https://pdos.csail.mit.edu/6.1810/2025/labs/syscall.html) adds its own: a "Using gdb" section whose questions overlap exercise B, a sandbox system call, and an attack on memory that the lab build does not zero.
- **Linux:** WSL 2 Ubuntu from Lab 00, plus `sudo apt-get install strace`.
- **Windows:** any x86-64 MinGW-w64 gcc. The WinLibs build also ships `gdb`.

## Read first

- The xv6 book: chapter 2, and sections 4.3 and 4.4 of chapter 4. That is MIT's reading list for this lab.
- `user/usys.pl`, `user/user.h`, `kernel/syscall.h`, `kernel/syscall.c`.
- `kernel/trampoline.S` (`uservec`, `userret`), `kernel/trap.c` (`usertrap`, `prepare_return`), `struct trapframe` in `kernel/proc.h`.
- `kernel/vm.c`: `walkaddr`, `copyin`, `copyout`, `vmfault`; `kernel/memlayout.h`: `TRAMPOLINE`, `TRAPFRAME`, `KERNBASE`.

## Exercises

As in Lab 00, write each prediction in `NOTES.md` before you run anything.

### A. Read the stub (xv6)

Run `make` once, then open the generated `user/usys.S`, or find `write:` in any `user/*.asm` file.

**Predict:** the value loaded into `a7`; which registers hold `fd`, `buf` and `n`; and where the return value is when `ret` executes.

*Expected, from source:* `li a7, SYS_write`, where `SYS_write` is 16 in master's `kernel/syscall.h`, then `ecall`, then `ret`. The arguments are already in `a0`, `a1` and `a2` because the C calling convention put them there. The result comes back in `a0` because `syscall()` stores it in `p->trapframe->a0` and `userret` restores `a0` last. Linux on RISC-V uses the same convention: `ecall`, the number in `a7`, arguments in `a0`–`a5` ([syscall(2)](https://man7.org/linux/man-pages/man2/syscall.2.html)).

### B. Follow one `write` in GDB (xv6)

`uservec` runs for every trap from user mode, timer interrupts included, so break on it only for `ecall` (`scause` 8) with `a7` = 16. `uservec` executes at the `TRAMPOLINE` address, `MAXVA - PGSIZE` = `0x3ffffff000`, not at the kernel link address that GDB's symbol `uservec` has.

```
(gdb) b *0x3ffffff000 if $scause == 8 && $a7 == 16
(gdb) c
```

Type `echo hi` in the xv6 console if no write happens on its own. At the stop, **predict, then print:** `$priv`, `$satp`, `$sepc`, `$sp`, `$a0`, `$a1`, `$a2`, `$stvec`. Then:

```
(gdb) x/12i $pc           uservec; there are no symbols at this address, so compare with trampoline.S
(gdb) si                  (repeat until after "csrw satp, t1"; print $satp and $sp each time)
```

Look `$sepc` up in the `.asm` file of the program that made the call (for the shell, `user/sh.asm`). `x/2i $sepc` may also work, but GDB reads memory through QEMU with the CPU's current privilege, and supervisor mode is not normally allowed to read `PTE_U` pages. If GDB answers `Cannot access memory`, that is the reason.

At `jalr t0` you enter `usertrap()`, and kernel symbols work again:

```
(gdb) bt
(gdb) p/x $sstatus        bit 8 (SPP): the mode the trap came from
(gdb) next                (repeat until past "p->trapframe->epc = r_sepc();")
(gdb) p/x p->trapframe->epc
(gdb) next                (repeat until past "p->trapframe->epc += 4;")
(gdb) p/x p->trapframe->epc
(gdb) b syscall
(gdb) c
(gdb) next
(gdb) p num
```

For the way back, compute where `userret` runs. Its symbol has the kernel link address, but it executes in the trampoline page:

```
(gdb) p/x 0x3ffffff000 + ((long)&userret - (long)&trampoline)
(gdb) b *<that value>
(gdb) c
(gdb) p/x $a0             the user satp that usertrap() returned
(gdb) si                  (repeat to the sret; print $sepc and $sstatus just before it)
(gdb) si
(gdb) p $priv
(gdb) p/x $pc
```

*Expected, from source:* at `uservec`, `$priv` is 1 (supervisor), `$satp` still holds the user page table, `$sepc` points at the `ecall`, `$sp` is the user stack and `$stvec` equals `0x3ffffff000`. `csrw satp, t1` switches to the kernel page table, and the code keeps running because the trampoline page is mapped at the same virtual address in both. After `sret`, `$priv` is 0 and `$pc` is the instruction after the `ecall`.

### C. Add a system call: `trapinfo` (xv6)

Add `int trapinfo(struct trapinfo *ti)`. It copies to user memory what the trap path saved for this very call. Create `kernel/trapinfo.h` with the struct shown at the top of [`xv6/trapinfotest.c`](xv6/trapinfotest.c) (`epc`, `ra`, `sp`, `a0`, `a7`, all `uint64`). Then edit the five places every xv6 system call needs:

1. `user/user.h`: a forward declaration `struct trapinfo;` and the prototype.
2. `user/usys.pl`: `entry("trapinfo");`.
3. `kernel/syscall.h`: `SYS_trapinfo`, the next free number in your tree.
4. `kernel/syscall.c`: the `extern` declaration and the `[SYS_trapinfo]` entry in `syscalls[]`.
5. `kernel/sysproc.c`: `sys_trapinfo()`. Read the destination with `argaddr`, fill a `struct trapinfo` from `myproc()->trapframe`, and copy it out with `copyout`. Check `copyout`'s signature in your tree; master takes the process size as a second argument. Return -1 if `copyout` fails.

Copy `trapinfotest.c` to `user/`, add `$U/_trapinfotest` to `UPROGS`, and run it.

**Predict, before running:** Is `epc` the address of the `ecall` or of the instruction after it? Find both in `user/trapinfotest.asm`. Is `a0` equal to `&ti`? (The trap frame holds `a0` as `uservec` saved it, before `syscall()` writes the return value.) What is `ra`? Do the three bad destinations at the end all return -1?

**Then decide what not to copy.** `struct trapframe` also holds `kernel_satp`, `kernel_sp` and `kernel_trap`. User code cannot read the trap frame page itself, because it is mapped without `PTE_U`. Write down what a process would learn if `trapinfo` copied those three fields out, and why kernels that randomize their own layout treat such a leak as a security bug.

### D. Bad pointers (xv6)

Copy [`xv6/badptr.c`](xv6/badptr.c) to `user/`, add `$U/_badptr` to `UPROGS`, and **fill in this table before running it:**

| Address | `write(1, addr, 8)`, kernel copies in | `pipe(addr)`, kernel copies out | `fstat(0, addr)`, kernel copies out |
| --- | --- | --- | --- |
| `0`, the start of user text | | | |
| `KERNBASE` `0x80000000` | | | |
| `TRAPFRAME` `0x3fffffe000` | | | |
| `MAXVA` `0x4000000000` | | | |

Also predict part 4: what happens when user code loads from `KERNBASE` itself, what the kernel prints, and the child's exit status.

<details>
<summary>What the source suggests. Read this only after you have predicted and run it.</summary>

From xv6-riscv master on 2026-11-15:

- `walkaddr()` refuses addresses at or above `MAXVA`, PTEs without `PTE_V`, and PTEs without `PTE_U`. If it fails, `copyin`/`copyout` call `vmfault()`, which only allocates lazily grown heap pages below `p->sz`.
- `copyout()` also refuses pages without `PTE_W`, so the read-only text page at address 0 can be read by `write` but not written by `pipe` or `fstat`. Trees without that check let `pipe(0)` overwrite the program's own code. If your tree is one of them, expect a crash later in the program and explain it.
- `consolewrite()` stops at the first failed copy and returns the count copied so far. So the console `write` cases return 0, not -1.
- `KERNBASE` fails because the user page table has no mapping there at all: `copyin` walks the process's page table, not the kernel's. `TRAPFRAME` is mapped in that table, but without `PTE_U`.
- Part 4: the load causes a page fault (`scause` 13). `usertrap()` prints `usertrap(): unexpected scause 0xd ...` with `stval=0x80000000`, marks the process killed, and the parent sees status -1.

</details>

**Break it:** delete the `PTE_U` test in `walkaddr()`. Predict what `write(1, TRAPFRAME, 8)` prints now, then run it. The kernel now copies a page that the process itself may not read. This kind of bug is called a confused deputy. Restore the test.

### E. The same boundary on Linux (WSL 2)

```sh
gcc -O2 -Wall -o boundary linux/boundary.c
./boundary
strace -o boundary.trace ./boundary > /dev/null
strace -c ./boundary > /dev/null
```

**Predict, then check in `boundary.trace` and the `-c` summary:**

1. The program writes the same line three ways: the libc wrapper, `syscall(SYS_write, ...)`, and a raw `syscall` instruction. Can `strace` tell them apart?
2. What do the two bad-pointer writes return, and with which `errno`?
3. How many `getpid` calls does `strace -c` count, and does `clock_gettime` appear at all? The [vdso(7)](https://man7.org/linux/man-pages/man7/vdso.7.html) page lists `__vdso_clock_gettime` among the x86-64 vDSO symbols and says vDSO calls do not show up in `strace`. Since glibc 2.25, `getpid()` always makes the real system call ([getpid(2)](https://man7.org/linux/man-pages/man2/getpid.2.html)). If `clock_gettime` does show up as a system call, record that and find out why on your system.
4. How much slower is `syscall(SYS_getpid)` under `strace` than without it?

Compare the registers with xv6. On x86-64 the number goes in `rax` and the arguments in `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9` ([syscall(2)](https://man7.org/linux/man-pages/man2/syscall.2.html)). The fourth argument uses `r10` instead of `rcx` because the `SYSCALL` instruction saves the return address in `rcx` and the flags in `r11`. The comment above the 64-bit entry code in Linux's [`arch/x86/entry/entry_64.S`](https://github.com/torvalds/linux/blob/master/arch/x86/entry/entry_64.S) describes this, and adds that `SYSCALL` saves nothing on the stack. RISC-V's `ecall` saves nothing on the stack either: the hardware records the trap in `sepc`, `scause`, `stval` and `sstatus`, and `uservec` stores every general-purpose register itself.

### F. The same boundary on Windows (run here)

```powershell
cd windows
gcc -O2 -Wall -o nt_boundary.exe nt_boundary.c
.\nt_boundary.exe
```

**Predict, before running:** for each row of part 1, will the call enter the kernel? Roughly how many plain calls does one kernel entry cost? Then run it.

Observed on 2026-11-15 (Windows build 26300, Intel Core i7-11700, hypervisor present; [full output](windows/example-output.txt)):

| Call | ns per call | Thread kernel time | Thread user time |
| --- | ---: | ---: | ---: |
| plain C function call | 2.0 | 16 ms | 484 ms |
| `GetCurrentProcessId()` | 2.4 | 0 ms | 500 ms |
| `GetTickCount64()` | 2.7 | 0 ms | 500 ms |
| `GetProcessHandleCount()` | 373.1 | 266 ms | 266 ms |
| `NtQueryInformationProcess()` | 438.0 | 234 ms | 297 ms |
| `WaitForSingleObject(set event, 0)` | 429.7 | 250 ms | 266 ms |

What this run shows, and what it does not:

- Three calls cost about as much as a plain call. The other three cost 370–440 ns, about 190 to 220 times a plain call. The disassembly below shows why.
- The user-only rows charged almost all of their time to user mode. The kernel rows charged roughly half to each mode, in every run made that day, even though the user-mode part of each call is a handful of instructions. Learn's [GetThreadTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadtimes) page does not say how these times are collected, so do not read the split as a measurement of where the time went. **Exercise:** propose an explanation and design a test for it. Stage 06's WPR/WPA trace is one way to check.

Part 2 prints the start of four ntdll stubs. All four had the same shape on this build. GDB shows it for `NtWriteFile` (its export is also named `ZwWriteFile`; Learn says the two [behave identically when called from user mode](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines)):

```
mov    r10,rcx
mov    eax,0x8
test   BYTE PTR ds:0x7ffe0308,0x1
jne    <+21>
syscall
ret
int    0x2e
ret
```

- `eax` gets the system service number: 0x8 for `NtWriteFile`, 0x19 for `NtQueryInformationProcess`, 0x4 for `NtWaitForSingleObject` and 0xF for `NtClose` on this build. These numbers are internal and can change with any Windows build.
- `mov r10, rcx` is there for the same reason as Linux's `r10`: `syscall` overwrites `rcx`.
- The test at `0x7ffe0308` chooses between `syscall` and `int 2Eh`. That address lies in `KUSER_SHARED_DATA`. Learn's [structure page](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-kuser_shared_data) describes a `SystemCall` member that is set when the system uses a different system-call mechanism. Check the offset in the WDK's `ntddk.h`.

Now find the routine under a Win32 call with a debugger, as the build plan asks:

```powershell
gcc -O0 -g -Wall -o win32_write.exe win32_write.c
gdb -q -batch -x find_nt_routine.gdb win32_write.exe
gdb -q -batch -x user_mode_calls.gdb win32_write.exe
```

Observed here, with standard output redirected to a pipe:

- Both `WriteFile` calls, to standard output and to a disk file, stopped in `ntdll!ZwWriteFile`, called from `KernelBase.dll!WriteFile`, called from `main`. The breakpoint on `NtDeviceIoControlFile` never fired.
- `GetCurrentProcessId` is `mov rax, QWORD PTR gs:0x40` followed by `ret`. On x64 Windows, `gs` addresses the current thread's environment block (TEB), so the process id is read from user memory.
- `GetTickCount64` reads `0x7ffe0004` and `0x7ffe0320`, which are the `TickCountMultiplier` and `TickCount` members of `KUSER_SHARED_DATA`. The kernel keeps that data current, and user code only reads it.
- `GetProcessHandleCount` in `kernel32.dll` is a single indirect jump into another DLL. Follow it yourself with `x/12i` at the jump target.

**Your turn:** run `find_nt_routine.gdb` from a real console window (Windows Terminal), then again with output redirected to a file. Does the console write still reach `NtWriteFile`? Record what you find. It was not tested here.

### G. Previous mode: bad pointers on NT (run here)

Part 3 of `nt_boundary.exe` passes a kernel-half address and `NULL` as buffers. Observed:

| Call | Buffer | Result |
| --- | --- | --- |
| `NtQueryInformationProcess` | kernel address | `0xC0000005` (`STATUS_ACCESS_VIOLATION`) |
| `NtQueryInformationProcess` | `NULL` | `0xC0000005` |
| `NtWriteFile` | kernel address | `0xC0000005`, which `RtlNtStatusToDosError` maps to 998 |
| `NtWriteFile` | `NULL` | `0xC00000E8`, which maps to 1784 |
| `WriteFile` | kernel address | `FALSE`, error 998, "Invalid access to memory location." |
| `WriteFile` | `NULL` | `FALSE`, error 1784, "The supplied user buffer is not valid for the requested operation." |

Explain each row using Learn's account of the native services. A call from user mode always has its parameters treated as untrusted, and the routine probes caller-supplied buffers to check that they lie in user memory ([Nt and Zw routines](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines), [PreviousMode](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/previousmode)). [ProbeForRead](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-probeforread) raises `STATUS_ACCESS_VIOLATION` for a range outside user mode. It also warns that another thread can unmap or change the buffer at any time, even after the probe, so later accesses need exception handling too. The two `NULL` rows differ, so the two services check a `NULL` buffer differently; Learn does not say why. Win32 then turns each `NTSTATUS` into a Win32 error code.

## Linux / NT comparison

- **Who checks the pointer.** xv6 copies through the process's own page table, so a kernel address is simply unmapped there. In Linux and NT the kernel works in an address space where user memory and kernel memory are both mapped, so a user-supplied pointer could name kernel memory, and the kernel must range-check it. Linux's generic `__access_ok` compares the address with `TASK_SIZE_MAX`, and its comment notes that architectures with a separate user address space can skip the check ([access_ok.h](https://github.com/torvalds/linux/blob/master/include/asm-generic/access_ok.h)). NT probes the buffer when the previous mode is user mode.
- **Nt and Zw.** NT system services have Nt and Zw entry points. From user mode the two behave the same. From kernel mode, a Zw call marks the parameters as trusted, and an Nt call uses the thread's previous mode to decide ([Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines)).
- **The documented interface.** Win32 is the documented interface. Learn describes native calls such as `NtQueryInformationProcess` as internal and subject to change between releases ([Learn](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess)). Linux's ABI documentation lists the kernel system call interface itself as stable ([ABI](https://docs.kernel.org/admin-guide/abi.html)), so a program may call it by number, as `syscall(2)` does. A Windows program that hard-coded the service numbers from part 2 would break on another build.
- **Calls that skip the kernel.** Linux's vDSO serves `clock_gettime` in user mode. Windows serves `GetTickCount64` from `KUSER_SHARED_DATA` and `GetCurrentProcessId` from the TEB. xv6 has no such path: every system call traps.

## Checkpoints

Answer these in `NOTES.md`.

1. Draw `write(1, buf, n)` going into the xv6 kernel and back. Show every register saved and where it is saved, every stack switch, every page table in use, and every privilege change. Mark which steps the hardware does on `ecall` and `sret` and which steps software does.
2. Why must the trampoline page be mapped at the same virtual address in the user and kernel page tables? Why is `TRAPFRAME` mapped without `PTE_U`?
3. Why does `usertrap()` add 4 to `epc`, and what would happen without it? Why does it enable interrupts only after reading `sepc`, `scause` and `sstatus`?
4. Explain what happens when `buf` points into kernel memory: in xv6 terms (`walkaddr`, the user page table, the return value) and in NT's previous-mode terms (probe, `STATUS_ACCESS_VIOLATION`, Win32 error 998). Then add one line on Linux (`access_ok`, `EFAULT`).
5. Fill in a table of every call in exercises E and F that you claim enters the kernel or stays in user mode. For each one, give the evidence (stub bytes, disassembly, `strace`, timing), not the expectation.
6. In your `trapinfo`, which trap frame fields did you copy out, which did you leave out, and why?

## Common failure modes

- **A breakpoint on `uservec` or `userret` by name never fires.** The symbols have kernel link addresses, but the code runs at `TRAMPOLINE`. Break on `*0x3ffffff000` and on the computed `userret` address.
- **The breakpoint at `0x3ffffff000` fires all the time.** Timer interrupts from user mode also enter `uservec`. Use the `$scause == 8` condition.
- **`No symbol "p" in current context`.** You are in the wrong frame. Use `bt`, then `frame N` or `up`.
- **`unknown sys call 23` on the console.** `syscall()` found no handler. The number in `syscall.h` and the `syscalls[]` entry do not match, or you did not rebuild.
- **`implicit declaration of function 'trapinfo'`.** The prototype is missing from `user/user.h`. The Makefile builds with `-Werror`, so every warning stops the build.
- **`undefined reference to 'trapinfo'`.** The `entry("trapinfo")` line is missing from `usys.pl`.
- **`exec trapinfotest failed`.** The program is not in `UPROGS`, so it is not in `fs.img`.
- **Different names in your tree:** `fork`/`kfork`, `usertrapret`/`prepare_return`, `printf`/`printk`. Grep before you set breakpoints.
- **`badptr` crashes after the address-0 cases.** Your `copyout` lacks the `PTE_W` check, so `pipe(0)` overwrote the program's code. Explain the crash instead of hiding it.
- **Windows: GDB prints "Could not find platform independent libraries".** WinLibs gdb 18.1 printed this Python warning on the machine this lab was written on, and the scripts still worked.
- **Windows: `Function "NtWriteFile" not defined`.** ntdll is not loaded until the program starts. Both scripts stop at `main` first for that reason.
- **Windows: timings jump between runs.** Power plan, turbo, background load and the hypervisor all show up in these numbers. Run several times and report the spread, not one run.

## Where this shows up later

- Stage 02 starts from this trap frame: `fork` copies it, and `swtch` saves a much smaller set of registers.
- Stage 03 changes what `walkaddr` and `vmfault` accept: page tables, lazy allocation and copy-on-write.
- Stages 08 and 17 hand a server memory that a client controls. The rule from this lab, that a pointer from the other side is untrusted, becomes the rule for shared buffers.

## Next

[Lab 02](../02-processes-threads/README.md) builds a pipeline with `fork`, `exec`, `pipe` and `wait`, traces `swtch`, and writes user-level threads.
