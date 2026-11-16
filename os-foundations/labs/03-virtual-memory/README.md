# Lab 03: Virtual memory

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 03](../../SYLLABUS.md#03-virtual-memory)

> **Draft for review, 2026-11-16. What was and was not executed:**
> - **xv6 parts (exercises A, B, C and F): built and run** in an Ubuntu 24.04 container under podman on Windows 11. The podman machine is a WSL 2 VM, so this was not WSL 2 Ubuntu itself, and any timing from it is a VM number. Tools in the container: QEMU 8.2.2, riscv64-linux-gnu-gcc 13.3.0, gdb-multiarch 15.1. The tree was xv6-riscv at commit `fa2ddcb7` (pinned below). [`xv6/run-xv6.sh`](xv6/run-xv6.sh) applied the reference solutions (not shipped) to a copy of the tree and drove QEMU and GDB with expect. Its full output is in [`xv6/example-output.txt`](xv6/example-output.txt). `usertests -q` passed on the copy-on-write kernel.
> - **Linux parts (exercises D and E): built and run** in the same container, on WSL's kernel 6.6.87.2. Output: [`linux/example-output.txt`](linux/example-output.txt).
> - **Windows part (exercise G): built and run** on the Windows 11 host with gcc 16.2 from WinLibs. Output: [`windows/example-output.txt`](windows/example-output.txt).
> - **Not executed:** typing the GDB commands by hand (the same commands ran from scripts in batch mode), and the "break it" steps marked *not run here*.

## Goal

Read a page table, change one, and watch it work. In xv6 you print a process's page table, then replace `fork`'s full copy with copy-on-write, and count the pages each version copies. On Linux you map one file into two processes, shared and private, and shrink it under a live mapping until `SIGBUS` arrives. On Windows you take memory through its three states (free, reserved, committed) and share a section between two processes. At the end you translate one virtual address by hand and confirm every step in GDB.

```
Sv39 virtual address (bits 63..39 must copy bit 38; xv6 uses only addresses below MAXVA = 2^38)
  | L2 index: bits 38..30 | L1 index: bits 29..21 | L0 index: bits 20..12 | offset: bits 11..0 |

satp.PPN ─► root page ─[L2]─► PTE ─► L1 page ─[L1]─► PTE ─► L0 page ─[L0]─► PTE ─► data page + offset
            512 PTEs of 8 bytes each; PTE bits 53..10 hold the next physical page number,
            bits 9..8 (RSW) are free for software, bits 7..0 are D A G U X W R V
```

## The pinned xv6 commit

Everything in this lab was written and run against **xv6-riscv commit `fa2ddcb7ca87a77af579380a6eafd5aff163b44f`** (2026-11-05, "usertests: bound the zero-length pipe read regression test"). Check it out in your Lab 00 clone, so that your tree matches the one the recorded output came from:

```sh
cd ~/xv6-riscv
git fetch
git checkout fa2ddcb7ca87a77af579380a6eafd5aff163b44f
make clean
```

In this tree `fork()` is `kfork()`, the kernel prints with `printk`, and `sleep` takes no arguments: callers use `sleep_prepare(chan)`, release their lock, then `sleep()`. `copyin`, `copyout` and `vmfault` take the process size as a second argument. `sbrk()` allocates at once, and `sbrklazy()` only moves the end of the process: `vmfault()` maps each page on first touch. MIT's lab branches (`git checkout pgtbl`, `cow` in `xv6-labs-2025`) start from different code, so everything here is for plain xv6-riscv only.

## Files

| File | What it is | Status |
| --- | --- | --- |
| [`xv6/vmprinttest.c`](xv6/vmprinttest.c) | prints where its text, data, stack and heap are, then its page table, or a forked child's with `fork` | built and run here |
| [`xv6/cowtest.c`](xv6/cowtest.c) | counts the pages one `fork` copies and shares, forks a process that holds 2/3 of free memory, and makes the kernel write into a copy-on-write page | built and run here |
| [`xv6/translate.gdb`](xv6/translate.gdb) | walks the page table in GDB to confirm a hand translation | run here, in batch mode |
| [`xv6/run-xv6.sh`](xv6/run-xv6.sh), [`xv6run.exp`](xv6/xv6run.exp), [`xv6gdb.exp`](xv6/xv6gdb.exp) | apply your patches to a copy of the tree, build, and drive QEMU and GDB with expect | run here, with the reference solutions |
| [`linux/mapshare.c`](linux/mapshare.c) | one file mapped by two processes, `MAP_SHARED` and `MAP_PRIVATE`, with `/proc/self/maps` | built and run here |
| [`linux/sigbus.c`](linux/sigbus.c) | truncate under a mapping, a server killed by a client, and a sealed memfd | built and run here |
| [`linux/forkcow.c`](linux/forkcow.c) | Linux's copy-on-write `fork` seen through minor-fault counts, and `fork` cost by size | built and run here |
| [`linux/run-linux.sh`](linux/run-linux.sh) | builds and runs the three Linux programs | run here |
| [`windows/nt_reserve_commit.c`](windows/nt_reserve_commit.c) | reserve, commit, touch, decommit, release; commit on demand; the address space by allocation | built and run here |
| [`windows/nt_section.c`](windows/nt_section.c) | a page-file-backed section shared by two processes, a copy-on-write view, `SEC_COMMIT` vs `SEC_RESERVE`, shrinking a mapped file | built and run here |
| `*/example-output.txt` | recorded output of everything above | recorded 2026-11-16 |

Reference solutions (not shipped): the xv6 output recorded here came from reference patches for exercises A, B and C, which are kept out of the repository. They solve the vmprint and cow assignments of MIT 6.1810. Write your own. To run `run-xv6.sh` on your work, save it as patches against the pinned commit, for example with `git diff` in your clone: `0001-vmprint-vmstat.patch` for exercises A and B, with both programs added to `UPROGS`, and `0002-cow.patch` for exercise C, on top of 0001. Put them in `xv6/solution/`, which git ignores, or point `SOLUTION_DIR` at them. A step whose patches are missing is skipped with a message.

## Setup

- The Lab 00 workbench at the pinned commit. Use one hart (`make CPUS=1 qemu`) while you learn, so that the counts and GDB sessions are repeatable.
- MIT's matching labs are [pgtbl](https://pdos.csail.mit.edu/6.1810/2025/labs/pgtbl.html) and [cow](https://pdos.csail.mit.edu/6.1810/2025/labs/cow.html). The pgtbl lab has its own `vmprint` assignment with a `kpgtbl()` system call, a page shared with the kernel for `ugetpid()`, and superpages. Do them as extra practice; this lab does not need them.
- To run every xv6 part without typing, as was done here: `sudo apt-get install expect patch`, then `SOLUTION_DIR=<your patches> sh xv6/run-xv6.sh ~/xv6-riscv` (the default is `xv6/solution/`). The script never touches your checkout. It copies it to `/tmp/lab03-xv6/` and builds there. `BREAKIT=1` adds the reference-count break, and `USERTESTS=1` adds `usertests -q`. The whole script, `usertests` included, took about three minutes here.
- **Linux:** WSL 2 Ubuntu with gcc, then `sh linux/run-linux.sh`. It builds in `/tmp/lab03-linux/`.
- **Windows:** any x86-64 MinGW-w64 gcc.

## Read first

- The xv6 book, chapter 3 (page tables) and the page-fault section of chapter 4. MIT gives chapter 3 for pgtbl and chapter 4 for cow.
- `kernel/riscv.h`: the `PTE_*` bits, `PX`, `PTE2PA`, `PA2PTE`, `MAKE_SATP`.
- `kernel/vm.c`: `walk`, `mappages`, `uvmcopy`, `freewalk`, `copyout`, `vmfault`.
- `kernel/kalloc.c`, `kernel/memlayout.h`, and in `kernel/exec.c` the part that builds the stack and its guard page.
- `kernel/trap.c`: `usertrap()`, which calls `vmfault()` on page faults with `scause` 13 (load) and 15 (store).

## Exercises

As before, write each prediction in `NOTES.md` before you run anything.

### A. Print a page table (xv6)

Write `void vmprint(pagetable_t)` in `kernel/vm.c`. Walk the three levels as `freewalk()` does: a valid PTE with none of `R`, `W`, `X` set points to the next level. Print one line per valid PTE: the virtual address it starts at, the PTE and the physical address, indented by level, plus the flag letters. Then add a system call `vmprint()` that prints the caller's own table, through the same five places as Lab 01's `trapinfo`. Copy [`vmprinttest.c`](xv6/vmprinttest.c) to `user/` and add `$U/_vmprinttest` to `UPROGS`. The output below came from the reference solution (not shipped).

**Predict before running `vmprinttest`:** How many leaf PTEs? Which pages have `U` clear, and why? Which virtual pages below `sz` have no PTE at all? Which flags will the hardware have set? Why is the trampoline's physical address inside the kernel's text?

Observed here (stock kernel, one hart):

```
text   main    0x0000000000000000
data   marker  0x0000000000001000
stack  local   0x0000000000004F9C
heap   eager   0x0000000000005000  (1 page from sbrk)
heap   lazy    0x0000000000006000  (3 pages from sbrklazy, only the 2nd touched)
size   sbrk(0) 0x0000000000009000
page table 0x0000000087f25000
 ..0x0000000000000000: pte 0x0000000021fc8401 pa 0x0000000087f21000 V--------
 .. ..0x0000000000000000: pte 0x0000000021fc8001 pa 0x0000000087f20000 V--------
 .. .. ..0x0000000000000000: pte 0x0000000021fc885b pa 0x0000000087f22000 VR-XU-A--
 .. .. ..0x0000000000001000: pte 0x0000000021fc7c17 pa 0x0000000087f1f000 VRW-U----
 .. .. ..0x0000000000002000: pte 0x0000000021fc7817 pa 0x0000000087f1e000 VRW-U----
 .. .. ..0x0000000000003000: pte 0x0000000021fc7407 pa 0x0000000087f1d000 VRW------
 .. .. ..0x0000000000004000: pte 0x0000000021fc70d7 pa 0x0000000087f1c000 VRW-U-AD-
 .. .. ..0x0000000000005000: pte 0x0000000021fc98d7 pa 0x0000000087f26000 VRW-U-AD-
 .. .. ..0x0000000000007000: pte 0x0000000021fd50d7 pa 0x0000000087f54000 VRW-U-AD-
 ..0x0000003fc0000000: pte 0x0000000021fc9001 pa 0x0000000087f24000 V--------
 .. ..0x0000003fffe00000: pte 0x0000000021fc8c01 pa 0x0000000087f23000 V--------
 .. .. ..0x0000003fffffe000: pte 0x0000000021fd08c7 pa 0x0000000087f42000 VRW---AD-
 .. .. ..0x0000003ffffff000: pte 0x000000002000184b pa 0x0000000080006000 VR-X--A--
```

The letters are `V R W X U G A D`, then `C` for RSW bit 8, which exercise C uses. Explain every line:

- **0x0, text: `VR-XU`.** `exec` gives a segment `X` or `W` from its ELF flags (`flags2perm`), so text is never writable.
- **0x1000 and 0x2000, data and bss: `VRW-U`, and no `A`.** `marker` is one aligned page on its own. Neither page has been touched by the program yet. `exec` filled them through their physical addresses, not through this page table, so no hardware walk has set `A`.
- **0x3000, the guard page: `U` clear.** `exec` allocates `USERSTACK + 1` pages and calls `uvmclear()` on the lowest one. A stack overflow faults there instead of running into the data.
- **0x4000, the stack: `A` and `D` set.** `start()` in `kernel/start.c` sets `MENVCFG_ADUE`, so on this QEMU the hardware itself sets `A` on access and `D` on write. Without that bit, the RISC-V privileged spec lets an implementation raise a page fault instead and leave the bits to software.
- **0x5000 and 0x7000, the heap.** `sbrk(4096)` mapped 0x5000 at once. `sbrklazy(3 * 4096)` only raised `sz` to 0x9000, and the write to `lazy[4096]` took a page fault that `vmfault()` turned into the page at 0x7000. 0x6000 and 0x8000 are below `sz`, but no page is mapped there.
- **0x3fffffe000, the trap frame, and 0x3ffffff000, the trampoline: `U` clear.** Only supervisor code uses them. The trampoline's physical page `0x80006000` is in the kernel image, because `trampoline.S` is linked into the kernel, and the same physical page is mapped into every process and into the kernel's own table.
- **Five page-table pages:** the root, two L1 pages and two L0 pages, one pair for the bottom of the address space and one for the top. Exercise B needs this number.

`vmprinttest fork` prints a forked child's table instead. With the stock kernel, the child's table has the same shape with different physical addresses: every page is a copy.

**Break it (not run here):** make `vmprint` recurse into every valid PTE, leaves included. Predict what it prints for the text page and whether the kernel survives.

### B. Count what `fork` copies (xv6)

Now add counters. Create `kernel/vmstat.h` with the struct shown at the top of [`cowtest.c`](xv6/cowtest.c). Count each page that `uvmcopy()` copies (`fork_copied`), keep a count of the pages on `kalloc()`'s free list, and add a system call `vmstat(struct vmstat *)` that fills the struct and copies it out with `copyout`. Several harts may run `uvmcopy()` at once, so count with `__atomic_fetch_add` or under a lock. The other three counters stay 0 until exercise C. Copy `cowtest.c` to `user/` and add `$U/_cowtest` to `UPROGS`. **Predict** for `cowtest` part 1: the process has 71 pages below `sz`. How many does `fork` copy? How many free pages does one `fork` use? Which kernel allocations in `kfork()` and `allocproc()` besides the copies take pages? The kernel stack does not: `proc_mapstacks()` gave every process slot one at boot.

Observed here (stock kernel, with the reference solution, not shipped):

```
  process size 71 pages
  the fork itself     : fork copied 71, fork shared 0, cow copied 0, cow reused 0
  free pages: 32465 before the fork, 32388 in the child just after it
  parent, after wait  : fork copied 71, fork shared 0, cow copied 0, cow reused 0
  free pages: 32465 before the fork, 32465 after the child exited: ok
2. fork a process that holds 2/3 of the free memory (21686 of 32529 pages)
  fork failed: a full copy needs more pages than are free
```

The fork used 77 pages: 71 copies, the child's five page-table pages and its trap-frame page. Part 2 fails because `uvmcopy()` runs out of pages partway through, frees what it copied, and `fork` returns -1. Compare Lab 02's `uvmcopy` page count with this one.

### C. Copy-on-write `fork` (xv6)

MIT's [cow lab](https://pdos.csail.mit.edu/6.1810/2025/labs/cow.html) states the task: `fork` shares the parent's pages instead of copying them, and a page is copied only when one side writes to it. Build it in this order:

1. **Mark the page.** Use an RSW bit, which the privileged spec leaves to supervisor software: `#define PTE_COW (1L << 8)` in `kernel/riscv.h`.
2. **Count the owners.** In `kalloc.c`, keep a count for each physical page, indexed by `(pa - KERNBASE) / PGSIZE` and protected by `kmem.lock`. `kalloc()` sets it to 1, a new `krefinc()` adds one, and `kfree()` frees the page only when the count drops to zero. `kinit()` frees every page once while all counts are still 0, so 0 must mean free.
3. **Share instead of copy.** In `uvmcopy()`, map the parent's physical page into the child. If the page was writable, clear `PTE_W` and set `PTE_COW` in both PTEs. Leave read-only pages, like text, read-only and shared. Count the new owner, then flush the TLB with `sfence_vma()`, because the parent's PTEs changed.
4. **Take the write fault.** In `usertrap()`, before the `vmfault()` case, handle `scause` 15 on a `PTE_COW` page. `vmfault()` returns 0 for any page that is already mapped, so if your test comes after it, the process is killed.
5. **Copy or take back.** In `cowfault(pagetable, va)`: if the count is 1, the other sharers are gone, so just set `PTE_W` and clear `PTE_COW`. Otherwise allocate a page, copy, map it writable, and drop one reference to the old page. Flush the TLB.
6. **The kernel's own writes.** `copyout()` writes user memory through physical addresses, so the hardware never faults on a COW page there. Make `copyout()` call `cowfault()` when it meets `PTE_COW`, and still refuse pages that are simply read-only.

In your copy-on-write code, also count `fork_shared` in `uvmcopy()`, and `cow_copied` and `cow_reused` in `cowfault()`. The numbers below came from the reference solution (not shipped). **Predict** each row of the table below before you run `cowtest` again, then run `usertests -q`.

| `cowtest`, observed here | stock `fork` | copy-on-write `fork` |
| --- | ---: | ---: |
| pages copied by `fork` | 71 | 0 |
| pages shared by `fork` | 0 | 71 |
| free pages one `fork` used (before the fork, then in the child) | 77 | 7 |
| copies while the child read 64 heap pages | 0 | 0 |
| copies when the child wrote 4 heap pages | 0 | 4 |
| copies and reuses during the whole of part 1 | 0 | 5 copied, 1 reused |
| free pages after the child exited, compared with before | equal | equal |
| `fork` of a process holding 2/3 of free memory | fails | succeeds |
| `pipe()` into a COW page in the child; the parent's copy unchanged | ok | ok |
| `usertests -q` with 3 harts | not run | `ALL TESTS PASSED` |

Explain the numbers that are not 0 or 71:

- **7 pages for the fork:** five page-table pages, the trap frame, and one copy. The "fork itself" row already shows that copy. The counts fit the child's first store, the function prologue of `snap()` on its stack page, which is shared at that moment. Check it yourself: break on `cowfault` and print `va`.
- **5 copies and 1 reuse:** the child's stack page and its 4 heap pages were copied. After the child exited, the parent's stack page had one owner left, so the parent's next write to it (`wait()`'s `copyout` of the status) only got `PTE_W` back.

