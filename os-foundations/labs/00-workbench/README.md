# Lab 00: Workbench, booting and debugging xv6

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 00](../../SYLLABUS.md#00-workbench-booting-and-debugging-xv6)

> **Draft for review, 2026-11-15, run on 2026-11-16. What was and was not executed:**
> - **Run on 2026-11-16: setup steps 3 and 5, exercises A–G and both break-it steps.** They ran in an Ubuntu 24.04 container under podman on Windows 11, whose podman machine runs on WSL 2. That is not the WSL 2 Ubuntu distribution that setup step 1 installs. The container had QEMU 8.2.2, riscv64-linux-gnu-gcc 13.3.0, binutils 2.42 and gdb-multiarch 15.1, and xv6-riscv master at commit `fa2ddcb` (2026-11-05). GDB was driven from batch scripts with the commands shown below. The output is in [`xv6/example-output.txt`](xv6/example-output.txt), and the observed values below come from it.
> - **Not run: setup steps 1, 2 and 4** (installing WSL 2 and Ubuntu, `apt-get`, `git clone`), because the container already had the packages and an xv6 checkout. The failure modes for WSL 1, `/mnt/c`, CRLF line endings and a missing `bc` were not reproduced.
> - The lab was written on a Windows 11 machine without WSL, a RISC-V toolchain or QEMU, from MIT 6.1810's [tools page](https://pdos.csail.mit.edu/6.1810/2025/tools.html), the [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) source and QEMU's source, read on 2026-11-15. Your tools and commit may differ, so write down what you actually see.

## Goal

Build a working xv6 workbench and use it to answer one question exactly: what runs between QEMU's reset and xv6's `main()`, and in which privilege mode? When you finish you can boot xv6, quit it, rebuild it, attach GDB, and stop at any instruction from the reset vector to the shell prompt.

```
QEMU reset ROM   ->  _entry        ->  start()      --mret-->  main()       ->  scheduler()  ->  /init, sh
0x1000, M-mode       0x80000000        start.c                 main.c           proc.c           U-mode
                     entry.S, M-mode   M-mode                  S-mode           S-mode
```

## Prerequisites

C with pointers and structs, a Linux shell, git and make, and reading RISC-V assembly at the level of stage 1 of the [`c-compiler-foundations` path](../../../c-compiler-foundations/research/04-integration-path.zh-TW.md): you can read a `-O0` prologue, find `ra`, and explain negative offsets from `fp`. xv6 is RV64 (`-march=rv64gc`), so registers and pointers are 64 bits wide, and you will meet CSRs and privilege modes, which that path does not cover.

## Setup (steps 1, 2 and 4 not run for this draft)

These steps follow MIT's 2025 tools page, which was still the current one on 2026-11-15: the Fall 2026 course home linked a `tools.html` that returned 404 that day.

1. **WSL 2 with Ubuntu 24.04.** MIT's Windows route is WSL 2 plus Ubuntu 24.04, and the page says WSL 1 does not work. In an administrator PowerShell, list the distributions with `wsl --list --online`, install the 24.04 one with `wsl --install -d <name from the list>`, then check that `wsl -l -v` shows `VERSION 2` for it ([Install WSL](https://learn.microsoft.com/en-us/windows/wsl/install)).
2. **Packages**, inside Ubuntu, exactly as MIT lists them:

   ```sh
   sudo apt-get update && sudo apt-get upgrade
   sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
   ```

3. **Versions.** MIT asks for QEMU 7.2 or newer and GDB 8.3 or newer. On 2026-11-15, packages.ubuntu.com listed these versions for Ubuntu 24.04 (noble): `qemu-system-misc` 8.2.2, `gdb-multiarch` 15.0.50, `gcc-riscv64-linux-gnu` 13.2.0, `binutils-riscv64-linux-gnu` 2.42. Record what you actually got:

   ```sh
   qemu-system-riscv64 --version
   riscv64-linux-gnu-gcc --version
   riscv64-linux-gnu-ld --version
   gdb-multiarch --version
   ```

   Observed in the container on 2026-11-16: QEMU 8.2.2, gcc 13.3.0, GNU ld 2.42 and GDB 15.1. The compiler and GDB are newer than the list above because they came from Ubuntu's updates (`gdb-multiarch 15.1-1ubuntu1~24.04.1`). The `gcc-riscv64-linux-gnu` package itself was still 13.2.0, and it pulls in `gcc-13-riscv64-linux-gnu` 13.3.0.

4. **Source**, cloned inside the Linux file system (your home directory), not under `/mnt/c`:

   ```sh
   cd ~
   git clone https://github.com/mit-pdos/xv6-riscv.git
   cd xv6-riscv
   git log -1 --format='%H %cd'
   ```

   Record the commit. From stage 01 on you will also use MIT's lab repository, `git clone git://g.csail.mit.edu/xv6-labs-2025`, which has one branch per lab.
5. **Let GDB read xv6's `.gdbinit`.** GDB refuses to run a `.gdbinit` from the current directory unless you allow it. MIT's guidance says to add the line the warning suggests to `~/.gdbinit`. For this checkout that is:

   ```sh
   echo "add-auto-load-safe-path $HOME/xv6-riscv/.gdbinit" >> ~/.gdbinit
   ```

   GDB 15.1's warning names `~/.config/gdb/gdbinit` as the file to edit. `~/.gdbinit` works too: the run for this lab put the line there, and GDB read `.gdbinit` from then on.

Start `NOTES.md` now. Write down the Windows build, the WSL version, the distribution, every version above and the xv6 commit.

## Read first

All of these are short.

- `Makefile`: `QEMUOPTS`, `CPUS := 3`, the `qemu` and `qemu-gdb` targets, `GDBPORT` (computed from your user id), and the rule that generates `.gdbinit` from `.gdbinit.tmpl-riscv`.
- `kernel/kernel.ld`: the kernel is linked at `0x80000000`, `kernel/entry.o(_entry)` goes first, and the trampoline gets a page of its own.
- `kernel/entry.S`: each hart computes `sp = stack0 + (hartid + 1) * 4096` and calls `start`.
- `kernel/start.c`: sets the previous privilege in `mstatus` to supervisor and `mepc` to `main`, turns paging off, delegates traps to supervisor mode, opens physical memory protection (PMP), lets the hardware set the page-table A and D bits (`menvcfg`), asks for timer interrupts, stores the hart id in `tp`, then executes `mret`.
- `kernel/main.c`: hart 0 initializes everything, and the other harts wait for `started`.
- The [xv6 book](https://pdos.csail.mit.edu/6.1810/2025/xv6/book-riscv-rev5.pdf), chapter 2, the section on starting xv6.

## Exercises

Before every exercise, write your prediction in `NOTES.md`. Then run it and write what happened next to the prediction. A wrong prediction you can explain counts for more than a right guess.

### A. Boot, look, quit

**Predict:** how many `hart N starting` lines appear, and in which order? Which process names will Ctrl-p list?

```sh
make qemu
```

At the `$` prompt, type `ls`, then press Ctrl-p. `procdump()` in the kernel prints one line per process: pid, state and name. Quit QEMU with Ctrl-a, then x.

Boot again with `make CPUS=1 qemu` and check your prediction for one hart.

<details>
<summary>Observed on 2026-11-16. Read this only after you have predicted and run it.</summary>

- With the default `CPUS := 3`: `xv6 kernel is booting`, then `hart 2 starting` and `hart 1 starting`, then `init: starting sh` and the `$` prompt. The next boot of the same build printed `hart 1` first. In `kernel/main.c`, hart 0 prints the banner and initializes everything, and the other harts print their line once `started` is set, in whatever order they get there.
- Ctrl-p after `ls` printed `1 sleep  init` and `2 sleep  sh`. `ls` had already exited.
- Ctrl-a x printed `QEMU: Terminated`.
- With `CPUS=1` no `hart N starting` line appeared.

</details>

### B. Every QEMU flag

`make -n qemu` prints the commands without running them. Observed: first the shell code of the `check-qemu-version` rule, with the `bc` result already filled in (`if [ "1" -eq 0 ]; then ...`), then the QEMU command on one line. Here it is split for reading:

```
qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic
  -global virtio-mmio.force-legacy=false
  -drive file=fs.img,if=none,format=raw,id=x0
  -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
```

For each flag, write one line in `NOTES.md` on what it does. Then name the line of xv6 source that depends on it. Start with these:

- `-machine virt` fixes the physical memory map. Compare QEMU's `virt` map with `kernel/memlayout.h`: `UART0`, `VIRTIO0`, `PLIC`, `KERNBASE`.
- `-bios none` means no firmware runs, so xv6 itself starts in machine mode. Exercise C shows the few instructions QEMU still runs first.
- `-m 128M` matches `PHYSTOP = KERNBASE + 128 * 1024 * 1024`.
- `-smp 3` must not exceed `NCPU` in `kernel/param.h`, which is 8 in the tree read for this lab. `stack0` in `start.c` has `NCPU` stacks.

**Break it:** change `-m 128M` to `-m 64M` in `QEMUOPTS`. Predict where and how the kernel fails. Hint: `kinit()` hands every page up to `PHYSTOP` to `freerange()`, and `kfree()` fills each page with junk. Run it, record what happens, and restore the flag.

<details>
<summary>Observed with <code>-m 64M</code>. Read this only after you have predicted and run it.</summary>

The console printed `xv6 kernel is booting` and then nothing at all, with no panic message. To find out why, run it under `make CPUS=1 qemu-gdb` with `b *0x0`: at this point `stvec` still holds its reset value 0, because `trapinithart()` runs later. The stop came in supervisor mode with `scause` 7 (store/AMO access fault), `sepc` in `memset`, and `stval` `0x84000000`, the first byte past 64 MiB of RAM. GDB could not read memory at address 0 either, so the trap handler address points at nothing, and the kernel never gets to print.

</details>

### C. The first instruction is not xv6's

Run QEMU stopped at reset in one terminal, and attach GDB from the same directory in a second terminal. Use one hart so that breakpoints fire once:

```sh
make CPUS=1 qemu-gdb      # terminal 1: prints "*** Now run 'gdb' in another window."
gdb-multiarch             # terminal 2, in ~/xv6-riscv, reads .gdbinit
```

**Predict:** what is `$pc` when GDB attaches?

```
(gdb) p/x $pc
(gdb) x/6i $pc
(gdb) p $priv
```

Single-step with `si` until `$pc` is `0x80000000`, and print `$a0`, `$a1` and `$a2` on the way. `x/2gx 0x1018` shows the two words the ROM loads.

QEMU's gdb stub exposes the current privilege level as a virtual register, `priv` (`target/riscv/gdbstub.c`), with 3 meaning machine mode, 1 supervisor and 0 user. If your GDB does not show `$priv`, use `p/x $mstatus` and the reasoning in exercise E instead.

<details>
<summary>What QEMU's source says, and what the run showed. Read this only after you have predicted and run it.</summary>

From QEMU's source: `hw/riscv/virt.c` maps a small ROM at `0x1000`, and `riscv_setup_rom_reset_vec()` in `hw/riscv/boot.c` writes six instructions into it. They put the hart id in `a0` and the device-tree address in `a1`, then jump to the address stored at `0x1018`. With `-bios none` that address is the start of RAM.

Observed (QEMU 8.2.2, GDB 15.1): `$pc` was `0x1000`, and `x/6i $pc` printed `auipc t0,0x0`, `addi a2,t0,40`, `csrr a0,mhartid`, `ld a1,32(t0)`, `ld t0,24(t0)` and `jr t0`. `$priv` was 3. `x/2gx 0x1018` printed `0x0000000080000000` and `0x0000000087e00000`. After the sixth `si`, `$pc` was `0x80000000` (`_entry () at kernel/entry.S:12`), with `a0` 0, `a1` `0x87e00000` and `a2` `0x1028`.

</details>

### D. From `_entry` to `start()`

```
(gdb) x/8i 0x80000000
(gdb) si          (repeat, and watch $sp, $a0, $a1)
(gdb) p/x $sp
(gdb) p &stack0
```

**Predict:** the value of `sp` just before `call start`, for hart 0. Check it against `&stack0`. Why `hartid + 1` and not `hartid`? Which way does the stack grow?

The first xv6 instruction is `la sp, stack0` at `_entry`. `la` is a pseudo-instruction, so `x/i` shows the real instructions it became. Write down what they are.

Rerun with the default three harts: `make qemu-gdb`, then `b *0x80000000` and `c` several times. **Predict** how many times the breakpoint is hit, then check with `info threads` and `info breakpoints`. QEMU presents each hart to GDB as a thread.

<details>
<summary>Observed. Read this only after you have predicted and run it.</summary>

- `la sp, stack0` became `auipc sp,0xa` and `ld sp,472(sp)`: a load of `stack0`'s address from the global offset table, which the kernel has (`.got` in exercise G). That is how the assembler expands `la` in position-independent code. Ubuntu's `riscv64-linux-gnu-gcc` is built with `--enable-default-pie`, and the Makefile's rule for `.S` files, unlike `CFLAGS`, does not pass `-fno-pie`. Assembling `entry.S` by hand with `-fno-pie` gave `auipc` and `addi` instead.
- `li a0, 1024*4` became `lui a0,0x1`, and `call start` became `jal 0x80000054 <start>`.
- Just before the `jal`, `sp` was `0x8000b220`, and `p &stack0` printed `0x8000a220`: hart 0's stack is the first 4096 bytes, and `sp` starts at its top. `a0` was `0x1000` and `a1` 1.
- With three harts, the breakpoint was hit three times, by threads 1, 3 and 2 in that run, and `info breakpoints` reported `already hit 3 times`. With `b scheduler` added, the next stop was `scheduler` on thread 1.

</details>

### E. `start()` and the `mret`

```
(gdb) b start
(gdb) c
(gdb) p/x $mstatus
(gdb) next        (repeat, printing $mstatus, $mepc, $satp, $medeleg, $mideleg)
```

- The MPP field of `mstatus` is bits 12:11 (`MSTATUS_MPP_MASK` in `kernel/riscv.h`). **Predict** its value after `w_mstatus(x)`: 0b01 means supervisor.
- After `w_mepc((uint64)main)`, compare `p/x $mepc` with `p main`.
- `start()` writes `0xffff` to `medeleg` and `mideleg`. **Predict** whether reading them back gives `0xffff`. The privileged spec makes many CSR fields WARL (write any value, read a legal one). Write down what you see and find the rule that explains it.
- Step to the `mret` itself with `si` and `x/i $pc`, or find its address in `kernel/kernel.asm`, `b *` it and `c`. Print `$priv` and `$pc`. Then `b *$mepc`, execute one `si`, and print both again. That one instruction is where xv6 leaves machine mode. Do not skip the breakpoint: in the run recorded for this lab (QEMU 8.2.2, GDB 15.1), a plain `si` on `mret` did not stop after one instruction, and xv6 ran on to the shell prompt. Copy the GDB output into `NOTES.md`; checkpoint 3 asks for it.

<details>
<summary>Observed in <code>start()</code>. Read this only after you have predicted and run it.</summary>

- `mstatus` was `0xa00000000` on entry and `0xa00000800` after `w_mstatus(x)`, so MPP was 0b01.
- After `w_mepc((uint64)main)`, `mepc` was `0x80000e2e`, and `p main` printed the same address.
- `medeleg` read back `0xbfff` and `mideleg` `0x3666`, not `0xffff`. Before the write, `mideleg` already read `0x1444`. Find the rule for each bit that did not stick and each bit that was set before xv6 wrote anything.
- `satp` stayed 0. `pmpaddr0` read `0x3fffffffffffff` and `pmpcfg0` `0xf`.
- At the `mret` (`0x800000c6` in this build), `$priv` was 3. After `b *$mepc` and one `si`, `$priv` was 1 and `$pc` `0x80000e2e`, the first instruction of `main`. `mstatus` was then `0xa00000080`: MPP back to 0, MPIE set.

</details>

**Break it:** comment out the two PMP lines (`w_pmpaddr0`, `w_pmpcfg0`), rebuild, and step over the `mret` again, with `b *$mepc` and also `b *0x0` set. Predict first. Then observe `$pc`, `$priv`, `$scause`, `$mcause` and `$mepc`. Restore the lines afterwards.

<details>
<summary>Observed without the PMP lines. Read this only after you have predicted and run it.</summary>

`pmpaddr0` and `pmpcfg0` read 0 at the `mret`. The `si` stopped at address 0, with `$priv` still 3, `$mcause` 1 (instruction access fault), `$mepc` equal to the address of the `mret` itself, and `$scause` 0. The fault was taken in machine mode, so the delegation to supervisor mode did not apply. It went to `mtvec`, which still held its reset value 0 because xv6 never writes it. Note that the fault was reported at the `mret`, not at the first instruction of `main`; find the code in QEMU's RISC-V target that decides this.

</details>

### F. `main()` and the first page table

```
(gdb) b main
(gdb) c
(gdb) bt
(gdb) set backtrace past-main on
(gdb) bt
(gdb) p $priv
(gdb) b kvminithart
(gdb) c
(gdb) p/x $satp
(gdb) finish
(gdb) p/x $satp
```

**Predict:** what will `bt` print at `main`, before and after `set backtrace past-main on`? Nothing called `main`. `mret` loaded `pc` from `mepc` and left `ra` holding whatever the last `call` inside `start()` put there. GDB builds the backtrace from that `ra`, so read its output with suspicion. After `kvminithart()`, the top four bits of `satp` should be 8 (`SATP_SV39`), and the rest is the physical page number of the kernel page table. **Predict:** do kernel addresses in GDB change once paging is on? Hint: read `kvmmake()` in `kernel/vm.c` and look at how it maps kernel text and RAM.

Continue with `c` until the shell prompt appears in terminal 1.

<details>
<summary>Observed. Read this only after you have predicted and run it.</summary>

- The first `bt` printed one frame, `#0 main () at kernel/main.c:13`, because GDB stops a backtrace at `main` by default. After `set backtrace past-main on` it printed `#1 0x00000000800000be in start () at kernel/start.c:44` and `#2 0x000000008000001a in _entry () at kernel/entry.S:19`. Line 44 is the call to `timerinit()`. `$ra` was `0x800000be`, `start+106`, the return address of that call.
- `$priv` was 1.
- `satp` was 0 at `kvminithart` and `0x8000000000087fff` after `finish`: mode 8, PPN `0x87fff`. `p/x kernel_pagetable` printed `0x87fff000`.
- After paging was on, `x/2i $pc` showed the same kind of addresses as before (`0x80000ea8 <main+122>`).

</details>

### G. The linker script, from the outside

```sh
riscv64-linux-gnu-readelf -h kernel/kernel | grep Entry
riscv64-linux-gnu-objdump -h kernel/kernel
grep -E ' (_entry|start|main|etext|_trampoline|end)$' kernel/kernel.sym
```

**Predict** the entry point and the first address of `.text` before you run these. Check that `_trampoline` is page aligned. Then answer: QEMU jumped to `0x80000000` because that is the start of RAM, not because it read the ELF entry point. What would happen if `kernel.ld` placed a different object file first?

<details>
<summary>Observed. Read this only after you have predicted and run it.</summary>

The entry point was `0x80000000`, and `.text` started there with size `0x7000`. `kernel.sym` listed `_trampoline` at `0x80006000` (page aligned), `etext` at `0x80007000`, `main` at `0x80000e2e`, `start` at `0x80000054` and `end` at `0x80023528`. `objdump -h` also listed `.got` and `.got.plt`, the table that `la sp, stack0` reads in exercise D.

</details>

## Checkpoints

Answer these in `NOTES.md`, with the GDB output that supports each answer.

1. Explain every QEMU flag from exercise B. For each one, name the xv6 constant or code that depends on it.
2. Show the first instruction the CPU executes (in QEMU's ROM at `0x1000`) and the first instruction xv6 executes (`_entry` at `0x80000000`), as GDB printed them.
3. Show the instruction where xv6 leaves machine mode, with `$priv` and `$pc` before and after it.
4. Compare with reset to `main` on a microcontroller (stage 1 of [`mcu-foundations`](../../../mcu-foundations/02-learning-path.md)). What extra work exists because this machine has privilege levels? List each item in `start()`: privilege setup, trap delegation, PMP, `menvcfg`, timer access, `tp`. Then go the other way: an MCU startup file copies `.data` and zeroes `.bss`, and `entry.S` does neither. Who did that work for xv6?
5. Copy the `bt` output at `main`, with and without `set backtrace past-main on`. Where it lists `start` as the caller, explain why that is misleading: which instruction actually transferred control, and where did the `ra` value that GDB used come from?

## Common failure modes

- **WSL 1.** MIT says the labs do not work under WSL 1. Check with `wsl -l -v`, and convert with `wsl --set-version <distro> 2`.
- **`Couldn't find a riscv64 version of GCC/binutils`.** The Makefile tries several tool prefixes by running each `objdump`. Install the packages from step 2, or run `make TOOLPREFIX=riscv64-linux-gnu-`.
- **`ERROR: Need qemu version >= 7.2`.** The distribution's QEMU is too old. MIT notes that you will probably need Ubuntu 24 or later.
- **A shell error from `check-qemu-version`.** That rule compares versions with `bc`. If `bc` is missing, install it with `sudo apt-get install bc`. (MIT's Arch package list includes `bc`; the Ubuntu list does not.)
- **`auto-loading has been declined`.** GDB ignored `.gdbinit`, so it is not connected and has no symbols; `p $pc` answers `No registers.` Add the safe-path line from setup step 5. GDB 15.1 suggests `~/.config/gdb/gdbinit`, and `~/.gdbinit` also works.
- **`si` on `mret` or `sret` runs away.** With QEMU 8.2.2 and GDB 15.1, a single step over either instruction did not stop; the machine ran on until the next breakpoint. Set a breakpoint on the target first (`b *$mepc` or `b *$sepc`), then `si`.
- **`ld: warning: kernel/kernel has a LOAD segment with RWX permissions`.** binutils 2.42 prints this on every kernel build. It is a warning, and the build continues.
- **`Connection refused` or a hang on `target remote`.** Start `make qemu-gdb` first, and run GDB in the same directory so that `.gdbinit` has your port. `make print-gdbport` prints it.
- **Plain `gdb` instead of `gdb-multiarch`.** Ubuntu's default GDB may not include RISC-V. Use `gdb-multiarch`.
- **QEMU does not respond to Ctrl-C.** Quit with Ctrl-a, then x. Under `qemu-gdb`, QEMU waits for GDB's `c` before it runs anything.
- **A breakpoint fires three times.** Each hart hits it. Use `CPUS=1` while you learn.
- **Slow builds or odd script errors under `/mnt/c`.** Clone inside WSL's own file system. A clone made with Windows git and `core.autocrlf=true` has CRLF line endings, which can break `usys.pl` and shell scripts.
- **Stale objects after switching branches.** MIT's lab steps run `make clean` after `git checkout`. Do the same.

## Where this shows up later

- Stage 01 uses the same GDB setup to stop on the trap path, where the privilege mode changes on every system call.
- Stage 03 reads the page table you saw switched on here; QEMU's monitor (Ctrl-a c, then `info mem`) prints it.
- Stage 04 follows the timer interrupt that `timerinit()` requests here.

## Next

[Lab 01](../01-system-calls/README.md) follows one system call from `ecall` through the trampoline into the kernel and back, then does the same on Linux and Windows.
