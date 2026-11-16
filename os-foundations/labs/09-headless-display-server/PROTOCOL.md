# Lab 09 protocol, version 1

**English** | [繁體中文](PROTOCOL.zh-TW.md) | [Lab 09](README.md)

This is the protocol your server implements. The harness clients in [`harness/`](harness/) speak it and [`harness/check.py`](harness/check.py) checks your server against it. [`harness/lab09-protocol.h`](harness/lab09-protocol.h) gives every number below a C name. The README explains [why the protocol is fixed](README.md#why-the-protocol-is-fixed) instead of designed by you, and exercise A asks you to design version 2.

"Must" marks what the harness checks or relies on. "May" marks a choice that is yours; write it down in your design note.

## Running the server

```
server --socket PATH --out DIR
```

- **Socket.** Listen on a Unix-domain stream socket at `PATH`. Remove a stale file at `PATH` first. Accept any number of clients one after another; at least 16 at once.
- **Output.** One simulated output of 320 × 240 pixels at 60 Hz. The frame buffer is premultiplied ARGB8888, cleared to `0xff202030` (opaque; R 0x20, G 0x20, B 0x30) before each composition.
- **Frames.** Write the composed image as `DIR/frame-NNNNNN.ppm`, where `NNNNNN` is the frame's `seq` (below), in decimal, zero-padded to at least six digits. The format is binary PPM: the header `P6\n320 240\n255\n`, then 320 × 240 RGB triplets, rows top to bottom, the alpha byte dropped. `DIR` exists before the server starts.
  - You must write a frame for every vblank at which the composed image changed. You may write more (every frame, for example); the checker always reads the newest file whose `seq` is at or below the one it wants.
  - Write each file under a temporary name in `DIR` and `rename(2)` it into place, so that no reader ever sees half a file.
  - The file for frame `n` must be complete before the server sends any `frame_done` with `seq` `n`.
- **Shutdown.** On `SIGTERM` or `SIGINT`, close everything and exit with status 0 within a second.
- **Logs.** Anything on stdout or stderr. `check.py` saves it as `server.log`. One line per connection and per disconnect, with the reason, makes debugging much easier.

## Wire format

A connection carries a stream of messages in each direction. Requests go from client to server, events from server to client. Every message is a whole number of 32-bit words in host byte order:

| Word | Contents |
| --- | --- |
| 0 | the object id the message is for (requests) or from (events) |
| 1 | `size << 16 \| opcode`: `size` is the message length in bytes, header included; `opcode` numbers the request or event within the object's interface |
| 2… | the arguments, one word each |

- `size` is a multiple of 4, at least 8 and at most 64 in version 1. The server must treat any other size as `invalid_message`.
- Argument types: `int` (signed 32-bit), `uint` (unsigned 32-bit), `new_id` (a `uint` the client chooses for a new object), `object` (a `uint` naming an existing object, 0 for none).
- **File descriptors** do not appear in the bytes. A request that takes an `fd` gets it from the `SCM_RIGHTS` ancillary data of the socket ([unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html)). The client sends each fd with the same `sendmsg` as the first byte of the request that takes it. The server appends every fd it receives to a per-connection queue, in arrival order, and each fd-taking request takes the oldest one. An fd-taking request with an empty queue is `no_fd`. Fds left in the queue when the connection ends must be closed.
- Messages may arrive split across reads, and several may arrive in one read. Queue bytes until you have a whole message.

## Objects and ids

- Object 1 is the **display**. It exists from the start of every connection.
- The client chooses the id of every object it creates: from 2 to 65535, not in use on this connection. Otherwise: `invalid_id`.
- Ids belong to the connection. One client cannot name another client's objects.
- `destroy` frees an id at once; the client may reuse it in its next request.
- A request to an id that does not exist, or whose interface has no such opcode, is `invalid_object` or `invalid_method`. An argument that must name a buffer but names something else is `invalid_object`.

## Versions

- The first request on a connection must be `display.hello(version)`. Anything else first, or a second `hello`, is `invalid_message`. `version` 0 is `version`.
- The server answers `display.welcome(v, 320, 240, 60000)` with `v` = the smaller of the client's `version` and the highest version the server implements. `v` applies to the whole connection.
- A later version keeps every message of the earlier ones unchanged. New requests and events get new opcodes. The server never sends an event newer than `v`, and treats a request newer than `v` as `invalid_method`.

## Interfaces

### display (object 1)

| Opcode | Request | Arguments | Size | What it does |
| --- | --- | --- | --- | --- |
| 0 | `hello` | `uint version` | 12 | must come first; answered by `welcome` |
| 1 | `sync` | `uint serial` | 12 | answered by `done(serial)` after every earlier request was handled |
| 2 | `create_surface` | `new_id surface` | 12 | a new surface with no content, at (0, 0), z 0 |
| 3 | `create_buffer` | `new_id buffer`, `fd` (out of band), `int offset`, `int width`, `int height`, `int stride`, `uint format` | 32 | a new buffer: pixel (x, y) is the 32-bit word at byte `offset + y × stride + 4 × x` of the fd's file |

| Opcode | Event | Arguments | Size | When |
| --- | --- | --- | --- | --- |
| 0 | `welcome` | `uint version`, `uint width`, `uint height`, `uint refresh_mhz` | 24 | answer to `hello` |
| 1 | `done` | `uint serial` | 12 | answer to `sync` |
| 2 | `error` | `object id`, `uint code` | 16 | a fatal error; the server closes the connection after sending it |

### surface

| Opcode | Request | Arguments | Size | What it does |
| --- | --- | --- | --- | --- |
| 0 | `destroy` | | 8 | the surface disappears from the next frame; no `frame_done` follows for it |
| 1 | `attach` | `object buffer` (0: remove the content) | 12 | pending: the buffer to show |
| 2 | `set_position` | `int x`, `int y`, `int z` | 20 | pending: the top-left corner on the output, and the stacking order (larger z on top) |
| 3 | `damage` | `int x`, `int y`, `int width`, `int height` | 24 | pending: a rectangle of the buffer that changed, in buffer pixels |
| 4 | `frame` | | 8 | pending: ask for a `frame_done` |
| 5 | `commit` | | 8 | apply all pending state at once (below) |

| Opcode | Event | Arguments | Size | When |
| --- | --- | --- | --- | --- |
| 0 | `frame_done` | `uint seq`, `uint time_us_hi`, `uint time_us_lo` | 20 | after frame `seq` was composed and written, for a commit that included `frame` |

### buffer

| Opcode | Request | Arguments | Size | What it does |
| --- | --- | --- | --- | --- |
| 0 | `destroy` | | 8 | the server forgets the buffer; surfaces keep what they show; a pending `attach` of it is dropped |

| Opcode | Event | Arguments | Size | When |
| --- | --- | --- | --- | --- |
| 0 | `release` | | 8 | the server will not read this buffer again until it is attached again |

## Buffers

`create_buffer` must refuse the buffer with `display.error(buffer id, bad_buffer)`, and close the connection, unless all of these hold:

1. `fcntl(fd, F_GET_SEALS)` succeeds and includes `F_SEAL_SHRINK` ([memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html), [fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html)). Check this *before* the size: without the seal, the size can change after you look at it.
2. The file is at least `offset + stride × height` bytes (`fstat`), computed without overflow.
3. `1 ≤ width ≤ 4096`, `1 ≤ height ≤ 4096`, `offset ≥ 0`, `stride ≥ 4 × width`, and `stride` is a multiple of 4.
4. `format` is 0 (ARGB8888) or 1 (XRGB8888). Any other value is `bad_format` rather than `bad_buffer`.

Formats use the `wl_shm` values ([protocol](https://wayland.freedesktop.org/docs/html/apa.html)): each pixel is a 32-bit word in host byte order with A (or x) in bits 31–24, then R, G and B. ARGB8888 is premultiplied: each color channel is already multiplied by alpha, so no channel exceeds alpha. In XRGB8888 the top byte means nothing: treat the pixel as opaque whatever the byte holds.

The fd is the server's to close. After a successful check it may map the file read-only (`MAP_SHARED`) and close the fd; the mapping stays valid.

## Commit

A commit applies the surface's pending state in this order, then clears it:

1. **Attach.** If the commit carries `attach(0)`, the surface loses its content and is not drawn. If it carries `attach(B)`, the server copies pixels out of `B` into its own copy of the surface:
   - all of `B` if `B` is not the buffer this surface showed last, or `B`'s size differs from the surface's size;
   - otherwise only the damaged pixels (below). Every other pixel keeps what the surface showed before.

   Then the server sends `B.release`. The server never reads a buffer after the commit that attached it, so a client may draw into a released buffer while the server shows the copy.
2. **Position.** `set_position` takes effect.
3. **Frame.** A pending `frame` request becomes a promise of one `frame_done` at the next vblank.

Damage without an `attach` in the same commit is discarded.

**Damage.** Rectangles are in buffer pixels. A rectangle with `width ≤ 0` or `height ≤ 0` is empty. Every other one is clipped to `[0, buffer width) × [0, buffer height)` with exact arithmetic: `x + width` can overflow 32 bits, so compute in 64 bits. The damage of a commit is the union of the clipped rectangles. A server must keep at least 16 rectangles per commit exactly; past that it may treat the commit as damaging the whole buffer.

## Vblank, composition and frame_done

- **Vblank** `n` (n = 1, 2, …) happens at `t₁ + (n − 1) × 16 666 667 ns` on `CLOCK_MONOTONIC`, where `t₁` is chosen when the server starts. Use a periodic `timerfd` with an absolute first expiry ([timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)), so the grid does not drift. If the server wakes so late that the timer reports `k > 1` expirations, `seq` advances by `k`: the missed vblanks are visible to clients as gaps.
- **Frame** `n` contains every commit the server handled before it handled vblank `n`.
- **Composition.** Clear to the background, then draw every surface that has content, in ascending z (surfaces with equal z in an order the server chooses), at its position, clipped to the output. Positions may be anywhere in the `int` range. Each pixel is blended with the stub of the [graphics interface](../../SYLLABUS.md#interface-with-the-graphics-course), premultiplied "over" on the encoded values, per channel (A, R, G, B):

  ```
  out = src + round(dst × (255 − src.alpha) / 255), at most 255
  ```

  `check.py` allows a difference of 1 per channel, so any rounding to nearest works. This blends encoded values; linear-light blending is the graphics course's T4 module's job (exercise F).
- **frame_done.** After frame `n` is composed and its file written (if it changed), the server sends one `frame_done(n, t_hi, t_lo)` for every surface whose committed state asked for one. `t` is the nominal time of vblank `n` in microseconds on `CLOCK_MONOTONIC`, split into its high and low 32 bits. Each `frame` request yields exactly one `frame_done`.

## Errors and disconnects

| Code | Name | When |
| --- | --- | --- |
| 1 | `invalid_object` | unknown object, or an object of the wrong type |
| 2 | `invalid_method` | unknown opcode for the object's interface |
| 3 | `invalid_message` | bad size; `hello` missing or repeated |
| 4 | `invalid_id` | a `new_id` out of range or in use |
| 5 | `no_fd` | an fd-taking request with no fd queued |
| 6 | `bad_buffer` | the buffer checks above failed |
| 7 | `bad_format` | unknown format |
| 8 | `no_resources` | a server limit was reached (objects, buffer memory, fds in flight) |
| 9 | `version` | `hello` with version 0 |

- After `error` the server closes the connection. The error is the last event; deliver it if the socket takes it, but never wait for that.
- The server also closes a connection when the peer hangs up, or when a send fails (`EPIPE`, `ECONNRESET`).
- **Back-pressure.** The server must never block on a client: not in `recv`, not in `send`, not while a message is incomplete. Events the socket does not take now wait in a per-client queue of bounded size. When a client lets that queue fill, the server either closes the connection or stops reading that client's requests until the queue drains. Both are allowed; choose one, and say why in your design note.
- The server must survive any bytes and any fds a client sends, and any client that disappears at any moment.
- Your limits (objects per client, buffer memory per client, fds in flight, clients) are yours to choose; exceeding one is `no_resources`.

## What version 1 leaves out

Compared with Wayland ([protocol](https://wayland.freedesktop.org/docs/html/apa.html)): no registry or globals, no roles or shell (positions are set by the client), no input, no regions, scale, transform or subsurfaces, no callback objects (`frame_done` and `done` are events on existing objects), no `delete_id`, one output, and no presentation feedback beyond `frame_done`'s time. The [Wayland comparison](README.md#the-wayland-comparison) asks what each of these costs or buys.