`vmprinttest fork` on the copy-on-write kernel prints the child's table just after the fork:

```
 .. .. ..0x0000000000000000: pte 0x000000002174a85b pa 0x0000000085d2a000 VR-XU-A--
 .. .. ..0x0000000000001000: pte 0x00000000218cb513 pa 0x000000008632d000 VR--U---C
 .. .. ..0x0000000000002000: pte 0x000000002194b913 pa 0x000000008652e000 VR--U---C
 .. .. ..0x0000000000003000: pte 0x00000000219cbd03 pa 0x000000008672f000 VR------C
 .. .. ..0x0000000000004000: pte 0x00000000213c8cd7 pa 0x0000000084f23000 VRW-U-AD-
 .. .. ..0x0000000000005000: pte 0x00000000215499d3 pa 0x0000000085526000 VR--U-ADC
 .. .. ..0x0000000000007000: pte 0x00000000214c95d3 pa 0x0000000085325000 VR--U-ADC
```

Data, guard and heap pages are read-only with `C` set. Text has no `C`, because it was never writable. The stack page is already the child's own writable copy: the child wrote to its stack on the way into `vmprint()`.

**Break it (run here):** delete the reference-count increment in `uvmcopy()`. `BREAKIT=1` does it for you if your patch has the reference solution's line `    krefinc((void *)pa);`; otherwise it says so and skips. **Predict** first. Observed: the counts said "cow reused" instead of "cow copied", so the child wrote straight into pages that the parent still mapped. When the child exited, those pages went back to the free list, filled with junk. Then the processes that shared them died one by one: `cowtest` itself (pid 3) with `scause 0x2` (illegal instruction) at `sepc=0x1000`, the shell (pid 2) with `scause 0x2`, init (pid 1) with `scause 0xc` (instruction page fault), and the kernel stopped with `panic: init exiting`.

