# Lab 08: IPC, shared memory and event loops

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 08](../../SYLLABUS.md#08-ipc-shared-memory-and-event-loops)

> **Draft for review, 2026-11-16. What was and was not executed:**
> - **Linux parts (exercises A–D): built and run** on 2026-11-16 in an Ubuntu 24.04 container under podman on Windows 11 (a rootful podman machine on WSL 2; not WSL 2 Ubuntu itself): kernel 6.6.87.2-microsoft-standard-WSL2, 16 vCPUs, gcc 13.3.0, strace 6.8, running as root. Two runs are recorded in [`linux/example-output.txt`](linux/example-output.txt). The fixed server in exercise C came from a reference fix that is not shipped.
> - **io_uring (exercise D): not executed.** `io_uring_setup` returned `ENOSYS` in the container, although the kernel has `CONFIG_IO_URING=y` and `io_uring_disabled` is 0, so podman's default seccomp profile refused it. Running without that profile was not done here.
> - **Windows parts (exercises E–H): built and run** natively and unelevated, twice each, with gcc 16.2 from WinLibs. Output: [`windows/example-output.txt`](windows/example-output.txt), with the account name, machine name and SID replaced.
> - **Not executed:** the Process Explorer step of exercise F (Process Explorer is not installed here; the program prints the same facts), and the libwayland reading of exercise I. No libwayland source was available, so the expected answers there come from its public API and documentation as known before this draft, not from the source.
> - **Documentation:** the man-page and Learn statements below were not re-read for this draft; no network access was used. Every behavior in the tables and logs was observed by running the programs.
> - **Machine:** Windows 11 Pro build 26300, Intel Core i7-11700 (16 logical processors), hypervisor present.

## Goal

A compositor is a server that maps memory handed to it by clients it does not trust, and that must never stop for any one of them. This lab builds the three mechanisms that stage 09's headless display server stands on, and their Windows counterparts:

- passing a descriptor over a Unix socket, and knowing who sent it;
- a shared buffer that the receiver can trust not to shrink under it;
- one thread that waits on sockets, a timer, signals and other threads at once.

Then it separates the two ways a kernel can report I/O, readiness and completion, and points you into libwayland's event loop to say which one it uses.

```
client                                       kernel                          server: one thread in epoll_wait
memfd_create("buf", MFD_ALLOW_SEALING)
write pixels; F_ADD_SEALS SHRINK|GROW|SEAL
sendmsg(sock, "BUFFER", SCM_RIGHTS [fd 5])  --> installs a new fd (11) that   socket readable -> recvmsg: fd 11
                                                refers to the same open file   SO_PEERCRED: pid, uid at connect()
                                                description as the client's 5  F_GET_SEALS: SHRINK set? size ok?
                                                                               mmap(PROT_READ, MAP_SHARED)
```

## Files

| File | What it is | Status |
| --- | --- | --- |
| [`linux/fdpass.h`](linux/fdpass.h), [`linux/fdpass.c`](linux/fdpass.c) | send and receive fds with `SCM_RIGHTS`, read `SO_PEERCRED`, count open fds; reusable in stage 09 | built and run here |
| [`linux/sealbuf.h`](linux/sealbuf.h), [`linux/sealbuf.c`](linux/sealbuf.c) | create, seal and check `memfd` buffers; the receiver's check; reusable in stage 09 | built and run here |
| [`linux/evloop.h`](linux/evloop.h), [`linux/evloop.c`](linux/evloop.c) | a small single-threaded `epoll` loop with fd, `timerfd`, `signalfd` and `eventfd` sources, shaped like libwayland's `wl_event_loop`; reusable in stage 09 | built and run here |
| [`linux/fdserver.c`](linux/fdserver.c) | exercise A: a Unix-socket server that receives fds and checks peers | built and run here |
| [`linux/sealtest.c`](linux/sealtest.c) | exercise B: the receiver's check against ten offers, a sealed buffer its sender cannot shrink, an unsealed one that raises `SIGBUS`, and write seals | built and run here |
| [`linux/evserver.c`](linux/evserver.c) | exercise C: one `epoll` loop serving clients, a 60 Hz `timerfd`, a `signalfd` and an `eventfd`, with scripted good and bad clients; it has a leak you fix | built and run here |
| [`linux/readiness.c`](linux/readiness.c) | exercise D: level- vs edge-triggered, writability as back-pressure, regular files, an `io_uring` probe | built and run here; the `io_uring` part was refused by seccomp |
| [`linux/run.sh`](linux/run.sh) | builds everything in `/tmp` and runs exercises A–D without input; runs your fix for exercise C if `SOLUTION_DIR` points at it | run here |
| reference solution (not shipped) | the fix for exercise C's leak | the recorded "your fix for the leak" output came from a reference patch kept outside this repository |
| [`windows/namedpipe.c`](windows/namedpipe.c) | exercise E: a message-mode named-pipe server and the client's identity | built and run here |
| [`windows/dupsection.c`](windows/dupsection.c) | exercise F: a page-file-backed section pushed to the client with `DuplicateHandle` | built and run here |
| [`windows/msgwait.c`](windows/msgwait.c) | exercise G: a GUI thread with a message-only window waiting on four handles, its message queue and APCs with `MsgWaitForMultipleObjectsEx` | built and run here |
| [`windows/iocp.c`](windows/iocp.c) | exercise H (optional): the pipe server as one thread on an I/O completion port | built and run here |
| [`linux/example-output.txt`](linux/example-output.txt), [`windows/example-output.txt`](windows/example-output.txt) | recorded output | recorded 2026-11-16 |

**Reference solutions (not shipped):** the section "exercise C: your fix for the leak" in the Linux output came from a reference fix that is kept out of the repository. Write your own: copy `linux/evserver.c`, fix it, and save the difference as a patch against the shipped file, for example `diff -u evserver.c my-evserver.c > ~/lab08-mine/evserver-fdleak.patch`. Then `SOLUTION_DIR=~/lab08-mine sh linux/run.sh` applies it to a copy in the build directory and runs the demo again. Without it, that step prints `SKIPPED`. The file names in the patch header do not matter, and `.gitignore` keeps `*.patch` and `linux/solution/` out of git.

## Setup

- **Linux:** gcc, plus `strace` and `patch` for the optional steps. WSL 2 Ubuntu from Lab 00 works; its numbers are VM numbers too.

  ```sh
  sh linux/run.sh                                  # everything
  SOLUTION_DIR=~/lab08-mine sh linux/run.sh        # and your fix for exercise C
  ```

  Run as a normal user, `fdserver` skips the two clients that switch to uid 65534, and `evserver`'s "foreign" client says it was skipped: switching uid needs root. The runs recorded here used the container from Lab 05, started from Git Bash with the repository mounted read-only:

  ```sh
  MSYS_NO_PATHCONV=1 podman run --rm --cap-add SYS_PTRACE -e SOLUTION_DIR=/sol \
    -v "C:/path/to/research-design:/work:ro" -v "C:/path/to/your-fix:/sol:ro" oslab:24.04 \
    sh /work/os-foundations/labs/08-ipc-event-loops/linux/run.sh
  ```

  A third run without `--cap-add SYS_PTRACE` traced the server just as well, because `strace` traces its own child.
- **Windows:** any x86-64 MinGW-w64 gcc, in a normal (unelevated) prompt:

  ```powershell
  cd windows
  gcc -O2 -Wall -o namedpipe.exe namedpipe.c -ladvapi32
  gcc -O2 -Wall -o dupsection.exe dupsection.c -ladvapi32
  gcc -O2 -Wall -o msgwait.exe msgwait.c -luser32
  gcc -O2 -Wall -o iocp.exe iocp.c
  ```

  Each program starts its own clients as copies of itself, so each runs unattended. For the Process Explorer step, [Process Explorer](https://learn.microsoft.com/en-us/sysinternals/downloads/process-explorer) from Sysinternals.
- **For stage 09:** `fdpass`, `sealbuf` and `evloop` are written to be copied into your stage 09 server. Each header says what the calls do and what they do not.

## Read first

- [unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html): `SCM_RIGHTS`, `SO_PEERCRED`, the abstract namespace, and the rule that a `SOCK_STREAM` socket must carry at least one byte of data with ancillary data. [cmsg(3)](https://man7.org/linux/man-pages/man3/cmsg.3.html) for building control messages.
- [memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html) and "File Sealing" in [fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html).
- [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html), [timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html), [signalfd(2)](https://man7.org/linux/man-pages/man2/signalfd.2.html), [eventfd(2)](https://man7.org/linux/man-pages/man2/eventfd.2.html), [io_uring(7)](https://man7.org/linux/man-pages/man7/io_uring.7.html).
- Learn: [CreateNamedPipe](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createnamedpipea) (including what its default security descriptor grants), [ImpersonateNamedPipeClient](https://learn.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-impersonatenamedpipeclient), [DuplicateHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle), [Creating Named Shared Memory](https://learn.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory), [MsgWaitForMultipleObjectsEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex), [About Messages and Message Queues](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues), [I/O Completion Ports](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports).
- Lab 03, exercise E (`SIGBUS` under a live mapping), and Lab 06, exercise D (`timerfd`).

## Exercises

Write each prediction in `NOTES.md` before you run anything.

### A. Pass an fd, and check who sent it (Linux)

[`linux/fdserver.c`](linux/fdserver.c) listens on a `SOCK_SEQPACKET` socket and forks its own clients. Each part is one question:

1. A client opens a file, writes `0123456789`, seeks to offset 4 and sends the fd. **Predict:** what offset does the server's fd have, what does it read, and what offset does the client see afterwards?
2. A client opens a file read-only, unlinks it, and sends the fd. **Predict:** can the server read it, write through it, or reopen it read-write through `/proc/self/fd`?
3. Three clients connect to an abstract socket: A as the server's uid, B after switching to uid 65534, C as uid 0 that switches to 65534 after `connect()` and before sending. **Predict** what `SO_PEERCRED` reports for each.
4. Three fds sent to a receiver with room for one; and fds sent with bytes on a `SOCK_STREAM` socket.

Observed in run 1, with paths and one message shortened; run 2 gave the same results ([full output](linux/example-output.txt)):

```
  client: opened /tmp/lab08-fdserver-.../shared.txt as fd 4, inode 63392, offset now 4; sending it
  server: message "file" with fd 5 -> /tmp/lab08-fdserver-.../shared.txt, inode 63392
  server: offset of the received fd: 4
  server: read 3 bytes through it: "456"
  server: my own open() of the same path is fd 6, offset 0: a new open file description
  client: after the server read 3 bytes, my fd 4's offset is 7

  server: fd 5 -> /tmp/lab08-fdserver-.../nameless.txt (deleted)
  server: pread -> 23 bytes: "only the fd leads here"
  server: write through it -> -1, errno 9 (Bad file descriptor): the access mode travelled with the fd
  server: open("/proc/self/fd/5", O_RDWR) -> 6  (a reopen is checked against the file's permissions, not the fd's mode)

  server: peer pid 48 (fork returned 48), uid 0, gid 0; message "A (same uid), sent with uid 0"
  server: SO_PEERPIDFD gave pidfd 5, whose fdinfo says Pid 48
  server: peer pid 49 (fork returned 49), uid 65534, gid 65534; message "B (uid 65534 before connect), ..."
  server: uid 65534 is not mine (0): rejected, connection closed
  server: peer pid 50 (fork returned 50), uid 0, gid 0; message "C (uid 65534 after connect), sent with uid 65534"

  a) 3 fds sent, room for 1: received 1, MSG_CTRUNC set, open fds 8 -> 9
  b) SOCK_STREAM: sent "AB"+fd, "CD", "EF"+fd, "GH"; each recvmsg asks for 100 bytes and up to 4 fds
     recvmsg -> "AB" with 1 fd(s)
     recvmsg -> "CDEF" with 1 fd(s)
     recvmsg -> "GH" with 0 fd(s)
```

What this shows:

- **An fd names an open file description, not a file.** The server's fd 5 and the client's fd 4 shared one offset: the server's read moved the client's offset from 4 to 7. The server's own `open()` of the path made a second description with its own offset. This is the distinction Lab 07's checkpoint asks for, and the reason two processes that share an fd must agree on who seeks.
- **An fd is a capability.** The unlinked file had no name left, yet the server could read it; the read-only mode came with the fd, so `write()` failed with `EBADF`. The capability is not airtight on Linux, though: opening `/proc/self/fd/5` again is an ordinary open of the same inode, checked against the inode's permissions (here as root, but a normal owner of a `0600` file would pass too), and it returned a read-write fd. Exercise B shows how a seal closes that hole for a `memfd`.
- **`SO_PEERCRED` reports the credentials of `connect()`, not of the sender.** Client C sent as uid 65534, and the server still saw uid 0. A process can connect while privileged and then drop privileges, or pass the connected socket itself to another process with `SCM_RIGHTS`; the server keeps seeing the connector. Check the peer at accept time, and treat the result as "who opened this connection".
- **A pid is a weak name.** After the peer exits its pid can be reused. `SO_PEERPIDFD` (Linux 6.5) returned a pidfd that refers to exactly the process that connected.
- **An abstract socket has no permissions.** Any process in the same network namespace can connect to `@lab08-fdserver-45`, so the uid check was the only check. Exercise C uses a socket file in a `0700` directory, and the foreign client never got past `connect()`. Wayland puts its socket in `$XDG_RUNTIME_DIR` for the same reason.
- **Received fds are the receiver's problem.** With room for one fd, the kernel installed one, set `MSG_CTRUNC` and did not install the other two (the open-fd count went up by one). With room for more, every fd the peer sends lands in your table, whether you wanted it or not: exercise C.
- **On a stream socket, fds ride with bytes.** On this kernel a `recvmsg` never returned the bytes of two sends that both carried fds, but it did join the plain `"CD"` to the following `"EF"` that carried one. A protocol on `SOCK_STREAM` must therefore know from its own message format how many fds each message takes, and keep received fds in a queue until a message claims them. Exercise I asks you to find that in libwayland.

### B. A buffer that cannot shrink (Linux)

[`linux/sealtest.c`](linux/sealtest.c) and [`linux/sealbuf.c`](linux/sealbuf.c). If you want to build the check yourself, write your own `sealbuf_check()` from memfd_create(2) and fcntl(2) before you read the shipped one: accept an fd only if it can never shrink and is as large as the sender claims.

**Predict** the verdict for each of these offers, then run `./sealtest` (or `sh linux/run.sh`). Observed, the same in both runs:

| Offer | Receiver's verdict |
| --- | --- |
| a pipe | reject: not a regular file |
| a regular file in `/tmp` | reject: cannot be sealed (`F_GET_SEALS`: `EINVAL`) |
| a POSIX `shm_open()` object (tmpfs) | reject: seals `SEAL`, need `SHRINK` |
| `memfd` without `MFD_ALLOW_SEALING` | reject: seals `SEAL`, need `SHRINK` |
| `memfd`, sealing allowed, no seals | reject: seals none, need `SHRINK` |
| `memfd` sealed `GROW` only | reject: seals `GROW`, need `SHRINK` |
| `memfd` sealed, but claims 16 pages of 4 | reject: claims 65536 bytes but has 16384 |
| `memfd` sealed `SHRINK` | **accept** |
| `memfd` sealed `SHRINK\|GROW\|SEAL` | **accept** |
| `memfd` sealed `SHRINK\|GROW\|WRITE\|SEAL` | **accept** |

memfd_create(2) says a `memfd` created without `MFD_ALLOW_SEALING` starts with `F_SEAL_SEAL`, so no seal can ever be added; the tmpfs file behind `shm_open()` reported the same. That is why a Wayland client that wants to offer a sealed buffer must use `memfd_create`.

**The checkpoint: a sealed buffer cannot be shrunk by its sender.** In part 2 the receiver accepts and maps a 4-page `memfd` sealed `SHRINK|GROW|SEAL`, and then the sender tries everything it has:

```
  sender:   ftruncate to 1 page                 -> EPERM (Operation not permitted)
  sender:   ftruncate to 0                      -> EPERM (Operation not permitted)
  sender:   ftruncate to 8 pages                -> EPERM (Operation not permitted)
  sender:   open(/proc/self/fd/4, O_TRUNC)      -> EPERM (Operation not permitted)
  sender:   F_ADD_SEALS WRITE (after SEAL)      -> EPERM (Operation not permitted)
  sender:   F_GET_SEALS                         -> SEAL|SHRINK|GROW; there is no call that removes a seal
  sender:   writes 0xAA into page 0 through its mapping -> ok (contents may change)
  sender:   punches a hole in page 2            -> ok
  receiver: first byte of each page: 0xaa 0x11 0x00 0x13  (no SIGBUS)
```

Seals live on the inode, not on one fd, so reopening through `/proc` did not help, and seals can only be added. But `SHRINK` protects the size only: the sender rewrote page 0 and punched a hole in page 2, which the receiver then read as zeros.

**What happens with an unsealed one** (part 3): a receiver that only checks `fstat()`'s size accepts an unsealed `memfd`, maps 4 pages, and the sender shrinks it to one page:

```
  a) receiver without a SIGBUS handler
  receiver: fstat says 16384 bytes >= 16384 claimed: looks fine, mapping it
  sender:   ftruncate to 1 page -> ok
  receiver was killed by signal 7 (Bus error)
  b) receiver with a SIGBUS handler
  receiver: SIGBUS (si_code 2, BUS_ADRERR) at offset 0x1000; dropping this client's buffer and carrying on
```

The size check was true when it ran and false when the receiver used it: a time-of-check-to-time-of-use race that the client wins whenever it likes. In a compositor, (a) is every other client's session ending. Lab 03's exercise E showed the same with a plain file.

**Freezing the contents** (part 4; "(Operation not permitted)" after each `EPERM` left out):

```
  F_SEAL_WRITE while the writer still has a shared writable mapping -> EBUSY (Device or resource busy)
  F_SEAL_WRITE after munmap                                    -> ok
  pwrite                                                       -> EPERM
  mmap PROT_WRITE, MAP_SHARED                                  -> EPERM
  mmap PROT_WRITE, MAP_PRIVATE                                 -> ok  (writes go to private copies)
    wrote 'X' through it; the file still starts with 'f'
  reopen /proc/self/fd/3 O_RDWR (ok), then write             -> EPERM
  F_SEAL_FUTURE_WRITE with the writer's mapping still in place -> ok
  the writer writes "frame 2" through its old mapping; a reader's mapping shows "frame 2"
  pwrite                                                       -> EPERM
  a new mmap PROT_WRITE, MAP_SHARED                            -> EPERM
```

`F_SEAL_WRITE` makes the bytes immutable for everyone, including a reopen through `/proc`, but the writer must give up its writable mapping first. `F_SEAL_FUTURE_WRITE` lets the writer that already holds a mapping keep writing while nobody can create a new way to write: the shape of a producer that hands out read-only views. **Decide** which seals a compositor should require for a client's pixel buffer (which the client redraws every frame) and for a keymap the compositor sends to clients (which no client should be able to change for the others), and why.

### C. One `epoll` loop (Linux)

[`linux/evserver.c`](linux/evserver.c) is a server on [`evloop.c`](linux/evloop.c). Its one loop thread waits in `epoll_wait` on the listening socket and every client socket, a 60 Hz `timerfd` (a stand-in for vblank), `signalfd`s for `SIGINT`, `SIGTERM` and `SIGUSR1`, and an `eventfd` that a worker thread posts when it has checksummed a client's buffer (it also sleeps 20 ms per job, standing in for real work). `./evserver demo` runs a scripted series of clients against it:

| Client | What it does | What the server logged (both runs) |
| --- | --- | --- |
| `good` | one sealed 64 KiB buffer, a PING | accepted, checksum after about 20 ms, PONG |
| `unsealed` | an unsealed buffer, then a sealed one claiming twice its size | rejected twice, with the reason |
| `foreign` | switches to uid 65534, then connects | `connect: Permission denied`: the socket's directory is `0700`, so the server never saw it |
| `stall` | three buffers, then asks the server to block for 100 ms | `timerfd read returned 6 expirations: 5 vblank tick(s) went by unseen`; `eventfd read 3 ... 3 job(s) done`; the PONG came before the three OKs |
| `crash` | sends a buffer and exits at once | `hung up`, then 20 ms later `client 4 left before its job finished; result dropped` |
| `fdflood` | 200 PINGs with 2 fds attached to each | 200 PONGs; then at `SIGUSR1`: `open fds 410 (at start 10)` |

Then `SIGUSR1` printed statistics through the `signalfd`, and `SIGTERM` shut the server down cleanly.

**Predict** before you run it: in the `stall` case, how many expirations will the next `timerfd` read return, what will the `eventfd` read return, and in which order will the client get its four replies?

The `strace` section of the output shows the readiness model at work for the `good` client. Run 1, shortened, with notes after `#`:

```
73    epoll_wait(3, [{events=EPOLLIN, data={...}}], 32, -1) = 1          # fd 10 is readable
73    recvmsg(10, {... msg_iov=[{iov_base="\2\0\0\0\0\0\1\0..." ...       # the BUFFER message
73    write(1, "[  147.7 ms] server: client 1 bu"..., 107) = 107
73    recvmsg(10, {msg_namelen=0}, MSG_CMSG_CLOEXEC) = -1 EAGAIN          # nothing more from it
73    epoll_wait(3, [{events=EPOLLIN, data={...}}], 32, -1) = 1          # the timerfd
73    read(8, "\1\0\0\0\0\0\0\0", 8)    = 8                                # one expiration
73    epoll_wait(3,  <unfinished ...>
74    write(9, "\1\0\0\0\0\0\0\0", 8 <unfinished ...>                      # the worker posts the eventfd
73    <... epoll_wait resumed>[{events=EPOLLIN, data={...}}], 32, -1) = 1
74    <... write resumed>)              = 8
73    read(9, "\1\0\0\0\0\0\0\0", 8)    = 8                                # the loop reads it
73    write(1, "[  168.8 ms] server: eventfd rea"..., 86) = 86
73    sendto(10, "e\0\0\0\0\200 ochecksum 6f208000\0\0\0\0\0\0\0"..., 64, MSG_DONTWAIT|MSG_NOSIGNAL, NULL, 0) = 64
-- the control data of the BUFFER message, which those lines cut off:
   msg_control=[{cmsg_len=20, cmsg_level=SOL_SOCKET, cmsg_type=SCM_RIGHTS, cmsg_data=[11]}]
```

`epoll_wait` only said "fd 10 is readable"; the data moved when the loop called `recvmsg`, and with it the client's `memfd`, installed as the server's fd 11. Thread 74 never touched a socket. It wrote 8 bytes to the `eventfd`, and that made the loop thread's `epoll_wait` return.

What this shows:

- **A blocked callback blocks everything.** During the 100 ms stall nothing else ran: no timer read, no other client, no worker result. The `timerfd` read that followed counted 6 expirations, so the server knows exactly how many vblanks it missed. A Windows waitable timer has no such count (exercise G).
- **An `eventfd` is a counter.** Three posts while the loop was blocked became one wakeup and one read of 3, so a loop must drain a queue on each wakeup rather than expect one wakeup per item. A Windows auto-reset event loses the count altogether (exercise G).
- **The worker's results outlive clients.** The `crash` client's job finished after the client was gone. The server looked the client up by id and dropped the result, rather than writing to a freed struct.
- **Signals as data.** `SIGTERM` arrived as a `signalfd_siginfo` with the sender's pid, in the loop, between two other events, not in a handler that interrupted the loop at an arbitrary instruction. The signals are blocked before the worker thread starts, so it inherits the mask (signalfd(2) requires them blocked).
- In run 1 one `timerfd` read during the `fdflood` client also returned 2; in run 2 none did, and the fixed server had none in either run. This was not investigated. One hypothesis to test: the leaking server's fd table grows from 64 to 512 entries during `fdflood`, and growing the table of a multi-threaded process may wait for other CPUs. Check it with `strace -T` or by making the worker a separate process.

**Fix the leak (yours to write).** After `fdflood`, the server holds 400 more fds than it started with: every fd that arrives with a message is installed in the server's table, and nothing closed the ones that came with PINGs. A client can exhaust the server's fd limit this way and stop it from accepting anyone. Change `evserver.c` so that the open-fd count at shutdown equals the count at start after the whole demo, and decide what a client that sends fds it should not send deserves (the fds closed, or the client disconnected; Wayland treats a malformed request as a protocol error that ends the client). Save it as `evserver-fdleak.patch` and run `SOLUTION_DIR=... sh linux/run.sh`. The reference fix, which is not shipped, closes every fd that a message type does not take and counts them:

```
[  379.2 ms] server: client 5 sent 2 fd(s) that message type 3 does not take; closing them (and any later ones)
[  394.1 ms] server: client 5 had sent 400 fd(s) without a use; all were closed
[  444.7 ms] server: statistics: 0.44 s, 26 vblank ticks (27 expected), 1 late timer reads, at most 6 expirations in one read; 0 client(s); open fds 10 (at start 10)
```

**Break it (not run here):** delete the `read()` in `evloop.c`'s timer case and predict what `epoll_wait` does from then on, with level triggering and then with `EPOLLET`. Then remove `pthread_sigmask` from `evloop_add_signal()` and send `SIGUSR1`.

### D. Readiness vs completion (Linux)

[`linux/readiness.c`](linux/readiness.c), observed the same in both runs:

```
  level-triggered (EPOLLIN):
    wrote 10 bytes                 -> epoll_wait reports 1 event(s)
    read 4, 6 left unread          -> epoll_wait reports 1 event(s)
    read until EAGAIN (6 bytes)    -> epoll_wait reports 0 event(s)
  edge-triggered (EPOLLIN | EPOLLET):
    wrote 10 bytes                 -> epoll_wait reports 1 event(s)
    read 4, 6 left unread          -> epoll_wait reports 0 event(s)
    wrote 1 more byte              -> epoll_wait reports 1 event(s)
    read until EAGAIN (7 bytes)    -> epoll_wait reports 0 event(s)

  SO_SNDBUF 212992 bytes; 278 messages sent before send() failed with EAGAIN
  the sender's EPOLLOUT now: 0 event(s)
  the peer had to read 209 of the 278 messages before EPOLLOUT came back

  poll() on a regular file: 1 ready, revents 0x1 (POLLIN is 0x1)
  epoll_ctl(EPOLL_CTL_ADD) on it: -1, EPERM (Operation not permitted)

  io_uring_setup(4 entries) -> -1, ENOSYS (Function not implemented)
  /proc/sys/kernel/io_uring_disabled: 0
  this process's Seccomp mode: 2 (a seccomp filter is installed)
```

- **Readiness** (`epoll`, `poll`, `select`): the kernel says that an operation would not block now; you then perform it, and the data is copied at that moment, into a buffer you choose then. Level-triggered reports a condition (unread bytes are still there), so a callback may read a little and come back. Edge-triggered reports a change (new bytes arrived), so a callback that does not read until `EAGAIN` waits for data that may never come.
- **Writability is back-pressure.** 278 messages of 64 bytes filled a 208 KiB send buffer (each message is charged more than its 64 bytes), and `EPOLLOUT` came back only after the peer had read 209 of them. A server that wants to write to a client that does not read must queue, wait for `EPOLLOUT`, and decide when to give up: stage 09's "never reads its socket" client.
- **Readiness cannot describe a disk.** `poll()` called a regular file always readable, and `epoll` refused it with `EPERM`; epoll_ctl(2) lists that error for fds that do not support `epoll`, a regular file among them. A read that has to wait for a disk still blocks.
- **Completion** (IOCP, overlapped I/O, `io_uring`): you hand the kernel the whole operation and its buffer up front; it tells you when it is done, and the data is already in your buffer. Exercises G and H show it on Windows. The cost is that buffers are committed while operations are pending, and cancellation becomes a protocol of its own. The benefit is that files, sockets and timers all fit one model.
- **`io_uring` was not run here.** The kernel was built with `CONFIG_IO_URING=y` and `io_uring_disabled` was 0, but `io_uring_setup` returned `ENOSYS` inside the container, which fits podman's default seccomp profile refusing the call. On a Linux machine you control, `./readiness` reports the ring sizes; submitting a read is left to io_uring(7).

### E. A named-pipe server (Windows)

```powershell
.\namedpipe.exe
```

[`windows/namedpipe.c`](windows/namedpipe.c) creates `\\.\pipe\lab08-namedpipe-<pid>` in message mode with `FILE_FLAG_FIRST_PIPE_INSTANCE` and `PIPE_REJECT_REMOTE_CLIENTS`, and starts a copy of itself as the client. The client opens the pipe with `SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION`, so the server may read its token but may not act as it. Observed in run 1 (run 2 differed only in PIDs), shortened:

```
  server: GetNamedPipeClientProcessId 23136 (matches CreateProcess), session 1 (mine 1)
  server: first message: "hello from the client" (21 bytes)
  server: impersonated the client at level SecurityIdentification and took its token
  client's token: user <machine>\<user> (S-1-5-21-<domain>-1001), integrity medium (0x2000)
  server's token: user <machine>\<user> (S-1-5-21-<domain>-1001), integrity medium (0x2000)
  server: same user: yes -> accept

  server: next message is 100 bytes (3100 bytes in the pipe in total)
    ReadFile into 64 bytes: 1 x (64 bytes, FALSE, ERROR_MORE_DATA), then 36 bytes, TRUE: end of message
  server: next message is 3000 bytes (3000 bytes in the pipe in total)
    ReadFile into 64 bytes: 46 x (64 bytes, FALSE, ERROR_MORE_DATA), then 56 bytes, TRUE: end of message

  a second instance with FILE_FLAG_FIRST_PIPE_INSTANCE: failed, error 5
  a second instance without the flag:           created (the flag guards only the first instance)
  client: pid 23136; GetNamedPipeServerProcessId says the server is pid 26824
```

- **Who is the peer?** `GetNamedPipeClientProcessId` is the NT counterpart of `SO_PEERCRED`'s pid, and the client's token gives what the uid gives on Linux, plus an integrity level. The server took the token handle while impersonating and reverted at once. A first version of this program looked up the account name while still impersonating at identification level, and the lookup failed (it printed `?\?`).
- **Messages keep their boundaries.** In message mode a short buffer gets `ERROR_MORE_DATA` and the rest of the same message on the next read, never the start of the next one. `PeekNamedPipe` tells you the size first.
- **Squatting.** A process that creates the pipe name first owns it. `FILE_FLAG_FIRST_PIPE_INSTANCE` makes the server fail (error 5) instead of quietly becoming a second instance of someone else's pipe; the client checks the other direction with `GetNamedPipeServerProcessId`. Read on Learn's CreateNamedPipe page what the default security descriptor lets other accounts do, and decide what DACL your server should pass instead.

### F. Hand a section to the client with `DuplicateHandle` (Windows)

```powershell
.\dupsection.exe          # unattended
.\dupsection.exe hold     # stops after part 3 for Process Explorer; press Enter to go on
```

[`windows/dupsection.c`](windows/dupsection.c) creates an unnamed 64 KiB section backed by the paging file. Without a name, no other process can open it: the only way in is a handle the server gives out. It learns the client's PID from the pipe, opens the client with `PROCESS_DUP_HANDLE` and nothing else, and pushes two handles into it, one with `FILE_MAP_READ` and one with `FILE_MAP_READ | FILE_MAP_WRITE`. The values travel over the pipe; they mean nothing in any other process. `NtQueryObject` prints what Process Explorer's handle view would show. Observed in run 2, shortened:

```
  server: section handle               handle 0x00f8  type Section  access 0x000f0007  handle count 1
  server: DuplicateHandle into pid 19692 -> 0x00fc (FILE_MAP_READ), 0x0100 (FILE_MAP_READ | FILE_MAP_WRITE); valid only in that process
  server: section handle               handle 0x00f8  type Section  access 0x000f0007  handle count 3
  client: read-only handle             handle 0x00fc  type Section  access 0x00000004  handle count 3
  client: read-write handle            handle 0x0100  type Section  access 0x00000006  handle count 3
  client: read-only view at 000001E6C86E0000, 65536 bytes, MEM_MAPPED: "page 0, written by the server"
  client: MapViewOfFile(read-only handle, FILE_MAP_WRITE) -> NULL, error 5
  client: DuplicateHandle(read-only handle, FILE_MAP_ALL_ACCESS) -> TRUE, error 0
  client: upgraded copy                handle 0x0108  type Section  access 0x000f001f  handle count 4
  client: MapViewOfFile(upgraded copy, FILE_MAP_WRITE) -> a writable view
  client: locked-down section handle   handle 0x0104  type Section  access 0x00000004  handle count 2
  client: DuplicateHandle(locked-down handle, FILE_MAP_ALL_ACCESS) -> FALSE, error 5
  server: page 1 of my view now reads "page 1, written by the client"
  server: pulled the client's 0x00fc back as 0x0150; CompareObjectHandles with my section: TRUE; with another section: FALSE
  client: closed my handles too; my view still reads "page 0, written by the server" and "page 1, written by the client"
  client: server's pseudo-handle, copied handle 0x0110  type Process  access 0x001fffff  handle count 9
```

The full output is in the [output file](windows/example-output.txt); both runs gave the same results.

- **One object, three handles, two processes.** The handle count rose from 1 to 3 as the server pushed two handles, and `CompareObjectHandles` on a handle pulled back from the client confirmed it was the same section. The client's view and the server's view are the same pages at different addresses, as in Lab 03.
- **A handle's access mask restricts that handle, not the object.** The read-only handle could not be mapped for writing (error 5). But the client asked `DuplicateHandle` for a copy with `FILE_MAP_ALL_ACCESS` and got one, and mapped it writable. That fits an access check against the section's security descriptor whenever more access is asked for than the source handle has: the default descriptor of an object created without one lets its creator's user do everything, and the client runs as that user. The second section, created with a DACL that grants this user only `SECTION_QUERY | SECTION_MAP_READ` (0x5), refused the same upgrade. It is the NT twin of the `/proc/self/fd` reopen in exercise A. A real restriction needs a descriptor that denies it, or a client that runs as another user or at a lower integrity level.
- **Lifetime.** After the server unmapped its view and closed its handle, and the client closed its handles, the client's view still worked: a mapped view keeps the section alive, as an `mmap` keeps a `memfd`'s pages.
- **Push, do not pull.** The client opened the server with `PROCESS_DUP_HANDLE` (allowed: same user) and duplicated the server's own pseudo-handle, `(HANDLE)-1`, into itself. It got a handle to the server process with access `0x1fffff`, everything. Giving another process `PROCESS_DUP_HANDLE` on you gives it all of you, so the server opens the client and pushes, never the other way round.
- **No seals on NT.** Win32 offers no call that shrinks a section, so the `SIGBUS` attack of exercise B has no direct counterpart here, and Lab 03 showed `SetEndOfFile` refused on a mapped file. But the creator keeps its write access, and nothing like `F_SEAL_WRITE` exists: a server that must not see bytes change has to copy them.

**Process Explorer (not executed here):** run `dupsection.exe hold`. In Process Explorer, choose View > Lower Pane View > Handles, select the server and then the client, and find the `Section` handles (they have no name). Compare their handle values and access masks with what the program printed, and check that the counts agree.

### G. One GUI thread waits on handles and messages (Windows)

```powershell
.\msgwait.exe
```

[`windows/msgwait.c`](windows/msgwait.c) starts `msgwait.exe loop` in a new process group, connects to its pipe and writes three messages, then sends it `CTRL_BREAK_EVENT` with `GenerateConsoleCtrlEvent`. The loop process owns a message-only window (`HWND_MESSAGE`, never shown), and its one thread calls `MsgWaitForMultipleObjectsEx` on the event of an overlapped pipe operation, a high-resolution waitable timer every 16 ms, an event set by the console control handler, an event set by a worker thread, its message queue (`QS_ALLINPUT`), and APCs (`MWMO_ALERTABLE`). Observed:

| Part 1, one thread | Run 1 | Run 2 |
| --- | --- | --- |
| `SetEvent` ×3 on an auto-reset event, then wait until timeout | 1 wakeup | 1 |
| `ReleaseSemaphore(3)`, then wait until timeout | 3 wakeups | 3 |
| `PostMessage` ×3, then `PeekMessage` until empty | 3 messages | 3 |
| 2 messages queued and looked at with `PeekMessage(PM_NOREMOVE)`; `MsgWaitForMultipleObjectsEx(..., 200 ms, QS_ALLINPUT, 0)` | `WAIT_TIMEOUT` after 201 ms | after 210 ms |
| the same with `MWMO_INPUTAVAILABLE` | returned at once | at once |

```
[  221.0 ms] pipe: a client connected (pid 26636)
[  329.9 ms] event: the worker's event is set
[  330.3 ms] message: WM_APP+1 from the worker, job 1
[  330.5 ms] APC: job 1's callback ran on thread 28340 inside the alertable wait
...
[  734.7 ms] pipe: overlapped ReadFile completed, 13 bytes already in my buffer: "first message"
...
[  923.2 ms] pipe: the client went away (error 109); waiting for the next one
[ 1126.5 ms] console control: CTRL_BREAK_EVENT, delivered by the handler on thread 7328; this is thread 28340
[ 1127.6 ms] message: WM_QUIT; leaving the loop

  in 0.92 s: 135 waits; waitable-timer wakeups 56 (58 expected at 16 ms); WM_TIMER 59 (92 asked for at 10 ms); worker: 3 event wakeup(s), 3 message(s), 3 APC(s)
```

- **One wait, four mechanisms.** Kernel objects (events, a timer), the thread's message queue, and APCs all ended the same wait. The pipe's event fired when a read had *completed*, with the bytes already in the buffer: inside a loop that looks like `epoll`, Windows overlapped I/O is a completion model.
- **How wakeups count.** An auto-reset event is a flag: three `SetEvent` calls before a wait give one wakeup, less than an `eventfd`, which at least returns the sum. A semaphore counts, and posted messages queue one by one.
- **The "new input" rule.** After the thread had looked at its queue, `MsgWaitForMultipleObjectsEx` without `MWMO_INPUTAVAILABLE` slept the full 200 ms with two messages waiting: it waits for input that is new since the last look. `GetQueueStatus` showed it, `new 0x0000, present 0x0008`. A loop that does not drain its queue, or does not pass `MWMO_INPUTAVAILABLE`, stalls exactly like this.
- **Signals arrive on another thread.** The console control handler ran on thread 7328, which the system created, not on the GUI thread; it only set an event. That is the Windows shape of `signalfd`: turn an asynchronous notification into something the loop waits on.
- **Timers.** The waitable timer woke 56 times where 58 periods of 16 ms fit. Unlike a `timerfd`, it does not say how many periods passed since the last wait; compare with `QueryPerformanceCounter` if you need to know. `SetTimer` with 10 ms delivered 59 `WM_TIMER` messages in 0.92 s, about one per 15.6 ms. That fits Lab 06's ordinary timer and a 15.6 ms tick; the tick itself was not measured here.

### H. Optional: an I/O completion port (Windows)

```powershell
.\iocp.exe
```

[`windows/iocp.c`](windows/iocp.c) opens three overlapped instances of one pipe, associates each with one completion port (the instance number is the completion key), starts a connect on each, and from then on only calls `GetQueuedCompletionStatusEx`. A thread-pool timer posts a "tick" packet every 16 ms with `PostQueuedCompletionStatus`, and the server posts "quit" to itself when the last client has gone. Observed:

```
[   29.4 ms] instance 0: connect completed, client pid 15704
[   29.8 ms] instance 0: read completed, 19 bytes already in the buffer: "client 0, message 1"
...
[  150.0 ms] instance 0: read completed with error 109: client pid 15704 is gone after 4 messages
...
  in 0.29 s: 36 GetQueuedCompletionStatusEx calls returned 49 entries (at most 3 at once); 18 timer packets (18 expected at 16 ms); clients that got every reply: 3 of 3
```

Run 2: 32 calls, 48 entries, at most 3 at once, 17 timer packets; 3 of 3 clients. Every entry was a finished operation with a byte count. Nothing ever asked "is this pipe readable?", and a timer is just one more packet on the same queue. **Explain** what changes if four threads call `GetQueuedCompletionStatusEx` on this port, given that each instance's state machine assumes one operation at a time, and what the port's concurrency value of 1 does.

### I. Read libwayland's event loop (not executed here)

libwayland's source was not available for this draft. Read it in the [wayland repository](https://gitlab.freedesktop.org/wayland/wayland), at a release tag you write down in `NOTES.md`:

| File | Look for |
| --- | --- |
| `src/wayland-server-core.h` | the `wl_event_loop` API: `wl_event_loop_add_fd`, `_add_timer`, `_add_signal`, `_add_idle`, `wl_event_loop_dispatch`, `wl_event_loop_get_fd`, and the `WL_EVENT_READABLE` / `WL_EVENT_WRITABLE` mask |
| `src/event-loop.c` | what `wl_event_loop_dispatch` sleeps in; which kernel object backs each kind of source; level or edge; what happens to a source removed while its event is still in the current batch; how timers are kept |
| `src/wayland-server.c` | `wl_display_run`, and the callback that a client's socket gets (`wl_client_connection_data`) |
| `src/connection.c` | `wl_connection_read` and `wl_connection_flush`: how bytes and `SCM_RIGHTS` fds are kept together on a `SOCK_STREAM` socket |
| `src/wayland-os.c` | the close-on-exec wrappers around `socket`, `recvmsg` and `epoll_create` |
| `src/wayland-shm.c` | what the compositor does when a client shrinks its `wl_shm` pool, and whether it looks at seals |
| `src/wayland-client.c` | why a client reads through `wl_display_prepare_read` and `wl_display_read_events` |

Answer, with file and function names:

1. Which model does `wl_event_loop` use, readiness or completion? Quote the system call and the line where a source's callback is called, and say what that callback receives: "the fd is ready", or "here are the bytes".
2. Compare each kind of source with [`evloop.c`](linux/evloop.c). Which kernel object backs a timer source and a signal source? How many timer fds does it create for ten timers?
3. How does it make removing a source inside a callback safe? Compare with `evloop.c`'s dead list.
4. How does `connection.c` match received fds to the messages that need them? Compare with exercise A's `SOCK_STREAM` result.
5. What does `wayland-shm.c` do about `SIGBUS`, and does it treat a sealed pool differently? Compare with exercise B's two receivers.

What this draft expects for question 1, written from libwayland's public API and documentation and **not checked against the source here**: readiness. `wl_event_loop_add_fd` takes a mask of readable and writable conditions and calls back with the mask that is ready, and the callback then reads or writes the fd itself; `wl_event_loop_get_fd` returns one fd that becomes readable when any source is ready, which is the epoll pattern. Check it: find the `epoll_wait` call in `event-loop.c`, then follow one client request from `wl_client_connection_data` to the `recvmsg` in `connection.c`.

## Linux / NT comparison

- **Handing over a capability.** Linux: the sender puts fds in an `SCM_RIGHTS` message, and the kernel installs new fds in the receiver when it calls `recvmsg`. NT: a process that holds `PROCESS_DUP_HANDLE` on another pushes a handle into it with `DuplicateHandle`, and must then tell it the value. Both carry rights: the fd's access mode, the handle's access mask. Both can be widened when the object allows it: a reopen through `/proc/self/fd`, a second `DuplicateHandle` (exercises A and F).
- **Who is the peer.** `SO_PEERCRED` gives pid, uid and gid as of `connect()`, and `SO_PEERPIDFD` a pidfd. `GetNamedPipeClientProcessId` gives the pid, and impersonation gives the client's token: user, groups, integrity level (exercises A and E).
- **Size and contents.** A `memfd` can be sealed against shrinking, growing and writing (exercise B). A section has a fixed size that Win32 offers no call to shrink, but there is no write seal: its creator can always change it.
- **Waiting.** `epoll` waits for readiness on fds, and a `signalfd`, `timerfd` and `eventfd` turn signals, time and wakeups into fds. `MsgWaitForMultipleObjectsEx` waits on up to 63 handles plus the thread's message queue and APCs. Window messages belong to the thread that created the window ([About Messages and Message Queues](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues)), so the GUI thread must be the one that waits. An I/O completion port waits for completions only, and anything else is posted to it as a packet (exercises C, G and H).
- **Timers.** A `timerfd` read returns the number of expirations; a waitable timer is only signaled or not.

## Checkpoints

Answer these in `NOTES.md`.

1. **Show that a sealed buffer cannot be shrunk by its sender**, with your own run of exercise B part 2. Then show what happens to a receiver that accepted an unsealed buffer (part 3), and say which seals a compositor should require for a client's pixel buffer and for a keymap it sends out.
2. **Explain readiness vs completion** with evidence: exercise D's level- and edge-triggered results and regular files, exercise C's `strace` (`epoll_wait` and then `recvmsg`), and exercise G's or H's overlapped reads with the bytes already in the buffer. Name the cost of each model: who owns the buffer while an operation is pending, and what happens on cancellation.
3. **Read libwayland's event loop and say which model it uses.** Answer exercise I's five questions with file and function names, at a tag you name.
4. Passing an fd or a handle passes a capability. What travels with it (the open file description, the access mode or mask), what does not (a name), where Linux and NT let the receiver widen it, and how a seal or a DACL closes that.
5. Why does `SO_PEERCRED` report the credentials of `connect()`, and what does that mean for a server that checks peers?
6. Fix exercise C's leak and show the open-fd count at shutdown equal to the count at start. Explain why a server cannot refuse fds a client sends, only close them.
7. From exercise G: why did three `SetEvent` calls give one wakeup and three `PostMessage` calls three messages, and when does `MsgWaitForMultipleObjectsEx` sleep although messages are waiting?

## Common failure modes

- **Sending fds with no data.** unix(7) requires at least one byte of real data with ancillary data on a `SOCK_STREAM` socket. `fdpass_send()` refuses an empty payload with `EINVAL`.
- **`F_ADD_SEALS` fails with `EPERM`.** The `memfd` was created without `MFD_ALLOW_SEALING` (it starts with `F_SEAL_SEAL`), or `F_SEAL_SEAL` is already set.
- **`F_ADD_SEALS` with `F_SEAL_WRITE` fails with `EBUSY`.** A writable shared mapping still exists; unmap it first, or use `F_SEAL_FUTURE_WRITE`.
- **The receiver dies with `SIGBUS`.** It checked the size, not the seals (exercise B part 3).
- **The open-fd count grows.** Some message type carries fds that nobody closes. Count entries in `/proc/self/fd` before and after (`fdpass_count_open()`).
- **`SIGTERM` kills the server instead of arriving through the `signalfd`.** The signal was not blocked in every thread. Block it before you create threads, as `evloop_add_signal()` does in the main thread.
- **The loop spins at 100 % CPU.** A level-triggered source was not drained: a `timerfd` or `eventfd` not read, or a client socket with a message left unread.
- **An edge-triggered loop stops hearing a client.** The callback did not read until `EAGAIN`.
- **Ticks go missing.** A callback blocked. The `timerfd` expiration count says how many.
- **`kill` on `strace` does nothing.** `strace -o FILE PROG` blocks fatal signals by default; this draft's first `run.sh` hung on exactly that. Signal the traced program instead.
- **`io_uring_setup` returns `ENOSYS` in a container** although the kernel supports it. The container's seccomp profile refuses it.
- **Windows: the account lookup fails while impersonating.** At identification level the thread cannot act as the client; take the token, `RevertToSelf()`, then look things up.
- **Windows: `MsgWaitForMultipleObjectsEx` sleeps with messages in the queue.** Pass `MWMO_INPUTAVAILABLE`, or drain the queue with `PeekMessage` every time.
- **Windows: a handle value from another process is used directly.** Handle values are per process: in this process the same number is invalid or, worse, names some unrelated object. Only a handle duplicated *into* this process is valid here.
- **Windows: `GenerateConsoleCtrlEvent` reaches the wrong processes.** This draft sends `CTRL_BREAK_EVENT` to a process started with `CREATE_NEW_PROCESS_GROUP`, so that only that group gets it; read Learn's GenerateConsoleCtrlEvent page before you try Ctrl+C (not tried here).

## Where this shows up later

- Stage 09 builds its headless display server on `fdpass`, `sealbuf` and `evloop`, ticks vblank with the `timerfd`, and must survive the clients sketched here: one that crashes, one that never reads (exercise D's back-pressure), one that sends an unsealed buffer and shrinks it.
- Stage 11's `wl_shm` pools are shared-memory fds, usually `memfd`s, passed with `SCM_RIGHTS`, and a `wl_keyboard` keymap arrives as an fd.
- Stage 14 passes dma-buf and `sync_file` fds the same way, and waits on fences.
- Stage 17 turns exercises B and C into attacks: unsealed buffers, fd floods, flooded requests.

## Next

Stage 09 of the [build plan](../../SYLLABUS.md#09-part-i-project-a-headless-display-server) is the Part I project: a headless display server with its own protocol.
