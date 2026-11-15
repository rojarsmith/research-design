# Lab 00: Workbench, booting and debugging xv6

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 00](../../SYLLABUS.md#00-workbench-booting-and-debugging-xv6)

> **Draft for review, 2026-11-15. Nothing in this lab was executed on the machine it was written on.** That Windows 11 machine has no Ubuntu WSL distribution, no RISC-V toolchain and no QEMU, and none were installed for this lab. The commands come from MIT 6.1810's [tools page](https://pdos.csail.mit.edu/6.1810/2025/tools.html), the [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) source and QEMU's source, all read on 2026-11-15. Output marked *expected* is what that source says should happen. None of it is observed output. Write down what you actually see.

## Goal

Build a working xv6 workbench and use it to answer one question exactly: what runs between QEMU's reset and xv6's `main()`, and in which privilege mode? When you finish you can boot xv6, quit it, rebuild it, attach GDB, and stop at any instruction from the reset vector to the shell prompt.

```
QEMU reset ROM   ->  _entry        ->  start()      --mret-->  main()       ->  scheduler()  ->  /init, sh
0x1000, M-mode       0x80000000        start.c                 main.c           proc.c           U-mode
                     entry.S, M-mode   M-mode                  S-mode           S-mode
```

## Prerequisites

C with pointers and structs, a Linux shell, git and make, and reading RISC-V assembly at the level of stage 1 of the [`c-compiler-foundations` path](../../../c-compiler-foundations/research/04-integration-path.zh-TW.md): you can read a `-O0` prologue, find `ra`, and explain negative offsets from `fp`. xv6 is RV64 (`-march=rv64gc`), so registers and pointers are 64 bits wide, and you will meet CSRs and privilege modes, which that path does not cover.

## Setup (not executed here)

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

Start `NOTES.md` now. Write down the Windows build, the WSL version, the distribution, every version above and the xv6 commit.

## Read first

All of these are short.

- `Makefile`: `QEMUOPTS`, `CPUS := 3`, the `qemu` and `qemu-gdb` targets, `GDBPORT` (computed from your user id), and the rule that generates `.gdbinit` from `.gdbinit.tmpl-riscv`.
- `kernel/kernel.ld`: the kernel is linked at `0x80000000`, `kernel/entry.o(_entry)` goes first, and the trampoline gets a page of its own.
- `kernel/entry.S`: each hart computes `sp = stack0 + (hartid + 1) * 4096` and calls `start`.
- `kernel/start.c`: sets the previous privilege in `mstatus` to supervisor and `mepc` to `main`, turns paging off, delegates traps to supervisor mode, opens physical memory protection (PMP), asks for timer interrupts, stores the hart id in `tp`, then executes `mret`.
- `kernel/main.c`: hart 0 initializes everything, and the other harts wait for `started`.
- The [xv6 book](https://pdos.csail.mit.edu/6.1810/2025/xv6/book-riscv-rev5.pdf), chapter 2, the section on starting xv6.

## Exercises

Before every exercise, write your prediction in `NOTES.md`. Then run it and write what happened next to the prediction. A wrong prediction you can explain counts for more than a right guess.

### A. Boot, look, quit

**Predict:** how many `hart N starting` lines appear, and in which order? Which process names will Ctrl-p list?

```sh
make qemu
```

*Expected, from `kernel/main.c` and MIT's util lab page:* `xv6 kernel is booting`, then one `hart N starting` line for each hart other than 0, then `init: starting sh` and a `$` prompt. With the default `CPUS := 3` there are lines for harts 1 and 2. They race, so the order can change between boots. Type `ls`, then press Ctrl-p. `procdump()` prints one line per process as pid, state and name, so expect lines for `init` and `sh`. Quit QEMU with Ctrl-a, then x.

Boot again with `make CPUS=1 qemu` and check your prediction for one hart.

### B. Every QEMU flag

`make -n qemu` prints the command without running it. *Expected, from the Makefile:*

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

*Expected, from QEMU's source (`hw/riscv/virt.c` maps a small ROM at `0x1000`, and `riscv_setup_rom_reset_vec()` in `hw/riscv/boot.c` writes six instructions into it):* `$pc` is `0x1000`, and the six instructions are `auipc t0`, `addi a2`, `csrr a0, mhartid`, `ld a1, 32(t0)`, `ld t0, 24(t0)` and `jr t0`. They put the hart id in `a0` and the device-tree address in `a1`, then jump to the address stored at `0x1018`. With `-bios none` that address is the start of RAM, `0x80000000`. QEMU's gdb stub also exposes the current privilege level as a virtual register, `priv` (`target/riscv/gdbstub.c`), with 3 meaning machine mode, 1 supervisor and 0 user. If your GDB does not show `$priv`, use `p/x $mstatus` and the reasoning in exercise E instead.

Single-step with `si` until `$pc` is `0x80000000`.

### D. From `_entry` to `start()`

```
(gdb) x/8i 0x80000000
(gdb) si          (repeat, and watch $sp, $a0, $a1)
(gdb) p/x $sp
(gdb) p &stack0
```

**Predict:** the value of `sp` just before `call start`, for hart 0. Check it against `&stack0`. Why `hartid + 1` and not `hartid`? Which way does the stack grow?

The first xv6 instruction is `la sp, stack0` at `_entry`. `la` is a pseudo-instruction, so `x/i` shows the real instructions it became. Write down what they are.

Rerun with the default three harts: `make qemu-gdb`, then `b *0x80000000` and `c` several times. **Predict** how many times the breakpoint is hit, then check with `info threads`. QEMU presents each hart to GDB as a thread.

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
- Step to the `mret` itself with `si` and `x/i $pc`. Print `$priv` and `$pc`, execute one `si`, and print both again. That one instruction is where xv6 leaves machine mode. Copy the GDB output into `NOTES.md`; checkpoint 3 asks for it.

**Break it:** comment out the two PMP lines (`w_pmpaddr0`, `w_pmpcfg0`), rebuild, and step over the `mret` again. Predict first. Then observe `$pc`, `$priv`, `$scause` and `$mcause`. Restore the lines afterwards.

### F. `main()` and the first page table

```
(gdb) b main
(gdb) c
(gdb) bt
(gdb) p $priv
(gdb) b kvminithart
(gdb) c
(gdb) p/x $satp
(gdb) finish
(gdb) p/x $satp
```

**Predict:** what will `bt` print at `main`? Nothing called `main`. `mret` loaded `pc` from `mepc` and left `ra` holding whatever the last `call` inside `start()` put there. GDB builds the backtrace from that `ra`, so read its output with suspicion. After `kvminithart()`, the top four bits of `satp` should be 8 (`SATP_SV39`), and the rest is the physical page number of the kernel page table. **Predict:** do kernel addresses in GDB change once paging is on? Hint: read `kvmmake()` in `kernel/vm.c` and look at how it maps kernel text and RAM.

Continue with `c` until the shell prompt appears in terminal 1.

### G. The linker script, from the outside

```sh
riscv64-linux-gnu-readelf -h kernel/kernel | grep Entry
riscv64-linux-gnu-objdump -h kernel/kernel
grep -E ' (_entry|start|main|etext|_trampoline|end)$' kernel/kernel.sym
```

**Predict** the entry point and the first address of `.text` before you run these. Check that `_trampoline` is page aligned. Then answer: QEMU jumped to `0x80000000` because that is the start of RAM, not because it read the ELF entry point. What would happen if `kernel.ld` placed a different object file first?

## Checkpoints

Answer these in `NOTES.md`, with the GDB output that supports each answer.

1. Explain every QEMU flag from exercise B. For each one, name the xv6 constant or code that depends on it.
2. Show the first instruction the CPU executes (in QEMU's ROM at `0x1000`) and the first instruction xv6 executes (`_entry` at `0x80000000`), as GDB printed them.
3. Show the instruction where xv6 leaves machine mode, with `$priv` and `$pc` before and after it.
4. Compare with reset to `main` on a microcontroller (stage 1 of [`mcu-foundations`](../../../mcu-foundations/02-learning-path.md)). What extra work exists because this machine has privilege levels? List each item in `start()`: privilege setup, trap delegation, PMP, timer access, `tp`. Then go the other way: an MCU startup file copies `.data` and zeroes `.bss`, and `entry.S` does neither. Who did that work for xv6?
5. Copy the `bt` output at `main`. If it lists `start` as the caller, explain why that is misleading: which instruction actually transferred control, and where did the `ra` value that GDB used come from?

## Common failure modes

- **WSL 1.** MIT says the labs do not work under WSL 1. Check with `wsl -l -v`, and convert with `wsl --set-version <distro> 2`.
- **`Couldn't find a riscv64 version of GCC/binutils`.** The Makefile tries several tool prefixes by running each `objdump`. Install the packages from step 2, or run `make TOOLPREFIX=riscv64-linux-gnu-`.
- **`ERROR: Need qemu version >= 7.2`.** The distribution's QEMU is too old. MIT notes that you will probably need Ubuntu 24 or later.
- **A shell error from `check-qemu-version`.** That rule compares versions with `bc`. If `bc` is missing, install it with `sudo apt-get install bc`. (MIT's Arch package list includes `bc`; the Ubuntu list does not.)
- **`auto-loading has been declined`.** GDB ignored `.gdbinit`, so it is not connected and has no symbols. Add the safe-path line from setup step 5.
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