**Break it (not run here):**
- Remove the `copyout()` change. Predict what part 3 of `cowtest` prints.
- Remove both `sfence_vma()` calls. Predict whether anything breaks on xv6. Hint: read `userret` in `trampoline.S`, which runs before every return to user space. Then explain why a kernel that did not flush there would need these calls.

### D. Shared and private mappings on Linux (run here)

```sh
cd linux
gcc -O2 -Wall -o mapshare mapshare.c
./mapshare
```

The parent (A) and the child (B) each open and `mmap` the file after the fork, as two unrelated processes would. **Predict** each "map" and "file" column before running. In round 2, B maps the file `MAP_PRIVATE`. What will B see after A writes page 0? After B writes page 0? After A writes page 1, which B never wrote?

Observed here (columns: page 0 and page 1 as this process sees them through its mapping, then as `pread` sees the file):

```
Round 1: A and B both map mapshare.dat with MAP_SHARED
  A /proc/self/maps: 7b04543c7000-7b04543c9000 rw-s 00000000 08:30 63677   /tmp/lab03-linux/mapshare.dat
  B after B writes page 1   map: [A wrote page 0  ] [B wrote page 1  ]   file: [A wrote page 0  ] [B wrote page 1  ]
  A after B writes page 1   map: [A wrote page 0  ] [B wrote page 1  ]   file: [A wrote page 0  ] [B wrote page 1  ]

Round 2: A maps with MAP_SHARED, B with MAP_PRIVATE
  B /proc/self/maps: 7b04543c7000-7b04543c9000 rw-p 00000000 08:30 63677   /tmp/lab03-linux/mapshare.dat
  B after A writes page 0   map: [A wrote page 0  ] [page 1: original]   file: [A wrote page 0  ] [page 1: original]
  B after B writes page 0   map: [B private write ] [page 1: original]   file: [A wrote page 0  ] [page 1: original]
  A after B's private write map: [A wrote page 0  ] [page 1: original]   file: [A wrote page 0  ] [page 1: original]
  B after A writes both     map: [B private write ] [A wrote page 1  ]   file: [A wrote 0 again ] [A wrote page 1  ]
```

