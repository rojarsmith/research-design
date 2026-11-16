# Lab 07: File systems

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 07](../../SYLLABUS.md#07-file-systems)

> **Draft for review, 2026-11-16. What was and was not executed:**
> - **xv6 parts (exercises A–F): built and run** on 2026-11-16 in an Ubuntu 24.04 container under podman on Windows 11 (a rootful podman machine on WSL 2; not WSL 2 Ubuntu itself), with QEMU 8.2.2, riscv64-linux-gnu-gcc 13.3.0, gcc 13.3.0 for the host tool and expect, on xv6-riscv commit [`fa2ddcb7`](https://github.com/mit-pdos/xv6-riscv/commit/fa2ddcb7ca87a77af579380a6eafd5aff163b44f) (2026-11-05). The results of exercises B, C and D came from reference implementations that are not shipped. Exercises A, E and F use only files in this folder. QEMU was killed with SIGKILL in the middle of writes 11 times: 5 times at chosen points of a commit, 6 times after fixed delays. Output: [`xv6/example-output.txt`](xv6/example-output.txt). `usertests -q` passed with three harts on the kernel with all three reference patches; it took about 8.5 minutes, and its `writebig` test now writes a 65,803-block file.
> - **Linux parts (exercises G and H): built and run** in the same container, twice, on kernel 6.6.87.2-microsoft-standard-WSL2 with gcc 13.3.0, on an ext4 podman volume. `kcmp(2)` was refused without `--cap-add SYS_PTRACE` and worked with it. Output: [`linux/example-output.txt`](linux/example-output.txt).
> - **Windows parts (exercises I and J): built and run** natively, unelevated, with gcc 16.2 from WinLibs, on NTFS. Output: [`windows/example-output.txt`](windows/example-output.txt).
> - **Not executed:** the steps marked *not run here*. No trace was taken of the threads that do the writing back (Linux's flusher threads, NT's lazy writer and mapped page writer); exercises H and J measure their effects from outside.
> - **Machine:** Windows 11 Pro build 26300, Intel Core i7-11700 (16 logical processors), hypervisor present. Other work, including two other xv6 labs in the same podman machine, was running during every run. Every time below is specific to this machine and these runs.

## Goal

A file system keeps two promises: a file says what was last written to it, and a crash at any moment leaves the disk in a state the next boot can repair. In xv6 you extend the on-disk inode to reach 64 MiB files, add symbolic links to path lookup, and map files into memory with `mmap`. Then you kill QEMU in the middle of writes, read the disk image from the host before the next boot, and watch the log put it back together. On Linux and NT you meet the two questions that matter later in this course: what a file descriptor (or handle) really points to, which is what stage 08 passes between processes, and who decides when a dirty mapped page reaches the disk.

```
process A          process A          process B (fork of A)     process C
fd 3 ──┐           fd 4 (dup of 3)─┐  fd 3 ──┐                  fd 5 ──┐
       ▼                           ▼         ▼                         ▼
  open file description: offset, status flags, access mode     another open file description
  (struct file in xv6 and Linux; a file object on NT)          (its own offset)
                     │                                                 │
                     └──────────────────► inode ◄──────────────────────┘
                                            │ buffer cache (xv6), page cache (Linux), cache manager (NT)
                                            ▼
                                           disk

xv6 commit():  write_log()       write_head(n)          install_trans()       write_head(0)
               blocks go to the  the commit point: the  blocks are copied     the transaction
               log area          header names them      to their homes        is erased
a crash here:  as if the call    the next boot copies   the next boot copies  nothing to do
               never happened    all n blocks home      them home again
               crash point 1     crash point 2 (k)      crash point 3
```

## The pinned xv6 commit

Everything here was written and run against **xv6-riscv commit `fa2ddcb7ca87a77af579380a6eafd5aff163b44f`**, the commit [Lab 03](../03-virtual-memory/README.md#the-pinned-xv6-commit) pins. What matters for this lab in that tree: blocks are 1024 bytes, `FSSIZE` is 2000 blocks, the log holds `LOGBLOCKS` = 30 blocks and every system call reserves `MAXOPBLOCKS` = 10 of them, and an inode has 12 direct blocks and one singly-indirect block. `fsinit()` runs in the first process (from `forkret()`), because recovery reads the disk and must be able to sleep. After recovery, `ireclaim()` frees inodes that have no links but were still open at the crash. There is a `sync()` system call. MIT's `fs` and `mmap` lab branches start from different code, so the patches here are for plain xv6-riscv only.

## Files

| File | What it is | Status |
| --- | --- | --- |
| [`xv6/ofdtest.c`](xv6/ofdtest.c) | `dup`, two `open`s and `fork` writing through two descriptors, on the stock kernel | built and run here |
| [`xv6/fsinspect.c`](xv6/fsinspect.c) | a host program that reads `fs.img`: layout, the log header and what each logged block is, a consistency check (an fsck), one file's blocks | built and run here |
| [`xv6/bigfile.c`](xv6/bigfile.c) | writes one file until the kernel refuses, reads it back, deletes it | built and run here, stock and reference kernels |
| [`xv6/symlinktest.c`](xv6/symlinktest.c) | symbolic links: follow, `O_NOFOLLOW`, chains, cycles, dangling links, `link`/`unlink` | built and run here, reference kernel |
| [`xv6/mmaptest.c`](xv6/mmaptest.c) | `mmap`/`munmap`: laziness, `MAP_SHARED` and `MAP_PRIVATE`, permissions, partial unmaps, `fork`, `exit` | built and run here, reference kernel |
| [`xv6/lab07-crashpoint.patch`](xv6/lab07-crashpoint.patch) | crash points in `commit()` and a `crashpoint(p, k)` system call that arms one | built and run here |
| [`xv6/crashwrite.c`](xv6/crashwrite.c) | gives the log a transaction to lose: an append, a truncate, an endless rewrite loop, and a check of every block | built and run here |
| [`xv6/break-install-first.patch`](xv6/break-install-first.patch) | broken on purpose: `commit()` installs blocks before it writes the header | built and run here |
| [`xv6/run.sh`](xv6/run.sh), [`xv6/xv6crash.exp`](xv6/xv6crash.exp) | builds every variant from a clean tree, boots it with Lab 03's `xv6run.exp`, kills QEMU with SIGKILL where asked, and runs `fsinspect` between boots | run here |
| reference solutions (not shipped) | exercises B, C and D | the recorded output for those came from reference patches kept outside this repository |
| [`linux/ofd.c`](linux/ofd.c) | descriptors and open file descriptions: offsets, flags, `/proc/self/fdinfo`, `kcmp`, OFD locks and POSIX locks | built and run here |
| [`linux/pagecache.c`](linux/pagecache.c) | one page cache for `mmap` and `read`; `msync`, `munmap`, `fsync`, and the flusher threads, watched through `/proc/self/smaps` | built and run here |
| [`linux/run.sh`](linux/run.sh) | builds and runs both | run here |
| [`windows/nt_handles.c`](windows/nt_handles.c) | `DuplicateHandle`, two `CreateFile`s, an inherited handle, a handle duplicated into a child, and a read-only duplicate | built and run here |
| [`windows/nt_flush.c`](windows/nt_flush.c) | a mapped file: `FlushViewOfFile`, `FlushFileBuffers`, a view kept dirty, an unmapped view, and `WriteFile` through the cache manager | built and run here |
| `*/example-output.txt` | recorded output | recorded 2026-11-16 |

Reference solutions (not shipped): the output recorded for exercises B, C and D came from reference patches kept outside this repository. They solve this lab's versions of MIT 6.1810's `fs` (large files, symbolic links) and `mmap` assignments. Write your own. To run `run.sh` on your work, save it as patches against the pinned commit, each on top of the one before: `0001-bigfile.patch` for B, `0002-symlink.patch` for C, `0003-mmap.patch` for D. Put them in `xv6/solution/`, which git ignores, or point `SOLUTION_DIR` at them. A part whose patches are missing is skipped with a message. You do not need to add the test programs to `UPROGS` yourself; `run.sh` adds them if your patch did not.

## Setup

- **xv6.** The Lab 00 workbench at the pinned commit, plus `expect`, `patch` and a host `gcc` (for `fsinspect`). `run.sh` copies your checkout to a temporary directory for every variant and never changes it:

  ```sh
  sh xv6/run.sh ~/xv6-riscv stock crash random break    # no solution needed
  SOLUTION_DIR=~/lab07-mine sh xv6/run.sh ~/xv6-riscv   # every part
  SOLUTION_DIR=~/lab07-mine sh xv6/run.sh ~/xv6-riscv usertests
  ```

  The whole default run took about 5 minutes here, about 3 of them for writing one maximum-size file in exercise B. Use one hart (`make CPUS=1 qemu`) when you work by hand, so that block numbers match the ones below.
- **The container used here.** An image with MIT's tool packages, expect and the pinned tree at `/opt/xv6-riscv`, started from Git Bash with the repository mounted read-only:

  ```sh
  MSYS_NO_PATHCONV=1 podman run --rm -v "C:/path/to/research-design:/work:ro" \
    -v "C:/path/to/your/patches:/sol:ro" oslab:24.04 \
    sh -c 'SOLUTION_DIR=/sol sh /work/os-foundations/labs/07-file-systems/xv6/run.sh /opt/xv6-riscv'
  ```

- **Linux:** gcc, then `sh linux/run.sh ~` in WSL 2 Ubuntu. Give it a directory on a disk file system: your home directory in WSL 2 is ext4. Not `/tmp` if it is tmpfs, and not `/mnt/c`. The runs here used a podman named volume (`podman volume create lab07-data`, mounted at `/data`), which is ext4, because the container's own root is overlayfs mounted with `volatile`, which skips `fsync`. For `kcmp(2)` in a container, add `--cap-add SYS_PTRACE`.
- **Windows:** any x86-64 MinGW-w64 gcc. `nt_flush.c` links with `-lpdh`. Nothing needs administrator rights.
- MIT's matching labs are [fs](https://pdos.csail.mit.edu/6.1810/2025/labs/fs.html) and [mmap](https://pdos.csail.mit.edu/6.1810/2025/labs/mmap.html). Their test programs (`bigfile`, `symlinktest`, `mmaptest`) live in the `xv6-labs-2025` branches; the ones here are this lab's own and run on plain xv6-riscv.

## Read first

- The xv6 book, chapter 10 (file system), which is what MIT's fs lab asks you to read.
- `kernel/log.c`: `begin_op()`, `end_op()`, `log_write()`, `commit()`, `recover_from_log()`. `kernel/bio.c`: `bread()`, `bwrite()`, `bpin()`. `kernel/fs.c`: `balloc()`, `bmap()`, `itrunc()`, `iput()`, `ireclaim()`, `namex()`. `kernel/file.c` and `kernel/file.h`: `struct file`. `kernel/sysfile.c`: `sys_open()`, `create()`, `sys_link()`, `sys_unlink()`. `mkfs/mkfs.c` for the disk layout.
- Linux: [open(2)](https://man7.org/linux/man-pages/man2/open.2.html) (open file descriptions), [F_GETFL(2const)](https://man7.org/linux/man-pages/man2/F_GETFL.2const.html), [F_OFD_SETLK(2const)](https://man7.org/linux/man-pages/man2/F_OFD_SETLK.2const.html), [proc_pid_fdinfo(5)](https://man7.org/linux/man-pages/man5/proc_pid_fdinfo.5.html), [msync(2)](https://man7.org/linux/man-pages/man2/msync.2.html), the `dirty_*` settings in [Documentation for /proc/sys/vm](https://docs.kernel.org/admin-guide/sysctl/vm.html), and the [VFS overview](https://docs.kernel.org/filesystems/vfs.html).
- Learn: [DuplicateHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle), [CreateFileMapping](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga), [FlushViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-flushviewoffile), [UnmapViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-unmapviewoffile), [FlushFileBuffers](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers), [File Caching](https://learn.microsoft.com/en-us/windows/win32/fileio/file-caching).

## Exercises

As before, write each prediction in `NOTES.md` before you run anything.

### A. Descriptors, open file descriptions and inodes in xv6 (run here)

Read `struct file` in `kernel/file.h` and `filealloc()`, `filedup()` and `fileclose()` in `kernel/file.c`. `p->ofile[fd]` is a pointer; `dup()` and `fork()` copy the pointer and raise `f->ref`, and `open()` allocates a new `struct file` for the same in-memory inode. The offset `f->off` lives in the `struct file`. Copy [`ofdtest.c`](xv6/ofdtest.c) to `user/`, add it to `UPROGS`, and **predict** the file's contents after each part before you run it.

Observed (stock kernel, one hart):

```
1. dup(): two descriptors, one struct file
  fd 3 and fd 4, inode 26 and 26
  write AAAA via fd a, then BBBB via fd b:
    the file is now "AAAABBBB"
2. open() twice: two struct files, one inode
  fd 3 and fd 4, inode 26 and 26
  write cccc via fd c, then dd via fd d:
    the file is now "ddccBBBB"
3. fork(): the child's descriptor is the parent's struct file
  the child writes child. and exits, then the parent writes parent:
    the file is now "child.parent"
4. the access mode lives in the struct file, so a dup has it too
  write via the O_RDONLY fd: -1, via its dup: -1
```

`struct file` is what POSIX calls the open file description, and what stage 08 passes from one process to another. xv6 has no `lseek` and no file status flags to change, so the offset and the access mode are all it holds.

Then look at the disk itself. Build [`fsinspect.c`](xv6/fsinspect.c) on the host against your tree (`gcc -O2 -Wall -I ~/xv6-riscv -o fsinspect fsinspect.c`) and run it on a fresh `fs.img`. **Predict** first, from `mkfs.c` and `param.h`: where does the log start and end, how many inode blocks are there, and where does the data area begin?

```
fs.img: 2000 blocks of 1024 bytes
  superblock: size 2000, nblocks 1953, ninodes 200, nlog 31
  layout: 0 boot | 1 super | 2-32 log (header + 30) | 33-45 inodes (16 per block) | 46-46 bitmap | 47-1999 data
  log header: n = 0
  data blocks in use (bitmap): 994 of 1953; inodes in use: 24 of 199
/README: inode 2, type 2 (file), nlink 1, size 2441 bytes (3 blocks)
  inode lives in block 33; struct dinode is 64 bytes, 13 addrs (NDIRECT 12)
  direct (3 of 12 used): 48-50
```

`fsinspect fs.img check` is a small fsck. It follows every inode's block pointers, checks that each data block is used once and marked in the bitmap, that every block marked in the bitmap is used, and that each inode's `nlink` equals the number of directory entries naming it (in xv6, `.` does not count). Exercises B and E rely on it.

### B. Larger files: a doubly-indirect block (xv6)

On the stock kernel, [`bigfile.c`](xv6/bigfile.c) shows the limit. Observed: `MAXFILE = 268 blocks`, `wrote 268 blocks (274432 bytes); the write of block 268 was refused`. With `bigfile 268 keep`, `fsinspect fs.img file /big.file` shows the 12 direct blocks (1041–1052), the indirect block 1053 and its 256 entries (1054–1309).

Change the inode as MIT's [fs lab](https://pdos.csail.mit.edu/6.1810/2025/labs/fs.html) describes:

1. `NDIRECT` becomes 11. `addrs[NDIRECT]` stays the singly-indirect block, and `addrs[NDIRECT + 1]` is a new doubly-indirect block: a block of 256 block numbers, each naming an indirect block of 256 data blocks. `struct dinode` keeps its 64 bytes, so `addrs[]` stays 13 entries long, in `struct dinode` and in `struct inode` alike. `MAXFILE` becomes 11 + 256 + 256 × 256 = 65,803 blocks.
2. `bmap()`: allocate the doubly-indirect block and the indirect block under it only when they are first needed, `log_write()` each index block you change, and `brelse()` every block you `bread()`.
3. `itrunc()`: free every data block, every indirect block under the doubly-indirect block, and the doubly-indirect block itself.
4. `FSSIZE` in `kernel/param.h`: a maximum-size file needs 65,803 data blocks and 258 index blocks. The reference used 70,000 blocks (70 MB image); MIT's lab uses 200,000. `mkfs` needs no other change. Why not?

Before you run it, **check** that one `filewrite()` chunk still fits in a transaction: `filewrite()` writes at most 3 blocks per transaction, and `begin_op()` reserves `MAXOPBLOCKS` = 10 log blocks per system call. Count the worst case in the doubly-indirect range: data blocks, bitmap blocks, the inode, the doubly-indirect block and the indirect blocks under it, for a write that is not block-aligned and crosses an indirect block. **Predict** how many index blocks `bigfile 6000 keep` leaves behind.

Observed with the reference solution (not shipped), one hart:

| | stock kernel | doubly-indirect kernel |
| --- | ---: | ---: |
| `MAXFILE` | 268 blocks (274,432 bytes) | 65,803 blocks (67,382,272 bytes) |
| `bigfile` | wrote 268, block 268 refused | wrote 65,803, block 65,803 refused |
| ticks to write the maximum-size file (1 tick ≈ 0.1 s) | 7 | 1,865 |
| `bigfile 6000 keep`: index blocks | (not possible) | 25: 1 indirect, 1 doubly indirect, 23 under it |
| `fsinspect check` after `bigfile` deleted its file | consistent | consistent: 7,019 blocks in use = 994 + 6,000 + 25 |

```
/big.file: inode 26, type 2 (file), nlink 1, size 6144000 bytes (6000 blocks)
  inode lives in block 34; struct dinode is 64 bytes, 13 addrs (NDIRECT 11)
  direct (11 of 11 used): 1049-1059
  singly indirect, block 1060 (256 of 256 entries used): 1061-1316
  doubly indirect, block 1317 (23 of 256 indirect blocks, 5733 data blocks): 1318 1575 1832 2089 2346 2603 ...
```

The indirect blocks under the doubly-indirect block sit every 257 blocks (1318, 1575, …): each one is allocated just before the 256 data blocks it lists. Per block, the maximum-size file cost about as much as the 268-block one: 0.028 and 0.026 ticks. That cost is the log: every 3-block chunk is a transaction that writes each data block twice (log, then home), plus the bitmap, the inode and index blocks twice, plus two header writes. `balloc()` scans the bitmap from block 0 for every allocation, a cost that grows as the disk fills, but at this size it did not show.

**Break it (not run here):** leave the doubly-indirect part out of `itrunc()`. **Predict** what `fsinspect check` reports after `bigfile 6000` deletes its file.

### C. Symbolic links (xv6)

Add `symlink(target, path)` as MIT's fs lab specifies: a new inode type `T_SYMLINK` (4, in `kernel/stat.h`) whose contents are the target path, and a flag `O_NOFOLLOW` (0x800, in `kernel/fcntl.h`, clear of the other `O_` flags). `open()` follows a link, and a link to a link, until it reaches something that is not a link; with `O_NOFOLLOW` it opens the link itself. A cycle must fail rather than loop; a depth limit (MIT suggests about 10) is enough. `link()` and `unlink()` act on the link itself, and links to directories are not required. Copy [`symlinktest.c`](xv6/symlinktest.c) to `user/`.

**Predict** before you write `open()`'s loop: may you hold the link's inode lock while you look up its target? What could happen with a link to itself if you did?

Observed with the reference solution (not shipped), which follows at most 10 links and calls `namei()` on the target only after `iunlockput()` on the link: all checks passed, twice in a row (the second run starts with leftovers removed). The 5-deep chain opened; the 12-deep one was refused (`open("/sl.12") = -1`). With `O_NOFOLLOW`, `/sl.b` opened as a `T_SYMLINK` inode containing `"/sl.a"`.

Think about what this simple version does not do:
- A link in the middle of a path (`/link/file`) is not followed: only `open()`'s last component is. Where in `namex()` would you add it?
- A relative target is looked up from the caller's current directory. Linux resolves it from the directory that holds the link ([path_resolution(7)](https://man7.org/linux/man-pages/man7/path_resolution.7.html)). The test only checks a link and a caller that are both in `/`, where the two agree.

### D. File-backed mmap (xv6)

Implement `mmap()` and `munmap()` as MIT's [mmap lab](https://pdos.csail.mit.edu/6.1810/2025/labs/mmap.html) describes, with these declarations in `user/user.h` and constants in `kernel/fcntl.h` (`PROT_READ` 1, `PROT_WRITE` 2, `PROT_EXEC` 4, `MAP_SHARED` 1, `MAP_PRIVATE` 2):

```c
void *mmap(void *addr, uint64 len, int prot, int flags, int fd, uint64 off);
int munmap(void *addr, uint64 len);
```

`addr` is always 0 (the kernel chooses), `off` is a multiple of the page size (0 in the tests), and `mmap` returns `(void *)-1` on failure. The parts:

1. **A table of mappings per process:** 16 entries of address, length, offset, protection, flags and `struct file *` (take a reference with `filedup()`). Put mappings below `TRAPFRAME`, each below the lowest so far, and keep the heap out of them.
2. **Nothing happens until a fault.** `mmap()` reads nothing and allocates nothing. In this tree, `usertrap()` sends load and store page faults to `vmfault()`, and `copyin()`/`copyout()` call it too, so handle a mapped address there: allocate a page, `readi()` the file into it (the part past the end of the file stays zero), and map it with the mapping's permissions. Then `write(fd, mapped_address, n)` works even for a page the process never touched.
3. **Write back at `munmap()`, `exit()` and `exec()`.** For `MAP_SHARED`, write back the pages the hardware marked dirty (`PTE_D`, bit 7; `start()` sets `MENVCFG_ADUE`, so QEMU sets it on the first store), in transactions no larger than `filewrite()`'s, and never past the end of the file. `munmap()` may remove the start, the end or all of a mapping. Free the pages; close the file when the mapping is gone.
4. **`fork()`** gives the child the same mappings (and file references). MIT allows the child to read the file again on its own faults. The reference copies every page the parent has touched instead, so that a private change survives the fork.

Two traps the reference had to handle: `pipewrite()` and `kwait()` call `copyin()`/`copyout()` while they hold a spinlock, and `readi()` may sleep, so a mapped page cannot be read in there (the reference refuses the fault, and the system call fails); and `read(fd, map_of_the_same_file, n)` faults while `readi()` already holds that inode's sleep lock.

Copy [`mmaptest.c`](xv6/mmaptest.c) to `user/`. Lines marked `ok`/`FAILED` are requirements; lines marked `observed` are behavior the requirements leave open. **Predict** every `observed` line before you run it. Observed with the reference solution (not shipped): `mmaptest: 0 failure(s)` with one hart and with three. The `observed` lines, next to what Linux does:

| Question | xv6, reference | Linux |
| --- | --- | --- |
| a page touched before a `write()` to the file shows | the old byte | the new byte |
| `read()` of the file before `munmap()` of a `MAP_SHARED` store | the old bytes | the new bytes |
| page 2, still mapped after `munmap()` of page 0 | not in the file yet | in the file's pages already |
| the child's view of the parent's earlier private write | inherited | inherited (copy-on-write) |
| the parent's `MAP_SHARED` page after the child wrote and exited | does not show it | shows it |
| process B's `MAP_SHARED` page after process A wrote its own | does not show it | shows it at once ([Lab 03](../03-virtual-memory/README.md), exercise D) |

Every row has one cause: xv6 has no page cache. Its buffer cache holds disk blocks for the file system code, and each mapping gets its own private copy of each page, which meets the file only when it is written back. Linux maps the page-cache pages themselves, the same pages `read()` and `write()` use (exercise H). The mapped address in the recorded run was `0x3FFFFFB000`: three pages just below `TRAPFRAME` (`0x3FFFFFE000`).

**Break it (not run here):** in your `vmafault()` (or whatever you call it), remove the spinlock check and run `write(pipe_fd, untouched_mapped_page, 1)`. **Predict** the panic.

### E. Kill QEMU in the middle of a write (xv6, run here)

This is the checkpoint. It needs no solution: [`lab07-crashpoint.patch`](xv6/lab07-crashpoint.patch) adds three places in `commit()` where the kernel can stop, and a system call `crashpoint(p, k)` (number 30) that arms one of them for the next commit. At the stop, the kernel prints the transaction's blocks, what the header on the disk says, and how many blocks it has installed, then turns off interrupts and spins. [`xv6crash.exp`](xv6/xv6crash.exp) sees the message and kills QEMU with SIGKILL, the closest thing to pulling the power cord. Every write xv6 saw complete is already in `fs.img`: `virtio_disk_rw()` sleeps until the device reports completion, and QEMU has handed the data to the host by then.

Each scenario in `run.sh crash` starts from the same image, made by `crashwrite prepare`: a 12-block `/crashfile` (inode 27, blocks 1093–1104), exactly the direct blocks of a stock inode. Then:

1. boot, run `crashwrite append P K` (or `trunc`), and kill QEMU at the crash point;
2. on the host, `fsinspect fs.img log` and `fsinspect fs.img check`: the disk as the crash left it;
3. boot again: `recover_from_log()` runs in `fsinit()` before `init` starts, and prints `recovering tail i dst b` for each block it copies home; then `crashwrite check` reads every block of `/crashfile`;
4. on the host, `fsinspect fs.img check` again.

The append writes blocks 12–14 with one `write()` of 3,072 bytes, which `filewrite()` makes one transaction. **Predict** its blocks, in the order `log_write()` first sees them, before you look: the bitmap, a new indirect block (block 12 is the first block past the direct ones), three data blocks, and the inode's block. Observed: `46 1105 1106 1107 1108 34`.

| Scenario | Header on the disk at the kill | Home blocks installed | `fsinspect check` before the next boot | Next boot | `/crashfile` after |
| --- | --- | --- | --- | --- | --- |
| append, crash point 1 | n = 0 | 0 of 6 | consistent | nothing to recover | 12 blocks, generation 1: the append never happened |
| append, crash point 2, k = 3 | n = 6 | 3 of 6 | 4 problems: blocks 1105–1108 marked in use, used by no inode | `recovering tail 0`…`5` | 15 blocks: 0–11 generation 1, 12–14 generation 2 |
| append, crash point 3 | n = 6 | 6 of 6 | consistent | `recovering tail 0`…`5` again | 15 blocks, as above |
| truncate, crash point 2, k = 1 | n = 2 | 1 of 2 (the bitmap) | 12 problems: blocks 1093–1104 in use by `/crashfile` but marked free | `recovering tail 0`, `1` | 0 blocks; `crashwrite other` then takes 4 of the freed blocks, and the check stays consistent |

Wherever the header said n > 0, `fsinspect` also checked the image "as recovery will leave it", by copying the logged blocks home in memory, and found it consistent before the boot did the same.

**Walk through crash point 2 step by step**, with the blocks above, then do it for your own run in `NOTES.md`:

1. `write()` → `filewrite()` → `begin_op()`: no commit is running and the log has room, so `log.outstanding` becomes 1.
2. `writei()` → `bmap()` for block 12: `balloc()` sets a bit in bitmap block 46 in the buffer cache and calls `log_write(46)`, which only records the block number and pins the buffer. It zeroes the new indirect block 1105 the same way, then allocates data block 1106 and writes its number into 1105. Blocks 13 and 14 add 1107 and 1108, and `iupdate()` adds the inode's block 34. Nothing has reached the disk yet.
3. `end_op()`: `outstanding` drops to 0, so this call commits. `write_log()` copies the six cached blocks into log blocks 3–8: six disk writes. (Crash point 1 stops here. The data is in the log, but the header still says n = 0, so the next boot ignores it, and the file is as it was. That is the "nothing" in all or nothing.)
4. `write_head()` writes block 2 with n = 6 and the six home block numbers. **This single-block write is the commit point.** A block write is assumed to be atomic, so after a crash the header holds either the old n = 0 or the new n = 6.
5. `install_trans(0)` copies the blocks home: 46, 1105, 1106, … and the crash point stops it before 1107. On the disk now: the bitmap says 1105–1108 are in use, block 1105 lists them, block 1106 holds data, but the inode in block 34 still says 12 blocks with no indirect block. `fsinspect check` calls 1105–1108 leaked. That is an inconsistent file system, and only the log makes it safe.
6. Next boot: `fsinit()` → `initlog()` → `recover_from_log()` → `read_head()` finds n = 6, `install_trans(1)` copies all six log blocks home again (`recovering tail 0 dst 46` … `recovering tail 5 dst 34`), including the three that were already there, and `write_head()` writes n = 0. Then `ireclaim()` looks for orphaned inodes.
7. `/crashfile` now has 15 blocks and the check is consistent. Crash point 3 shows the same replay after everything had already been installed: copying a block home twice leaves the same bytes, so recovery is idempotent and a crash during recovery is harmless too.

The truncate shows the dangerous direction. Freeing comes first in the log (`bfree()` clears the bits in block 46), then the inode. Half-installed, the bitmap says the file's blocks are free while the inode still points to them. Without recovery, the next file created would get those blocks. Exercise F shows that happening.

**Random kills (run here).** `run.sh random` boots the crash kernel without arming a crash point, runs `crashwrite loop` (rewrite a 30-block `/crashfile` and create and delete `/crash.tmp`, round after round, printing nothing), and kills QEMU after a fixed delay. Observed, six kills:

| Kill after | Header on the disk | `fsinspect check` before the next boot | Next boot | `/crashfile` after |
| --- | --- | --- | --- | --- |
| 2 s | n = 6: blocks 18–20 being appended, 4 of 6 equal at home already | consistent | 6 blocks copied home | 21 blocks, all generation 23 |
| 3 s | n = 5: blocks 9–11 being appended, none home yet | consistent | 5 blocks copied home | 12 blocks, all generation 35 |
| 4 s | n = 0 | consistent | nothing to recover | 18 blocks, all generation 45 |
| 5 s | n = 0 | consistent | nothing to recover | 30 blocks, all generation 55 |
| 6 s | n = 0 | consistent | nothing to recover | 12 blocks, all generation 65 |
| 7 s | n = 6: blocks 21–23 being appended, 2 of 6 equal at home | 3 problems: blocks 1116–1118 marked in use, used by no inode | 6 blocks copied home | 24 blocks, all generation 76 |

Every image was consistent after the next boot, and every block of `/crashfile` held its own line. The file's length is whatever the last committed transaction left: each round truncates it and writes it again 3 blocks at a time. In half the kills a committed transaction was waiting in the log. A committed transaction can be half installed and still look consistent: at 2 s the first four blocks in log order were equal at home and the last two (a data block and the inode block) were not, most likely a kill in the middle of `install_trans()`; the new blocks already home lay past the end of the file as the old inode saw it. "Same as home" in `fsinspect log` means equal bytes, which is also what an unchanged block looks like; it does not prove the block was installed.

**Predict** before you run it: what fraction of kills should find n > 0 in the header? What decides it?

### F. Break it: install before the commit (run here)

[`break-install-first.patch`](xv6/break-install-first.patch) swaps two lines in `commit()`: it installs the home blocks first and writes the header afterwards. Nothing else changes, and without a crash nothing looks different. `run.sh break` repeats the truncate scenario on that kernel. **Predict** what the next boot recovers, and what happens to `/crashfile` after `crashwrite other` creates a 4-block file.

Observed:

```
lab07: crash point 2, a transaction of 2 blocks: 46 34
lab07: all 2 written to the log; the header on the disk says n = 0; 1 of 2 installed at home
...
log header at block 2: n = 0. Recovery will install nothing.
  12 problem(s): blocks 1093-1104 in use (data blocks 0-11 of inode 27 /crashfile) but marked free in the bitmap
--- next boot (no "recovering" lines)
$ crashwrite check
check: /crashfile is 12288 bytes, 12 blocks
check:   blocks 0-11: generation 1
$ crashwrite other
$ crashwrite check
check: block 0 is torn, it starts "other: a new file"
...
check:   blocks 4-11: generation 1
check: 4 block(s) torn
--- host: fsinspect fs.img check
  PROBLEM: block 1093 is used twice: by inode 27 (/crashfile) and inode 28 (/other)
  ... (1094-1096 the same; 1097-1104 still in use but marked free)
```

(Shortened; the full lines are in the example output.) The file system came back up without complaint, and two files now share four blocks: writing either one corrupts the other. This is why the header write must come after the log writes and before any home write, and why the log must hold the whole transaction before the commit point.

### G. Descriptors and open file descriptions on Linux (run here)

```sh
sh linux/run.sh ~        # or: gcc -O2 -Wall -o ofd linux/ofd.c && ./ofd ~
```

[open(2)](https://man7.org/linux/man-pages/man2/open.2.html): "Each open() of a file creates a new open file description", and a descriptor made by `dup()` or inherited across `fork()` refers to the same one, so the two "share the file offset and file status flags". [F_GETFL(2const)](https://man7.org/linux/man-pages/man2/F_GETFL.2const.html) says that on Linux, `F_SETFL` can change only `O_APPEND`, `O_ASYNC`, `O_DIRECT`, `O_NOATIME` and `O_NONBLOCK`. [`ofd.c`](linux/ofd.c) runs xv6's `ofdtest` again and adds what Linux has on top. **Predict** every line of parts 1–5 first, in particular parts 4 and 5.

Observed (run 2, with `--cap-add SYS_PTRACE`; run 1 was the same except that `kcmp` said `Operation not permitted`):

```
1. dup(): two descriptors, one open file description
  write AAAA via fd a, then BBBB via its dup b:        file: "AAAABBBB"
    after F_SETFL O_APPEND and F_SETFD FD_CLOEXEC on fd 3 only:
    fd 3: O_APPEND on, FD_CLOEXEC on
    fd 4: O_APPEND on, FD_CLOEXEC off
    fdinfo/3:  pos:	8  flags:	02102002  ino:	64211
    fdinfo/4:  pos:	8  flags:	0102002  ino:	64211
    kcmp(KCMP_FILE, 3, 4) = 0 (the same open file description)
2. open() twice: two open file descriptions, one inode
  write cccc via fd c, then dd via fd d:               file: "ddccBBBB"
    kcmp(KCMP_FILE, 3, 4) = 1 (different ones)
3. fork(): the child's descriptor shares the parent's offset
  child writes child. and exits, then parent writes:   file: "child.parent"
4. open("/proc/self/fd/3"): a new description or the same?
    reopened as fd 4; its offset is 0, fd 3's is 12
5. locks: OFD locks belong to the description, POSIX locks to the process
    F_OFD_SETLK write lock via fd a:              granted
    F_OFD_SETLK write lock via its dup b:         granted
    F_OFD_SETLK write lock via a second open() c: refused (EAGAIN)
    after close(a), via c:                        refused (EAGAIN)
    after close(b) too, via c:                    granted
    F_SETLK (POSIX) write lock via fd a:          granted
    child, F_SETLK on its own open():             refused (EAGAIN)
    parent opened and closed an unrelated fd c for the same file
    child, again after the parent closed fd c:    granted
```

(Shortened; the full output has every `fdinfo` line.) What each part shows:

- **Status flags travel with the description, descriptor flags do not.** `O_APPEND`, set through fd 3, showed up on fd 4; `FD_CLOEXEC` did not. In `fdinfo`, `flags` is octal ([proc_pid_fdinfo(5)](https://man7.org/linux/man-pages/man5/proc_pid_fdinfo.5.html)): `02` is `O_RDWR`, `02000` `O_APPEND`, `0100000` `O_LARGEFILE`, and `02000000` is `O_CLOEXEC`, which the kernel adds to the line when that descriptor has close-on-exec set. `ino` is the same inode for every descriptor.
- **`kcmp(KCMP_FILE)`** answers "same open file description?" directly: 0 means the same, and 1 or 2 give an order between different ones ([kcmp(2)](https://man7.org/linux/man-pages/man2/kcmp.2.html)). It needs ptrace access to the processes compared. In the container it failed with `EPERM` until `--cap-add SYS_PTRACE` was given; podman's default seccomp profile is the likely reason, which was not checked further.
- **Opening `/proc/self/fd/3` made a new description** (offset 0, and `kcmp` said different). open(2) does not describe this case; the run is what shows it on this kernel.
- **Locks.** An OFD lock belongs to the open file description: the dup could take it again, a second `open()` in the same process could not, and it was released only when the last descriptor of that description closed ([F_OFD_SETLK(2const)](https://man7.org/linux/man-pages/man2/F_OFD_SETLK.2const.html)). A traditional POSIX lock belongs to the process, and that page says it is released when the process closes *any* descriptor of the file: here the parent's lock vanished when it opened and closed an unrelated descriptor, and the child got the lock it had been refused. This is why stage 08's clients and servers should not depend on POSIX record locks.

xv6's `struct file` is the same object, smaller: an offset and the access mode, no status flags, no locks.

### H. The page cache and who writes it back, on Linux (run here)

Read the [VFS overview](https://docs.kernel.org/filesystems/vfs.html) with the diagram at the top of this lab in mind. "A file object represents a file opened by a process" (the open file description), "when the VFS opens a file, it creates a new struct file", initialized "with a pointer to the dentry", which leads to the inode. The inode's `address_space` is the page cache: "The address space object is used to group and manage pages in the page cache", and when data is written to a page, its dirty flag is set. `read()`, `write()` and every `MAP_SHARED` mapping of the file use those same pages.

[`pagecache.c`](linux/pagecache.c) maps a 16 MiB file `MAP_SHARED`, dirties every page, and counts the dirty pages of its own mapping in `/proc/self/smaps`. Run it on a disk file system (here an ext4 volume). **Predict** first: does `pread()` on another descriptor see a store through the mapping at once? How long do `msync(MS_ASYNC)` and `msync(MS_SYNC)` take for 16 MiB? Do `munmap()` and `close()` write anything? If you call nothing at all, when do the pages become clean, given the settings printed on the second line?

Observed, two runs (the full output also shows `/proc/meminfo`'s `Dirty` and `Writeback`, which count the whole VM):

| Step (4,096 dirty pages, 16 MiB) | Run 1 | Run 2 |
| --- | --- | --- |
| a store through the mapping, then `pread()` of that byte on another fd | sees it | sees it |
| `msync(MS_ASYNC)` | 0.001 ms, still 16,384 kB dirty | 0.001 ms, still 16,384 kB dirty |
| `msync(MS_SYNC)` | 29.5 ms, then 0 kB dirty | 29.0 ms, then 0 kB dirty |
| `munmap()` and `close()`, map again, read every page | 16,384 kB dirty in the new mapping | the same |
| `fsync(fd)` | 6.0 ms, then 0 kB dirty | 6.8 ms, then 0 kB dirty |
| call nothing: the pages became clean after | 31 s | 26 s |
| settings | `dirty_expire_centisecs` 3000, `dirty_writeback_centisecs` 500 | the same |

- **One copy of each page.** The store through the mapping was visible to `pread()` with no `msync()`, and `pwrite()` was visible through the mapping. There is nothing to keep coherent, because there is only the page cache.
- **`MS_ASYNC` does nothing.** [msync(2)](https://man7.org/linux/man-pages/man2/msync.2.html): "Since Linux 2.6.19, MS_ASYNC is in fact a no-op", because "the kernel properly tracks dirty pages and flushes them to storage as necessary". `MS_SYNC` "requests an update and waits for it to complete".
- **Unmapping and closing write nothing.** After `munmap()` and `close()`, a new mapping found the same 16 MiB still dirty: the dirty state belongs to the page cache, not to the mapping. msync(2): "Without use of this call, there is no guarantee that changes are written back before munmap(2) is called." `fsync()` on any descriptor of the file cleaned the mapped pages too.
- **The flusher threads decide.** The [kernel's vm settings](https://docs.kernel.org/admin-guide/sysctl/vm.html): data "which has been dirty in-memory for longer than" `dirty_expire_centisecs` (30 s here) "will be written out next time a flusher thread wakes up", and the threads wake every `dirty_writeback_centisecs` (5 s). Writeback also starts earlier when dirty memory passes `dirty_background_ratio` of available memory, and a writing process is made to write back itself past `dirty_ratio`. Run 1 matched the rule: 31 s. Run 2 (and a trial run before it) came at 26 s, before the pages were 30 s old. Why was not determined: other containers were writing to the same ext4 file system, and anything that starts writeback of the whole device, such as a `sync()` elsewhere, takes these pages with it. Run it yourself a few times.

Compare with exercise D: in xv6 nothing writes a dirty mapped page except `munmap()`, `exit()` and `exec()` of the process that dirtied it; on Linux, the process that dirtied it is not involved at all.

### I. Handles and file objects on NT (run here)

```powershell
cd windows
gcc -O2 -Wall -o build\nt_handles.exe nt_handles.c
build\nt_handles.exe .
```

On NT, `CreateFile` creates a file object, and the file object holds the current position. A handle is a process's reference to an object, with its own access rights. [DuplicateHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle): "The duplicate handle refers to the same object as the original handle. Therefore, any changes to the object are reflected through both handles. For example, if you duplicate a file handle, the current file position is always the same for both handles." For different positions, the same page says, call `CreateFile` again. **Predict** each part of [`nt_handles.c`](windows/nt_handles.c), especially part 4.

Observed, the same in both runs:

```
1. DuplicateHandle: two handles, one file object
  write AAAA via handle a, then BBBB via its duplicate b:  file: "AAAABBBB"
    after SetFilePointerEx(a, 0, FILE_BEGIN): position via a 0, via b 0
    handles 00000000000000B0 and 00000000000000B4: CompareObjectHandles TRUE (one file object); file index 000c0000000eb088 and 000c0000000eb088
2. CreateFile twice: two file objects, one file
  write cccc via handle c, then dd via handle d:           file: "ddccBBBB"
    handles 00000000000000B4 and 00000000000000B0: CompareObjectHandles FALSE (two file objects); file index 000c0000000eb088 and 000c0000000eb088
3. a child process gets the parent's file object
  inherited: the child writes child., then the parent writes: file: "child.parent"
  DuplicateHandle into a child: it writes dup'd., then the parent: file: "child.parentdup'd.PARENT"
4. access rights belong to the handle, the position to the file object
    WriteFile via the read-only duplicate: FALSE, error 5 (ERROR_ACCESS_DENIED)
    ReadFile of 6 bytes via the duplicate; position via a is now 6
```

(Handle values and the file index differ between runs.)

- Parts 1–3 are exercise G again: a duplicated or inherited handle is a second reference to one file object, as `dup()` and `fork()` give a second descriptor for one open file description, and `CreateFile` twice is `open()` twice. The file index from `GetFileInformationByHandle` plays the role of the inode number. [CompareObjectHandles](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-compareobjecthandles) (Windows 10 and later) is NT's `kcmp(KCMP_FILE)`, and it needs no special rights.
- Part 3's second half is the NT side of stage 08: `DuplicateHandle` into another process, whose new handle value reached the child through a pipe. On Linux the same step is `SCM_RIGHTS`.
- Part 4 is where the two systems differ. The read-only duplicate could not write (error 5), though the original handle can, and reading through it moved the position seen through the original. On Linux the access mode is part of the open file description, so a `dup()` always has the same access, and fewer rights need a new `open()` (exercise G, part 4). DuplicateHandle notes that a duplicate cannot always get *more* rights than the original: a `GENERIC_READ` file handle cannot be duplicated with write access.

### J. Mapped files and flushing on NT (run here)

```powershell
gcc -O2 -Wall -o build\nt_flush.exe nt_flush.c -lpdh
build\nt_flush.exe . 256 30      # directory, MiB, seconds to watch; about 2 minutes
```

[`nt_flush.c`](windows/nt_flush.c) maps a 256 MiB file on NTFS and dirties every page. Two system-wide counters, read through PDH without elevation, show where dirty data waits: `\Memory\Modified Page List Bytes` and `\Cache\Dirty Pages`. Other programs move them too. The timings are the cleaner evidence: a flush that finds 256 MiB dirty takes tens of milliseconds, one that finds nothing returns in about 2–3 ms. **Predict** each part first: after 30 s with the view still mapped, will `FlushViewOfFile` find the pages dirty? After unmapping, how long until they are written?

Observed, two runs:

| Step (65,536 dirty pages, 256 MiB) | Run 1 | Run 2 |
| --- | --- | --- |
| a store through the view, then `ReadFile` of that byte on another handle | sees it | sees it |
| `FlushViewOfFile` of the dirty view | 70.8 ms | 83.4 ms |
| `FlushViewOfFile` again, nothing dirty | 3.4 ms | 2.2 ms |
| `FlushFileBuffers` after it | 0.1 ms | 0.1 ms |
| dirty again, view kept mapped for 30 s: modified page list | unchanged | unchanged (other jumps) |
| then `FlushViewOfFile` | 72.6 ms: still dirty | 65.6 ms: still dirty |
| dirty again, `UnmapViewOfFile` and `CloseHandle`: modified page list | +257 MiB, gone again within 1 s | +256 MiB, gone again within 1 s |
| the same 256 MiB through `WriteFile`, then `FlushFileBuffers` at once | 104.1 ms | 93.4 ms |
| the same, but `FlushFileBuffers` after 30 s | 24.5 ms; cache dirty 256 → 83 MiB meanwhile | 24.6 ms; cache dirty 258 → 80 MiB meanwhile |

- **While a view is mapped, nothing wrote its dirty pages.** After 30 s, `FlushViewOfFile` took as long as on freshly dirtied pages. [UnmapViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-unmapviewoffile) explains it: "Modified pages in the unmapped view are not written to disk until their share count reaches zero, or in other words, until they are unmapped or trimmed from the working sets of all processes that share the pages." Nothing here shows a trim; on a machine short of memory, trimming would take the pages out of the working set and they would be written. Linux's flusher wrote mapped pages after about 30 s (exercise H); here NT left them alone for as long as the view existed.
- **Unmapping hands them to the memory manager.** The 256 MiB appeared on the modified page list at once and left it within a second. A modified page has to be written before it can move to the standby list (the memory manager's design as *Windows Internals* describes it; the Learn pages cited here do not go into it), so they were most likely written within that second. The same page continues: "Even then, the modified pages are written 'lazily' to disk", and it tells applications to call `FlushViewOfFile` "to minimize the risk of data loss in the event of a power failure or a system crash". One second is what this machine did; it is not a promise.
- **`FlushViewOfFile` is not the whole story.** [FlushViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-flushviewoffile) "does not flush the file metadata, and it does not wait to return until the changes are flushed from the underlying hardware disk cache"; for that, call `FlushFileBuffers` afterwards. Here the second call took 0.1 ms, which says little about what the disk's own cache did.
- **`WriteFile` goes through the cache manager instead.** [File Caching](https://learn.microsoft.com/en-us/windows/win32/fileio/file-caching): the cache manager runs a lazy writer every second that "queues one-eighth of the pages that have not been flushed recently to be written to disk" and adjusts the amount as it goes. The cache's dirty pages fell by about two thirds in 30 s, and `FlushFileBuffers` then had a quarter of the work left (24.5 ms against 104.1 ms).
- **Coherence.** A store through the view was visible to `ReadFile`, and `WriteFile` was visible through the view, in both runs. [CreateFileMapping](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga) promises coherence only among views of the same file ("coherent or identical at a specific time", except for remote files) and says "A mapped file and a file that is accessed by using the input and output (I/O) functions (ReadFile and WriteFile) are not necessarily coherent." So this is what NTFS on this machine did, not a guarantee.

## Linux / NT comparison

| | xv6 (your `mmap`) | Linux | NT |
| --- | --- | --- | --- |
| What a descriptor or handle points to | `struct file`: offset, access mode | an open file description (`struct file`): offset, status flags, access mode, OFD locks | a file object: the current position; access rights are in each handle |
| Shared by `dup()` / `DuplicateHandle`, `fork()` / inheritance | yes | yes | yes, with possibly fewer rights per handle |
| A second `open()` / `CreateFile` | a new `struct file` | a new description | a new file object |
| A mapped file's pages | a private copy per mapping | the file's page-cache pages | pages shared by every view of the file |
| Who writes dirty mapped pages back | `munmap()`, `exit()`, `exec()` of that process; nothing else | the flusher threads, once the data is older than `dirty_expire_centisecs`, or sooner past the dirty thresholds or on a sync; `msync(MS_SYNC)` or `fsync()` on demand | nothing while the view is mapped and not trimmed; after the last unmap the memory manager, "lazily"; `FlushViewOfFile` and `FlushFileBuffers` on demand |
| Measured here | written at `munmap()` (mmaptest part 6) | 26 s and 31 s, with nothing called | still dirty after 30 s mapped; written within 1 s of the unmap |
| Crash consistency of metadata | the log in this lab | the file system's own journal (jbd2 for ext4) | NTFS's own log |

The last row is not measured in this lab; it says where to look next.

## Checkpoints

Answer these in `NOTES.md`.

1. **Walk through the crash recovery step by step.** Use your own crash at point 2 with k between 1 and n − 1: list every disk write of the transaction in order (log blocks, header, home blocks), what `fsinspect log` and `fsinspect check` said before the next boot, the `recovering` lines, and the check afterwards. Say exactly which write is the commit point and why a crash on either side of it is safe. Then explain why replaying a transaction twice is harmless (crash point 3), and what `break-install-first.patch` broke (exercise F).
2. **Explain the difference between an fd and the open file description behind it.** Use your `ofd` output: which state lives in the descriptor (`FD_CLOEXEC`), which in the description (offset, status flags, access mode, OFD locks), and which in the inode. Do the same for xv6's `p->ofile[]` and `struct file`, and for an NT handle and its file object (`nt_handles` part 4). Stage 08 passes the open file description to another process with `SCM_RIGHTS`: after that, what do the sender and the receiver share, and what can either change without the other noticing?
3. **For each system, find who decides when dirty mapped pages reach the disk,** and back it with your numbers: your xv6 `mmap` (mmaptest parts 3 and 6), Linux (pagecache part 4), NT (nt_flush parts 3 and 4).
4. Your `bigfile` numbers: why `NDIRECT` drops to 11, how `MAXFILE` comes out at 65,803, why `FSSIZE` had to grow, and how many index blocks a file of n blocks needs.
5. Why does `MAP_SHARED` in your xv6 behave differently from Linux in mmaptest parts 2, 3, 7 and 8? What would xv6 need to behave like Linux?
6. Why must `open()` release a symbolic link's inode before it looks up the target?

## Common failure modes

- **`patch` reports failed hunks in `run.sh`.** Your patches were made against another commit or one of MIT's lab branches. Check which commit the clone you diffed is on.
- **`run.sh` prints `SKIPPED: needs ... in SOLUTION_DIR`.** Your patches are not where it looks, or not under the names it expects.
- **`fsinspect` reports nonsense block numbers, or problems on a fresh image.** It was built against another tree's `kernel/fs.h`, with a different `NDIRECT`. Build it with `-I` pointing at the tree whose `fs.img` you read.
- **`bigfile` stops at 268 blocks on your kernel, or `panic: bmap: out of range`.** `MAXFILE` or `bmap()` still covers only the old layout; `writei()` refuses anything past `MAXFILE * BSIZE`.
- **`balloc: out of blocks` during `bigfile`.** `FSSIZE` is too small for a maximum-size file and its index blocks.
- **`panic: freeing free block`, or leaked blocks in `fsinspect check` after `bigfile`.** `itrunc()` frees a block twice, or skips the doubly-indirect part.
- **`open()` of a symlink cycle hangs or deadlocks.** No depth limit, or the link's inode is still locked when `namei()` looks up the target.
- **`usertrap(): unexpected scause 0xd` on the first touch of a mapped page.** The fault never reached your mapping code: in this tree `vmfault()` returns 0 for any address at or above `p->sz`.
- **`panic: freewalk: leaf` when a process with mappings exits.** Mapped pages were not unmapped before `proc_freepagetable()`; they live above `p->sz`, which `uvmfree()` does not cover.
- **`panic: sched locks` in `pipewrite()`.** Your fault handler called `readi()` while the CPU held a spinlock.
- **The file grows after `munmap()`.** Write-back wrote whole pages past the end of the file.
- **`expect` times out waiting for `waiting to be killed`.** The crash point was not reached: k was not less than the number of blocks in the transaction, and the kernel printed `not reached ... disarmed`.
- **The kill leaves QEMU running.** The signal went to `make`, not to QEMU. `xv6crash.exp` starts QEMU itself for that reason.
- **Linux: the mapping never becomes clean in pagecache part 4.** The file is on tmpfs, which has no disk to write to.
- **Linux: `kcmp` says `Operation not permitted`.** In a container, add `--cap-add SYS_PTRACE`.
- **Windows: `undefined reference to PdhOpenQueryA`.** Link with `-lpdh`.
- **Windows: the counters jump by tens of MiB.** Other programs write too. Run twice, and trust the flush timings more than the counters.

## Where this shows up later

- Stage 08 passes open file descriptions between processes with `SCM_RIGHTS`, and NT handles with `DuplicateHandle` into another process (exercise I, part 3). A sealed `memfd` has no disk behind it, so exercise H's writeback question does not arise for it.
- Stage 09's server maps client buffers, and stage 14's dma-buf file descriptors are open file descriptions too.

## Next

Stage 08 of the [build plan](../../SYLLABUS.md#08-ipc-shared-memory-and-event-loops) passes file descriptors as capabilities, seals shared memory, and runs one event loop.
