# Lab 09: Part I project, a headless display server

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 09](../../SYLLABUS.md#09-part-i-project-a-headless-display-server) | [Protocol](PROTOCOL.md)

> **Draft for review, 2026-11-16. What was and was not executed:**
> - **The harness (six clients, `check.py`, `run.sh`): built and run** on 2026-11-16 in an Ubuntu 24.04 container (image `oslab:24.04`) under podman on Windows 11. The podman machine is a WSL 2 VM, so this was not WSL 2 Ubuntu itself, and every timing below is VM timing: kernel 6.6.87.2-microsoft-standard-WSL2, 16 vCPUs, gcc 13.3.0, Python 3.12.3. The server it checked was a reference implementation that is not shipped. Three runs, all passing, are in [`harness/example-output.txt`](harness/example-output.txt). The VM's one-minute load average was 0.6–0.7 when they started.
> - **The harness against broken servers: run.** The reference was also built 14 times with one deliberate bug each (see [What the harness catches](#what-the-harness-catches)). Every one of them failed, each on the check meant for its bug. Those builds are not shipped either; their output is in the same file.
> - **Not executed:** anything on WSL 2 Ubuntu itself (the steps are the same, but were not tried); a run with the machine under CPU load (exercise E); the design note, the Wayland comparison and the extension exercises, which are yours to write. The graphics course's T4 compositing module does not exist yet, so only the blend stub was used.
> - **Machine:** Windows 11 Pro build 26300, Intel Core i7-11700 (16 logical processors). Other containers were running on the same machine. All numbers are specific to this machine and these runs.

## Goal

Part I ends with one program that needs everything it taught: a display server without a display. Clients connect over a Unix socket, hand it sealed shared-memory buffers with `SCM_RIGHTS`, and say what changed with attach, damage and commit. At every simulated vblank, sixty times a second, the server composes their surfaces into an offscreen frame if anything changed, writes that frame to an image file, and tells each client that asked that its frame is done. Then the bad clients arrive. One crashes in the middle of a frame, one never reads its socket, one sends damage outside its buffer, one shrinks its buffer under the server. The server must survive all of them and keep its 60 Hz cadence while they misbehave.

```
 good-anim (A) ──────┐  Unix stream socket: requests ->, <- events
 good-alpha (B) ─────┤  sealed memfds ride along as SCM_RIGHTS ancillary data
 bad-* (C N R D U) ──┘
                     │
                     ▼  one thread, one epoll loop: listening socket, client sockets,
                        a timerfd at 60 Hz, a signalfd for SIGTERM
  per client: input bytes, fd queue, objects, a bounded output queue
                     │  commit: copy the damaged pixels out of the client's memfd, release the buffer
                     ▼
  surfaces (server copies) ── vblank n: compose if anything changed ── frames/frame-n.ppm
                                └── frame_done(n, time of vblank n) to each surface that asked
```

This is Lab 06's 60 Hz wakeup, Lab 03's `SIGBUS`, and stage 08's fd passing, sealing and event loop, put to work against clients that do not cooperate. Stage 08's primitives are taught in [`labs/08-ipc-event-loops/`](../08-ipc-event-loops/); this project assumes you have done them.

**Hours:** 20–35. **Runs on:** WSL 2 (any Linux with gcc and python3; no display needed).

## The brief

Write a server, in C, that:

1. implements the [protocol](PROTOCOL.md): framing, objects and ids, `hello` and version negotiation, `sync`, surfaces and buffers, and errors;
2. accepts a buffer only as a memfd sealed against shrinking, and only if it is large enough;
3. keeps state per client, and frees all of it (objects, mappings, queued fds) whatever way the client leaves;
4. applies attach, damage and commit atomically, copies the damaged pixels, clips damage safely, and releases buffers;
5. composes the surfaces into a 320 × 240 offscreen frame with the premultiplied ARGB8888 blend stub, in stacking order;
6. ticks a simulated 60 Hz vblank with `timerfd`, and sends `frame_done` with the frame's sequence number and vblank time;
7. writes changed frames as PPM files;
8. never blocks on a client, bounds what it queues for each one, and survives `SIGPIPE`;
9. exits cleanly on `SIGTERM`.

Then run the harness until it passes three times in a row, and write the design note and the one-page Wayland comparison.

## Files

| File | What it is | Status |
| --- | --- | --- |
| [`PROTOCOL.md`](PROTOCOL.md), [`PROTOCOL.zh-TW.md`](PROTOCOL.zh-TW.md) | the protocol your server implements, version 1 | specification |
| [`harness/lab09-protocol.h`](harness/lab09-protocol.h) | the protocol's numbers as C names; your server may include it | built here |
| [`harness/clientlib.c`](harness/clientlib.c), [`clientlib.h`](harness/clientlib.h) | what the clients share: connect, send with an fd, read events, make sealed or unsealed memfds | built and run here |
| [`harness/good-anim.c`](harness/good-anim.c) | client A: one opaque surface, two buffers, a commit per `frame_done`; changes its picture on command; its timestamps measure the cadence | built and run here |
| [`harness/good-alpha.c`](harness/good-alpha.c) | client B: a translucent surface in a buffer at an offset, and an XRGB8888 surface with junk in the top byte; asks for version 7 | built and run here |
| [`harness/bad-crash.c`](harness/bad-crash.c) | bad client C: dies by `SIGKILL` with half a commit sent, an unused fd in flight and an unattached buffer | built and run here |
| [`harness/bad-noread.c`](harness/bad-noread.c) | bad clients N and R, one program in two modes: N floods requests and never reads; R shuts down its read side, so every reply fails with `EPIPE` | built and run here |
| [`harness/bad-damage.c`](harness/bad-damage.c) | bad client D: damage outside the buffer, including an `x + width` that overflows 32 bits | built and run here |
| [`harness/bad-unsealed.c`](harness/bad-unsealed.c) | bad client U: an unsealed memfd shrunk to 0 after it was accepted, and a sealed one too small for its size | built and run here |
| [`harness/check.py`](harness/check.py) | starts your server and the clients, and checks survival, cadence, resources and every compared frame; standard library only | run here |
| [`harness/run.sh`](harness/run.sh) | builds the clients (and your server), then runs `check.py` | run here |
| [`harness/example-output.txt`](harness/example-output.txt) | three passing runs and the 14 broken builds | recorded 2026-11-16 |
| reference solution (not shipped) | a server that passes, and the 14 broken builds of it | the recorded output came from it |

Reference solutions (not shipped): the passing runs recorded here came from a reference server kept outside the repository, and so did the 14 broken builds. Write your own. To run the harness on your server, put its source in `solution/` next to this README (git ignores it), as a `Makefile` whose default target builds a program called `server`, or as `.c` files that compile together with `gcc -O2 -Wall -Wextra`. Or point `SOLUTION_DIR` at the directory, or pass a program you built yourself: `sh harness/run.sh ./server`. Without a server, `run.sh` builds the clients and says it skipped the check.

## Why the protocol is fixed

The syllabus asks for "a small protocol you design". This lab publishes one instead, [PROTOCOL.md](PROTOCOL.md), and your server implements it. The reasons:

- **A harness can only check a server it can talk to.** With a fixed wire format, the bad clients can be exact: a commit cut off after 6 of its 8 header bytes, a damage rectangle whose `x + width` overflows `int32`. The checker can also predict every pixel of every frame.
- **The hard part of this stage is the server, not the message layout.** Per-client state, back-pressure, never blocking and frame pacing are all still your design. PROTOCOL.md says what must be true on the wire, not how to get there, and it names the choices it leaves to you, such as whether a client that does not read is disconnected or throttled.
- **Protocol design moves to versioning.** Exercise A asks you to design version 2, and the harness's version 1 clients must keep passing against it. Client B already asks for version 7, so your server has to negotiate down from the start.
- **It is close to Wayland on purpose, and differs on purpose.** It has the same header layout, fds out of band, attach, damage, commit, frame and release. It differs in a few places: one buffer per memfd, sealing required, damage that decides what is copied, `frame_done` as an event on the surface, and one version for the whole connection. That gives the comparison something to say.

If you want your own protocol anyway: the clients do all their encoding through `clientlib.c` and `lab09-protocol.h`. Port those two files, plus the messages `bad-crash.c` and `bad-noread.c` build by hand, and the rest of the harness still applies. That path was not tried here.

## Setup

- WSL 2 Ubuntu 24.04 from Lab 00, or any Linux with gcc and python3. No other package, no display, no root.
- The runs recorded here used the container from earlier labs, from Git Bash, with this repository and the reference mounted read-only:

  ```sh
  MSYS_NO_PATHCONV=1 podman run --rm \
    -v "C:/path/to/research-design:/work:ro" -v "C:/path/to/your/server:/solution:ro" oslab:24.04 \
    sh -c 'SOLUTION_DIR=/solution sh /work/os-foundations/labs/09-headless-display-server/harness/run.sh'
  ```

  `run.sh` builds in a new directory under `/tmp` and never writes to this folder. Each run takes about 10 seconds.

## Read first

- [PROTOCOL.md](PROTOCOL.md), all of it, before you write any code.
- Stage 08, in [`labs/08-ipc-event-loops/`](../08-ipc-event-loops/): `SCM_RIGHTS`, sealed memfds, and one `epoll` loop with a `timerfd` and a `signalfd`. [unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html): ancillary data as a barrier in a stream, `MSG_CTRUNC` and the fds it closes, `SO_PEERCRED`, and `EPIPE`, `SIGPIPE` and `MSG_NOSIGNAL`. [memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html) and the seals in [fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html). [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html), [timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html).
- [Lab 03](../03-virtual-memory/README.md), exercise E: `SIGBUS` from a file shrunk under a mapping, and the sealed memfd that cannot shrink. [Lab 06](../06-scheduling-latency/README.md), exercise D: how late a 60 Hz `timerfd` wakeup runs in this VM.
- Wayland: `wl_surface`, `wl_buffer`, `wl_shm` and `wl_callback` in the [protocol appendix](https://wayland.freedesktop.org/docs/html/apa.html), and the wire format in [Wayland Protocol and Model of Operation](https://wayland.freedesktop.org/docs/book/Protocol.html).

## The checklist

| # | Requirement | Where it is specified | How the harness checks it |
| --- | --- | --- | --- |
| 1 | Listen, `hello`/`welcome`, negotiate the version down | PROTOCOL.md, Running the server, Versions | setup: A asks for 1 and must get 1; B asks for 7 and must get 1 to 7; both get their first `frame_done` |
| 2 | Accept only sealed, large-enough buffers | Buffers | unsealed phase: both of U's buffers refused with `bad_buffer`, connection closed, server alive |
| 3 | Free everything a client leaves behind | Wire format (fd queue), Errors | after every bad phase: the server's fd count and memfd mappings (from `/proc/PID`) back to the baseline |
| 4 | Attach, damage, commit, copy, release | Commit | A's partial redraw; D's three states; at the end A's releases equal its attaches |
| 5 | Clip damage without overflow | Commit (damage) | D state 1: exactly two small regions updated (4 × 4 in a corner, 4 × 5 on the top edge), nothing else |
| 6 | Compose with the blend stub, in z order, with stride, offset and XRGB right | Buffers, Composition | every compared frame is composed again by `check.py` from the clients' known pictures; ±1 per channel |
| 7 | 60 Hz `timerfd` vblank, `frame_done(seq, time)` | Vblank | each phase: A's `frame_done` events, the vblanks it missed, how late each arrived, and whether the times sit on the 60 Hz grid |
| 8 | Changed frames as PPM, written before their `frame_done`, renamed into place | Running the server | each compared frame is the newest file at or before the `seq` the checker needs |
| 9 | Never block on a client; bounded queues | Errors (back-pressure) | crash phase: C stalls 500 ms with half a message sent; noread phase: N floods for 2 s, then the cadence and the server's peak memory (`VmHWM`) are checked |
| 10 | Survive `SIGPIPE` | Errors | noread phase: R has shut its read side, so the server's first reply fails with `EPIPE` |
| 11 | Clean exit on `SIGTERM` | Running the server | end: exit status 0 within 3 s |

The harness does not check your limits (objects, buffer memory, fds in flight) or a client that sends garbage headers or thousands of fds. Exercise B and stage 17 do.

Where everything is drawn, one character per 16 × 16 pixels:

```
      x 0   80  160 240 320
y   0  ....................     A   good-anim         (16, 16)   160 × 120  z 0  opaque, stride 168 px
   16  .AAAAAAAAAA...22222.     1   good-alpha B1     (96, 64)   128 × 96   z 1  translucent, offset 4096
   32  .AAAAAAAAAA...22222.     2   good-alpha B2     (232, 16)   64 × 48   z 2  XRGB8888, junk top byte
   48  .AAAAAAAAAA...22222.     C   bad-crash         (200, 150)  96 × 72   z 3
   64  .AAAAA11111111......     N   bad-noread flood  (120, 176)  48 × 48   z 3
   80  .AAAAA11111111......     D   bad-damage        (16, 160)   64 × 64   z 3
   96  .AAAAA11111111.uuuu.     u   bad-unsealed      (250, 100)  48 × 40   z 3  must never appear
  112  .AAAAA11111111.uuuu.
  128  .AAAAA11111111.uuuu.
  144  ......111111CCCCCCC.
  160  .DDDD.......CCCCCCC.
  176  .DDDD..NNNN.CCCCCCC.
  192  .DDDD..NNNN.CCCCCCC.
  208  .DDDD..NNNN.CCCCCCC.
  224  ....................
```

## Your project, in milestones

A suggested order. Run the harness after each step, with `CHECK_ARGS="--only crash"` (or another phase) once the good phase passes.

1. **Skeleton.** The listening socket, the `epoll` loop, the `timerfd` on an absolute 60 Hz grid, the `signalfd`, and a frame file of background only at the first vblank. Log how late each tick is, as in Lab 06.
2. **Framing.** Per-client input buffer, whole messages only, `hello`/`welcome`, `sync`/`done`, an object table, `error` followed by a close.
3. **Buffers.** The fd queue, the seal and size checks, a read-only mapping, the fd closed.
4. **Surfaces.** Pending and current state, `commit`, the copy with clipped damage, `release`, composition, PPM files, `frame_done`. The good phase should now pass.
5. **The bad clients,** one at a time: `--only crash`, `--only noread`, `--only damage`, `--only unsealed`.
6. **Three full passing runs,** then the design note.

## Run the harness

```sh
sh harness/run.sh                         # your server from ./solution
SOLUTION_DIR=~/lab09 sh harness/run.sh    # or from elsewhere
sh harness/run.sh ./server                # or a program you built
CHECK_ARGS="--only damage" sh harness/run.sh
python3 harness/check.py --help           # every option and default
```

`check.py` runs five phases against one server process. A and B stay connected throughout, and A commits once per `frame_done`, so every vblank should reach it. After each phase it uses A as a barrier: A shows a new picture, the checker waits three more vblanks, and then it notes which frame file to compare. That way a client that just died has had time to disappear. Frames are compared after the run, so that composing the expected images costs no cadence.

### Tolerance

For each phase, the checker reads A's log. A `frame_done` is late if it reached A more than `--late-ms` (8 ms) after the vblank time it names. It counts the vblanks A missed from the gaps in `seq`. A phase passes with at most `--max-late` (3) late events and `--max-skipped` (2) missed vblanks. The lateness includes the server's wakeup, composition, the send and A's own wakeup. These defaults leave room for a single scheduling hiccup in a VM. Lab 06 measured 60 Hz `timerfd` wakeups in this container with tails of 4–15 ms under load. In one exploratory run while this lab was written (not in the recorded file), one `frame_done` arrived 19.5 ms late and one vblank was missed. A server that stalls fails by far, as the next table shows. On real hardware, tighten them. The noread phase also allows the server's peak resident memory to grow by `--max-growth-mib` (16 MiB).

### What the recorded runs show

Three runs of the reference, back to back (`sh harness/run.sh` with `SOLUTION_DIR`), [recorded](harness/example-output.txt):

| Phase | Length | A's frame_done events | Missed vblanks | Late (> 8 ms) | Lateness, median | Max |
| --- | --- | --- | --- | --- | --- | --- |
| good | 1.17 s | 70 / 70 / 70 | 0 / 0 / 0 | 0 / 0 / 0 | 0.29 / 0.24 / 0.26 ms | 1.14 / 1.03 / 1.03 ms |
| crash | 0.62 s | 37 / 37 / 37 | 0 / 0 / 0 | 0 / 0 / 0 | 0.27 / 0.27 / 0.30 ms | 0.94 / 0.93 / 1.27 ms |
| noread | 2.58 s | 155 / 155 / 155 | 0 / 0 / 0 | 0 / 0 / 0 | 0.23 / 0.21 / 0.21 ms | 0.81 / 1.09 / 0.72 ms |
| damage | 0.13 s | 8 / 8 / 8 | 0 / 0 / 0 | 0 / 0 / 0 | 0.60 / 0.57 / 0.62 ms | 1.01 / 0.78 / 0.93 ms |
| unsealed | 0.08 s | 5 / 5 / 5 | 0 / 0 / 0 | 0 / 0 / 0 | 0.19 / 0.14 / 0.20 ms | 0.67 / 0.79 / 0.71 ms |

- **All 43 checks passed in all three runs.** Every one of the 10 compared frames matched, and the server wrote 15 frame files per run, one for each change. Its own log reported 0 missed vblanks, and its latest tick came 0.32–0.67 ms after the due time.
- **The noread phase is the cadence test that matters:** 2.6 s with a client writing as fast as it can. The reference disconnected N after N had written 369–422 KB. N saw `ECONNRESET` in all three runs, because the server closed the socket with N's requests still unread; some earlier runs while this lab was written saw `EPIPE` instead. The server's peak memory stayed at its 2.4–2.5 MiB baseline. R was disconnected in every run, and the server was not hurt.
- **The damage and unsealed phases are short** (5–8 vblanks). Their cadence numbers cover only that window. The damage phase has the highest medians (0.57–0.62 ms); why was not investigated.
- **The crash phase includes C's 500 ms** with half a commit sent. The server keeps the 6 bytes and goes back to `epoll_wait`, so A saw no gap.
- **These are VM numbers** from a lightly loaded machine, and they moved between recordings: other sets of three runs while this lab was written had medians from 0.10 to 0.8 ms and maxima up to 1.6 ms. A's lateness includes composition, a frame file, a send and a second process's wakeup, so it sits above Lab 06's idle `timerfd` medians (41–110 µs).

### What the harness catches

The same reference built with one deliberate bug each (`-DBUG_...`, not shipped), run once each:

| Bug | What failed |
| --- | --- |
| blocking sockets and `send` | noread cadence: 119 vblanks missed, a 2.0 s gap (the server sat in `send` to N until the checker killed N) |
| a blocking read to finish a partial message | crash cadence: 29 vblanks missed, a 501 ms gap (C's 500 ms pause) |
| an output queue without a limit | noread memory: `VmHWM` grew by 1,076 MiB in 2 s |
| `SIGPIPE` not ignored, no `MSG_NOSIGNAL` | server killed by `SIGPIPE` when R connected |
| no seal check | U's unsealed buffer accepted; server killed by `SIGBUS` at its commit, after U truncated it |
| no size check | U's unsealed buffer refused, the too-small one accepted; server killed by `SIGBUS` at its commit |
| damage not clipped | server killed by `SIGSEGV` at D's state 1 |
| damage clipped with 32-bit sums | D state 1: 16 pixels wrong, the 4 × 4 corner whose `x + width` overflowed |
| damage ignored (always a full copy) | D state 1: 4,060 pixels wrong |
| `stride` taken as `4 × width` | 10 frames wrong: A shows its magenta padding, and B1 over it differs too |
| buffer `offset` ignored | 10 frames wrong: all of B1 that is visible |
| XRGB8888's top byte used as alpha | 10 frames wrong: 3,017 of B2's 3,072 pixels |
| queued fds not closed when a client dies | fds 10 instead of 9 after the crash phase, and in every phase after it |
| buffers not unmapped when a client dies | memfd mappings 6–7 instead of 4 |

Read one failure in full before you need it. The recorded file shows how each one looks.

## Deliverables

1. **Source code** of your server, building with `make` or with the single `gcc` line `run.sh` uses, with no library beyond libc.
2. **Three consecutive passing runs** of `sh harness/run.sh`, the whole output with the cadence tables, and the `server.log` of one of them.
3. **A design note of 1–2 pages** that follows the outline below.
4. **The Wayland comparison, one page,** following the outline after it.

### Design-note outline

1. **Architecture.** The loop, every fd in it, what each event does, and the per-client state, as a diagram.
2. **Message handling.** Framing, the fd queue, how each request is validated, and your error policy.
3. **Never blocking, and back-pressure.** How much input you handle per wakeup, your output queue limit, what happens to a client that does not read, and why. Use numbers from your noread phase.
4. **Frame pacing.** How the timer is set up, what you do when the timer reports more than one expiration, and the distribution of your tick lateness (median, p99, max) next to Lab 06's.
5. **Buffer ownership and safety.** Seals, sizes, when you read client memory and when you release it, and why `SIGBUS` cannot reach your server. Say whether the compositor or the T4 module would convert encoded to linear values ([graphics interface](../../SYLLABUS.md#interface-with-the-graphics-course)).
6. **One bug you diagnosed.** The symptom, your first hypothesis, the measurement or log that confirmed or refuted it, and the fix.
7. **Limits and next steps.** Which stage 17 attack clients would break your server today.

## The Wayland comparison

One page that compares your protocol with Wayland's `wl_surface` attach, damage, commit and frame callback. Cite the [protocol appendix](https://wayland.freedesktop.org/docs/html/apa.html) and the [wire format](https://wayland.freedesktop.org/docs/book/Protocol.html) for every Wayland claim. Points to cover, each checked against those pages on 2026-11-16:

- **Wire format.** Both protocols use a sender or target id word, then a word with the size (header included, at least 8) in the upper 16 bits and the opcode in the lower 16. Fds travel in ancillary data, in the order of the messages that carry them. Wayland clients allocate ids in [2, 0xfeffffff] and the server in [0xff000000, 0xffffffff]. This protocol has no server-allocated ids. What would you need them for?
- **Buffers.** `wl_shm.create_pool` hands over an fd that the compositor maps. A pool holds several buffers, and `wl_shm_pool.resize` can only grow it. Nothing in `wl_shm` requires seals. Here each buffer has its own memfd, and the seal is mandatory. Find out what libwayland-server's `wl_shm` code does when a client shrinks a pool under it, and compare the two approaches.
- **Attach and release.** In Wayland the compositor may read the buffer at any time after the commit, until it sends `wl_buffer.release`. Here the server copies at commit and releases at once. What does each cost in copies, memory and latency? Attaching a null buffer removes the content in both. `wl_surface.attach` also has x and y arguments, which must be zero from `wl_surface` version 5 on, when `wl_surface.offset` took over. Both move the new buffer relative to the old one. Here the client places its surface on the output with `set_position`. Find which Wayland request, if any, lets a client choose where its window appears on the output, and explain why.
- **Damage.** `wl_surface.damage` is in surface coordinates, and the appendix tells new clients to use `wl_surface.damage_buffer`, in buffer coordinates, instead. Pending damage is a union, and the parts outside the surface are ignored. Here damage is in buffer coordinates and is exact: it decides which pixels are copied. In Wayland it is information about what changed. Which design lets a compositor upload less, and which one lets it trust less?
- **Commit.** Surface state is double-buffered in both, and Wayland applies the buffer before the rest of the state. Is the order of the steps in this protocol's commit the same?
- **Frame callbacks.** `wl_surface.frame` creates a `wl_callback` that fires once, when it is a good time to draw the next frame. Its data is a time in milliseconds with an undefined base, and the compositor should not fire it for a surface that is not visible. Here `frame_done` is an event on the surface with the vblank's sequence number and its `CLOCK_MONOTONIC` time in microseconds, and it fires even for a hidden surface. What can a client do with a sequence number that it cannot do with Wayland's time? Stage 18's `presentation-time` answers part of this.
- **Errors, sync and ids.** `wl_display.error` is fatal in both. `wl_display.sync` returns a `wl_callback` where this protocol returns `display.done`. Wayland confirms each freed client id with `wl_display.delete_id`. Which race does that prevent, and why can this protocol free an id at once?
- **Versions.** Wayland versions each interface separately: a global advertises its highest version, and `wl_registry.bind` picks the version of the new object. Here one version covers the whole connection. What does per-interface versioning buy a compositor with dozens of extensions?

Close with the stage 11 table: every message of this protocol next to the Wayland request or event that does its job, or "none".

## Extension exercises

- **A — Version 2.** Design and implement version 2. Examples: `surface.set_opacity`, a `frame_done` that also reports how late the frame was, or a `buffer` that lives in a pool. Negotiate it, write a version 2 client, and show that the harness's version 1 clients still pass.
- **B — More attack clients (a preview of stage 17).** A header with size 0 or 65532, a thousand `create_surface` requests, fds sent with no request to take them, 10,000 damage rectangles in one commit, buffers of 4096 × 4096 until something gives. Add each one to your copy of `check.py` as a phase. What limit stops each one?
- **C — Zero copy.** Compose straight from the client's mapping, keep the buffer until the frame that used it is written, and release it then. Measure the copy time you saved, and explain what the seal now protects and what it does not.
- **D — Damage in the output.** Recompose only the damaged regions of the frame and report the pixels composed per frame. A static desktop should cost almost nothing.
- **E — Under load.** Run the harness with a CPU hog on every core (Lab 06's load threads), with your server at `SCHED_OTHER` and then `SCHED_FIFO`. How many late `frame_done` events does each give? Not run here.
- **F — The real compositing module.** When the graphics course's T4 module exists, replace the stub with it, so that blending happens in linear light. Change `check.py`'s `compose()` to match, and find a pixel of B1 over A where the two results differ by more than 1.
- **G — A deadline.** Compose at vblank minus a margin instead of at vblank, report frames that miss their deadline, and measure how small the margin can get in this VM.

## Self-review

Before you hand it in, check that:

- `run.sh` passes three times in a row, and once more with `--only` for each phase.
- Every policy in the design note exists in the code (limits, what happens to a client that does not read), and the code has none that the note lacks.
- `strace -f -e trace=sendmsg,recvmsg,sendto,recvfrom` on your server during the noread phase shows every call returning data or `EAGAIN`, never sleeping. In a container that needs `--cap-add SYS_PTRACE`.
- The server builds with `-Wall -Wextra` and no warnings.
- `server.log` says, for every disconnect, which client and why.

## Checkpoints

Answer these in `NOTES.md`.

1. Show that your server survives every bad client and keeps its frame cadence: three passing runs, with the spread of their cadence numbers.
2. For each bad behaviour (crash with half a message, flood, read side shut, damage outside the buffer, unsealed buffer, buffer too small), name the system call or signal that would have hurt a naive server, and point to the line of yours that prevents it. Remove one fix, run that phase with `--only`, and show the failure.
3. Where can your server still block? Go through every system call in the loop. Writing a frame file is one: what happens when the disk is slow or full, and how would you take it off the loop?
4. Compare your tick lateness (median, p99, max) with Lab 06 exercise D, and explain any difference.
5. Hand in the one-page Wayland comparison.

## Common failure modes

- **`server died: killed by SIGPIPE` in the noread phase.** The server wrote to a socket whose peer cannot read. Ignore `SIGPIPE`, or send with `MSG_NOSIGNAL`, and treat `EPIPE` as a disconnect.
- **`server died: killed by SIGBUS` in the unsealed phase.** You accepted a buffer without `F_SEAL_SHRINK`, or one smaller than `offset + stride × height`, and then read it.
- **The noread cadence fails with a gap of about 2 s.** The server blocked in `send` to N until the checker killed N. Make every client socket non-blocking.
- **The crash cadence fails with a gap of about 500 ms.** The server waited for the rest of C's half-sent commit. Keep partial messages in the per-client buffer and go back to `epoll_wait`.
- **The noread memory check fails.** Your output queue has no limit.
- **`fds back to the baseline` fails after the crash phase.** You did not close the fds left in a dead client's queue, or a buffer's fd after mapping it.
- **A's frames show magenta stripes on the right.** You used `4 × width` as the stride. A's padding is magenta.
- **B1 is wrong everywhere and the rest is right.** You ignored `offset`.
- **B2 shows holes or tints.** You blended XRGB8888's top byte as alpha.
- **D state 1 fails with 16 pixels wrong.** `x + width` overflowed in 32 bits. With about 4,060 wrong, you copied the whole buffer and ignored the damage.
- **`no frame file at or before seq`.** Frames are missing, misnamed, or written after their `frame_done`.
- **`the server did not create ... display.sock`.** Check the arguments: `--socket PATH --out DIR`.
- **`frame_done times on the 60 Hz grid` fails, or the lateness is negative.** The time is not the nominal vblank time on `CLOCK_MONOTONIC` in microseconds, or its two halves are swapped.

## Where this shows up later

- **Stage 11** maps every message of this protocol to the Wayland request that does its job.
- **Stage 13** replaces the `timerfd` with real page-flip events; the `frame_done` path stays.
- **Stage 14** replaces memfds with dma-bufs, and the copy with fences.
- **Stage 17** attacks the compositor, and its test script starts with these bad clients.
- **Stage 18's** compositor has the same per-client state, the same never-block rule and the same frame loop, on DRM/KMS.

## Next

Part II begins with [stage 10](../../SYLLABUS.md#10-window-system-architecture-measured-on-windows): the DWM presentation path on Windows, measured with PresentMon.