(Shortened; the full lines are in the example output.) `MAP_SHARED` pages are the file's page-cache pages, so both mappings and `read(2)` see every write at once, with no `msync`. A private mapping starts out mapping the same pages. B's first write to page 0 gave B its own copy, so B never saw A's later writes to page 0, while page 1, which B never wrote, still showed A's write. [mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html) says it is unspecified whether a `MAP_PRIVATE` mapping sees changes made to the file after the `mmap()` call, so this is Linux's behavior on this kernel, not a promise. In `/proc/self/maps` the fourth letter of the permissions is `s` for shared and `p` for private ([proc_pid_maps(5)](https://man7.org/linux/man-pages/man5/proc_pid_maps.5.html)).

Then run `./forkcow`. Observed here:

```
1. fork with 16384 written pages (64 MiB) in the parent
  child: minor faults 10 at start, +0 after reading 16384 pages, +1000 after writing 1000 pages
2. fork + _exit + wait, by size of the parent's written memory
     1 MiB: best of 20 runs    167.7 us
    64 MiB: best of 20 runs   1643.6 us
   256 MiB: best of 20 runs   5827.9 us
```

Reading the shared pages costs no faults. Each first write costs one minor fault, which is the copy, exactly as in your xv6 version. `fork` still gets slower as the parent grows, because [fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html) says its cost is the time and memory to duplicate the parent's page tables and to create the child's task structure. These are VM numbers; compare them only with each other.

### E. Truncate under a live mapping on Linux (run here)

```sh
gcc -O2 -Wall -o sigbus sigbus.c
./sigbus
```

**Predict** each read in part 1 after the file shrinks from 3 pages to 1 page plus 100 bytes. Observed here:

```
  read page 0, offset 0             -> 0x61
  read page 1, offset 50            -> 0x62
  read page 1, offset 200 (past EOF) -> 0x00  (zero)
  read page 2, offset 0             -> SIGBUS, si_code 2 (BUS_ADRERR), si_addr = map + 0x2000
```

[mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html) lists `SIGBUS` for access to a page of the buffer that lies beyond the end of the mapped file. It also says the rest of a partial last page is zeroed. So the bytes after the new end of file but inside its last page read as 0, and the first whole page beyond it gets the signal.

Part 2 is the case the checkpoint asks about. A server maps a file it got from a client and reads it. Halfway through, the client truncates the file:

```
  a) server without a SIGBUS handler
  client: truncated the shared file to one page
  server was killed by signal 7 (Bus error)
  b) server with a SIGBUS handler
  client: truncated the shared file to one page
  server: SIGBUS at buffer + 0x1000; rejecting this client's buffer and carrying on
```

Part 3 shows the fix that stage 08 builds: a `memfd` sealed with `F_SEAL_SHRINK` refuses `ftruncate` with `EPERM` ([memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html), and "File Sealing" in [fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html)).

### F. Translate an address by hand, then confirm it in GDB (run here)

Take `local`, the stack variable that `vmprinttest` printed, at `0x4F9C`, and translate it on paper using only the `vmprint` output above:

1. Split the address. L2 index = `(va >> 30) & 0x1ff` = 0, L1 index = `(va >> 21) & 0x1ff` = 0, L0 index = `(va >> 12) & 0x1ff` = 4, offset = `va & 0xfff` = `0xF9C`.
2. Entry 0 of the root (at `0x87f25000`) is `0x21fc8401`. `R`, `W` and `X` are clear, so it points to a table: physical page number `0x21fc8401 >> 10` = `0x87f21`, so the L1 page is at `0x87f21000`.
3. Entry 0 there is `0x21fc8001`, so the L0 page is at `0x87f20000`.
4. Entry 4 there is `0x21fc70d7`. Its flags are `0xd7` (`V R W U A D`), so it is a leaf the user may write. Physical page `0x87f1c`, physical address `0x87f1cf9c`.

Now do the same for an address of your own choosing, including one in the top half, and confirm it in GDB. [`translate.gdb`](xv6/translate.gdb) stops in `sys_vmprint`, reads the process's root page from `cpus[0].proc->pagetable`, and walks the table by physical address. It does this for `marker`, where the bytes at the end must be the marker string, and for `TRAPFRAME`, where the result must equal the kernel's own pointer `p->trapframe`. Observed here:

```
process vmprinttest, pid 3
va of marker       0x0000000000001000
root page table    0x0000000087f25000  (its satp value: 0x8000000000087f25)
satp right now     0x8000000000087fff  (the kernel's page table)
indexes            L2 0, L1 0, L0 1, offset 0x0
L2 PTE at 0x87f25000  0x0000000021fc8401  flags 0x001
L1 PTE at 0x87f21000  0x0000000021fc8001  flags 0x001
L0 PTE at 0x87f20008  0x0000000021fc7c17  flags 0x017
physical address   0x0000000087f1f000
string at that physical address:
0x87f1f000:	"lab03 marker: translate my address by hand"

va of TRAPFRAME    0x0000003fffffe000  indexes L2 255, L1 511, L0 510
PTEs               0x0000000021fc9001  0x0000000021fc8c01  0x0000000021fd08c7
leaf flags 0x0c7: U is clear
physical address   0x0000000087f42000
p->trapframe       0x0000000087f42000
```

GDB can read those physical addresses because the breakpoint is in the kernel, and `kvmmake()` maps all of RAM at its own physical address in the kernel's page table. Through the user's table, as the hardware would walk it, GDB could not even read the first page-table page.

### G. Reserve, commit and sections on Windows (run here)

```powershell
cd windows
gcc -O2 -Wall -o build\nt_reserve_commit.exe nt_reserve_commit.c -lpsapi
build\nt_reserve_commit.exe
gcc -O2 -Wall -municode -o build\nt_section.exe nt_section.c -lpsapi
build\nt_section.exe
```

**Predict** each column for each step of part 1, then run. Observed on 2026-11-16 (Windows build 26300, Intel Core i7-11700, 64 GiB RAM, hypervisor present; [full output](windows/example-output.txt)):

| Step | Process commit charge | System commit total | Working set | Page faults | `VirtualQuery` |
| --- | ---: | ---: | ---: | ---: | --- |
| reserve 1 GiB | +0.0 MiB | +0.0 MiB | +0.1 MiB | +14 | `MEM_RESERVE` |
| commit the first 256 MiB | +256.5 MiB | +256.6 MiB | +0.1 MiB | +147 | `MEM_COMMIT` |
| write one byte in each page | +256.5 MiB | +258.1 MiB | +256.1 MiB | +65,683 | `MEM_COMMIT` |
| decommit the 256 MiB | +0.0 MiB | +1.6 MiB | +0.1 MiB | +65,684 | `MEM_RESERVE` |
| release the region | +0.0 MiB | +1.6 MiB | +0.1 MiB | +65,684 | `MEM_FREE` |

- **Reserving costs nothing but address space.** [VirtualAlloc](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc) says `MEM_RESERVE` reserves a range "without allocating any actual physical storage in memory or in the paging file on disk".
- **Committing charges, touching allocates.** `MEM_COMMIT` takes memory charges from the overall size of memory and the paging files, and Learn says actual physical pages are not allocated until the addresses are accessed. Here the commit charge jumped at commit time, and the working set and the page-fault count jumped only when each page was written: 65,536 faults for 65,536 pages. `PrivateUsage` is the process's commit charge ([PROCESS_MEMORY_COUNTERS_EX](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex)). The system total also moves with every other process.
- **Commit on demand.** Part 2 reserves 64 MiB, writes 16 pages, and commits each one from a vectored exception handler, the way Learn's [Reserving and Committing Memory](https://learn.microsoft.com/en-us/windows/win32/memory/reserving-and-committing-memory) example does with an exception filter. It is the same idea as `sbrklazy` and `vmfault()` in xv6. Observed: 16 pages committed every time, but the commit charge grew by 31 or 33 pages when the 16 pages were 4 MiB apart, and by 16 or 18 when they were side by side. Learn does not say what else is charged. **Exercise:** propose an explanation (page-table pages are one candidate, since each 4 MiB step needs a new one) and design a test.
- **Part 3** lists the address space by allocation, the NT counterpart of `/proc/self/maps`: `MEM_IMAGE` for the exe and DLLs, `MEM_MAPPED` for sections (locale files and unnamed ones), `MEM_PRIVATE` for heaps and stacks.

`nt_section.exe`, observed:

```
  A after A writes page 0          view 00000213F02B0000: [A wrote page 0      ] [                    ]
  B after opening by name          view 000001BB35820000: [A wrote page 0      ] [                    ]
  B after B writes page 1          view 000001BB35820000: [A wrote page 0      ] [B wrote page 1      ]
  B copy view, after writing 0     view 000001BB355E0000: [B private write     ] [B wrote page 1      ]
  B shared view, same moment       view 000001BB35820000: [A wrote page 0      ] [B wrote page 1      ]
  B commit charge: +0.012 MiB for mapping a 3-page copy view
  B copy view page 0: PAGE_READWRITE  resident, private
  B copy view page 1: PAGE_WRITECOPY  resident, shared
  A after B exited                 view 00000213F02B0000: [A wrote page 0      ] [B wrote page 1      ]
  SEC_COMMIT  create  +256.6 MiB, map a view    +0.5 MiB, (nothing more)           +0.0 MiB
  SEC_RESERVE create    +0.0 MiB, map a view    +0.5 MiB, VirtualAlloc 16 MiB     +16.0 MiB
  with a view mapped (same handle):            SetEndOfFile -> FALSE, error 1224 (The requested operation cannot be performed on a file with a user-mapped section open.)
  with a view mapped (second handle):          SetEndOfFile -> FALSE, error 1224
  view unmapped, section handle still open:    SetEndOfFile -> FALSE, error 1224
  view unmapped and section closed:            SetEndOfFile -> TRUE, file size now 4096
```

- **One section, two processes, two addresses.** [CreateFileMapping](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga) with `INVALID_HANDLE_VALUE` creates an object "backed by the system paging file", and a second process can open it by name, by an inherited handle or with `DuplicateHandle` (stage 08). Each process mapped it at a different address, so a shared buffer must hold offsets, not pointers.
- **`FILE_MAP_COPY` is NT's private mapping.** [MapViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile) says a write to a copy-on-write page copies it to a new page private to the process, whose protection changes from copy-on-write to read/write. That is what `VirtualQuery` and `QueryWorkingSetEx` showed for page 0. The same page also says the commit charge is taken for the entire view, because every page could become private. That is the +0.012 MiB, three pages, charged before any write.
- **`SEC_COMMIT` charges at creation, `SEC_RESERVE` at `VirtualAlloc`.** Both are described under `CreateFileMapping`. `SEC_COMMIT` is the default.
- **NT refuses to shrink a mapped file.** [SetEndOfFile](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setendoffile) says every view must be unmapped and the mapping object closed first. Here the call failed with error 1224 through a second handle too, and even after the view was unmapped while the section handle was still open. Compare that with the Linux run in exercise E.

## Linux / NT comparison

- **Reserve and commit.** NT separates them ([Learn](https://learn.microsoft.com/en-us/windows/win32/memory/reserving-and-committing-memory)), and the commit charge is accounted when you commit, not when you touch. Linux has no separate commit call: `mmap` of anonymous memory reserves address space, pages arrive on first touch, and whether `mmap` may promise more memory than exists is a system policy ([proc_sys_vm(5)](https://man7.org/linux/man-pages/man5/proc_sys_vm.5.html), `overcommit_memory`). xv6 has `sbrk()` (eager) and `sbrklazy()` (on touch), and no accounting at all.
- **Sections and mappings.** Learn says a file mapping object is also known as a section object ([Learn](https://learn.microsoft.com/en-us/windows/win32/memory/file-mapping)). Page-file-backed sections are NT's anonymous shared memory. On Linux the closest match is a `memfd` or `MAP_SHARED | MAP_ANONYMOUS` across `fork`.
- **Copy-on-write in three kernels.** Your xv6 `fork`, Linux `fork` and `MAP_PRIVATE`, and NT's `FILE_MAP_COPY` all share a page read-only and copy it on the first write fault. Win32 has no `fork`, so on NT you meet copy-on-write in views like this one.
- **A mapped file that shrinks.** Linux lets the file shrink and sends `SIGBUS` to whoever touches the vanished pages. NT refuses `SetEndOfFile` while any section of the file exists (error 1224). NT has its own failure for mapped data files, though: [Reading and Writing From a File View](https://learn.microsoft.com/en-us/windows/win32/memory/reading-and-writing-from-a-file-view) says an access can raise `EXCEPTION_IN_PAGE_ERROR`, for example when a remote file's server is lost or the disk is full, and tells you to wrap every access in structured exception handling.

## Checkpoints

Answer these in `NOTES.md`.

1. **Translate one virtual address by hand, then confirm it in GDB.** Choose an address from your own `vmprint` output that is not in the first page, ideally one in the top half. Write the three indexes and the offset, each PTE with its flags, which PTEs point to tables and which is the leaf, and the physical address. Then confirm it with `translate.gdb` or the same commands typed by hand. Explain why GDB could read page-table pages by physical address at that breakpoint.
2. **Show the pages COW saved.** Give your `cowtest` numbers for both kernels and explain the free-page arithmetic (77 and 7 here). Show the 2/3-of-memory fork failing and then succeeding. Paste your `usertests -q` result.
3. **Explain why the `SIGBUS` case is a security problem** for any server that maps memory a client handed it. What does the client control? What happens to the server, and to its other clients, without a handler? Give three defenses: a seal that forbids shrinking, a handler that recovers, and copying instead of mapping. Give the cost of each. How does NT's error 1224 change the picture, and what does `EXCEPTION_IN_PAGE_ERROR` leave open? Stage 17 comes back to this.
4. Why must `copyout()` handle copy-on-write pages itself, when user writes are caught by a page fault?
5. Why does Linux's `fork` take longer for a bigger parent, even though it copies no data?
6. On NT, what does the commit charge measure, and why did writing the pages change the working set but not the commit charge?

## Common failure modes

- **`patch` reports failed hunks in `run-xv6.sh`.** Your patches were made against another commit or one of MIT's lab branches. Check with `git log -1` in the clone you diffed.
- **`run-xv6.sh` prints `SKIPPED: ... needs ... in SOLUTION_DIR`.** Your patches are not where the script looks, or not under the names it expects.
- **`usertrap(): unexpected scause 0xf` on the first write after `fork`.** The COW case is missing from `usertrap()`, or it comes after the `vmfault()` case, which returns 0 for a page that is already mapped.
- **`panic: kfree` or `panic: freewalk: leaf`.** Reference counts are out of step: a count was not set in `kalloc()`, a shared page was freed twice, or `uvmunmap` freed a page that another table still maps.
- **Random crashes in other processes after a child exits.** A shared page was freed while someone still mapped it. This is the break-it run above.
- **`pipe()` or `read()` fails in a child under COW.** `copyout()` still refuses every page without `PTE_W`.
- **`fork` succeeds but the parent sees the child's writes.** The parent's PTE kept `PTE_W`. Both PTEs must lose write access.
- **GDB prints `Cannot access memory` for a user address at a kernel breakpoint.** The kernel's page table does not map user addresses. Read the physical address, which the kernel's table does map.
- **A GDB script corrupts the kernel.** `set $a0 = ...` or `set $t1 = ...` writes a CPU register. Use names that are not register names.
- **`expect` times out.** A command printed something the script did not wait for, or the kernel panicked. Read the last lines of the output.
- **Linux: no `SIGBUS`.** The read was still inside the last partial page, which reads as zero. Only whole pages past the new end of file fault.
- **Windows: `undefined reference to WinMain`.** `nt_section.c` uses `wmain`, so it needs `-municode`.
- **Windows: the system commit column jumps.** Other processes allocate too. Compare the process column, and run twice.

## Where this shows up later

- Stage 04 looks at what else takes the CPU from user code: interrupts. `usertrap()` is the same entry point.
- Stage 07 implements file-backed `mmap` in xv6 and asks who writes dirty mapped pages back, on each system.
- Stage 08 seals a `memfd` so that a peer cannot shrink it, and hands an NT section to another process with `DuplicateHandle`.
- Stages 09 and 17 feed a server a client that shrinks its buffer: exercise E's part 2.

## Next

[Lab 04](../04-interrupts-drivers/README.md) follows a device interrupt and the timer interrupt through xv6, writes a small Linux character-device module, and looks at interrupts and DPCs on Windows.
