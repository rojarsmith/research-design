#!/usr/bin/env python3
"""check.py - Lab 09: run a display server against the harness clients and check it.

usage: python3 check.py --server PROGRAM --bin DIR --work DIR [options]

It starts PROGRAM as "PROGRAM --socket WORK/display.sock --out WORK/frames"
(PROTOCOL.md, "Running the server"), starts the two well-behaved clients
(good-anim as A, good-alpha as B) and keeps them running, then runs one bad
client per phase:

  good      only A and B: the frames and the cadence everything else is compared with
  crash     bad-crash dies in the middle of a frame
  noread    bad-noread floods requests and never reads its socket
  damage    bad-damage sends damage outside its buffer
  unsealed  bad-unsealed sends buffers that can shrink, or are too small

After every phase it checks that the server is still alive, that A kept
receiving frame_done events on the 60 Hz grid (the cadence, with the
tolerance below), that the server holds no more file descriptors or memfd
mappings than before the phase, and it notes which frames to compare. At
the end it compares those frames, pixel by pixel, with images it composes
itself from the clients' known pictures and the blend in PROTOCOL.md.

Cadence: A commits once per frame_done. For each phase the checker counts,
from A's log, the vblanks A missed (gaps in seq) and the frame_done events
that reached A later than --late-ms after the vblank time the event names.
The phase passes if at most --max-skipped vblanks were missed and at most
--max-late events were late. The defaults suit a VM; see the README.

Only the Python 3 standard library is used. Everything goes to WORK: the
server's and clients' logs, the frames, and expected-*.ppm for every frame
that did not match.
"""

import argparse
import os
import re
import signal
import statistics
import subprocess
import sys
import threading
import time

W, H = 320, 240
PERIOD_US = 1_000_000 / 60
BG = (0x20, 0x20, 0x30)
PHASES = ["good", "crash", "noread", "damage", "unsealed"]
INT32_MAX = 2**31 - 1
INT32_MIN = -(2**31)


def now_us():
    return time.monotonic_ns() // 1000      # CLOCK_MONOTONIC, as in the clients


# ---------------------------------------------------------------- the pictures
# Each function must compute what the matching C client draws, in buffer
# coordinates, as premultiplied (a, r, g, b).

def pattern_a(tag, x, y):
    r = (x * 3 + tag * 40) & 0xFF
    g = (y * 2) & 0xFF
    b = 0xC0 if ((x >> 3) ^ (y >> 3)) & 1 else (tag * 50) & 0xFF
    return (0xFF, r, g, b)


def pattern_b1(x, y):
    a = 0x80 | ((x ^ y) & 0x3F)
    return (a, a * x // 128, a // 4, a * y // 96)


def pattern_b2(x, y):
    return (0xFF, 255 - 2 * x, 4 * y, 0x80)     # XRGB8888: always opaque


def pattern_c(x, y):
    return (0xFF, 0xFF, 0, 0xFF) if ((x + y) // 8) & 1 else (0xFF, 0x80, 0, 0x80)


def pattern_d(state, x, y):
    if state == 0:
        return (0xFF, 0, 0x80 + x, (y * 4) & 0xFF)
    return (0xFF, 0xFF, (x * 4) & 0xFF, 0x20)


def clip_damage(x, y, w, h, bw, bh):
    """PROTOCOL.md's damage rule, in exact integers."""
    if w <= 0 or h <= 0:
        return None
    x0, y0, x1, y1 = max(x, 0), max(y, 0), min(x + w, bw), min(y + h, bh)
    if x1 <= x0 or y1 <= y0:
        return None
    return (x0, y0, x1, y1)


# bad-damage's state 1: which pixels were damaged
D_RECTS = [(-1000, -1000, 10, 10), (60, 60, INT32_MAX, INT32_MAX), (INT32_MIN, 10, INT32_MAX, 2),
           (10, -5, 4, 10), (20, 20, -5, -5), (64, 0, 10, 10), (0, 64, 1, 1)]
D_CLIPPED = [r for r in (clip_damage(*d, 64, 64) for d in D_RECTS) if r]


def pattern_d1(x, y):
    for x0, y0, x1, y1 in D_CLIPPED:
        if x0 <= x < x1 and y0 <= y < y1:
            return pattern_d(1, x, y)
    return pattern_d(0, x, y)


class Surf:
    def __init__(self, name, x, y, z, w, h, pix):
        self.name, self.x, self.y, self.z, self.w, self.h, self.pix = name, x, y, z, w, h, pix


class AModel:
    """What client A shows: one tag per pixel, as its commands left it."""

    def __init__(self):
        self.tags = [[0] * 160 for _ in range(120)]

    def full(self, tag):
        self.tags = [[tag] * 160 for _ in range(120)]

    def partial(self, tag):
        for y in range(30, 54):
            for x in range(40, 72):
                self.tags[y][x] = tag

    def surface(self):
        tags = [row[:] for row in self.tags]
        return Surf("A", 16, 16, 0, 160, 120, lambda x, y: pattern_a(tags[y][x], x, y))


B_SURFS = [Surf("B1", 96, 64, 1, 128, 96, pattern_b1), Surf("B2", 232, 16, 2, 64, 48, pattern_b2)]
C_SURF = Surf("C", 200, 150, 3, 96, 72, pattern_c)


def compose(surfs):
    """The blend stub of PROTOCOL.md: premultiplied over, on encoded values."""
    img = [list(BG) for _ in range(W * H)]
    owner = ["background"] * (W * H)
    for s in sorted(surfs, key=lambda s: s.z):
        for sy in range(max(0, -s.y), min(s.h, H - s.y)):
            row = (s.y + sy) * W
            for sx in range(max(0, -s.x), min(s.w, W - s.x)):
                a, r, g, b = s.pix(sx, sy)
                i = row + s.x + sx
                if a == 255:
                    img[i] = [r, g, b]
                else:
                    d, ia = img[i], 255 - a
                    img[i] = [min(255, r + (d[0] * ia + 127) // 255),
                              min(255, g + (d[1] * ia + 127) // 255),
                              min(255, b + (d[2] * ia + 127) // 255)]
                owner[i] = s.name
    return img, owner


def read_ppm(path):
    data = open(path, "rb").read()
    fields, pos = [], 0
    while len(fields) < 4:                      # magic, width, height, maxval
        while data[pos:pos + 1].isspace():
            pos += 1
        if data[pos:pos + 1] == b"#":
            pos = data.index(b"\n", pos) + 1
            continue
        start = pos
        while not data[pos:pos + 1].isspace():
            pos += 1
        fields.append(data[start:pos])
    pos += 1                                    # one whitespace byte before the pixels
    if fields[0] != b"P6" or int(fields[3]) != 255:
        raise ValueError("not a P6 PPM with maxval 255")
    return int(fields[1]), int(fields[2]), data[pos:]


def write_ppm(path, img):
    with open(path, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (W, H))
        f.write(bytes(c for px in img for c in px))


# ---------------------------------------------------------------- processes

def parse_line(raw):
    toks = raw.split()
    rec = {"_raw": raw, "_tag": toks[0] if toks else "", "_what": toks[1] if len(toks) > 1 else ""}
    for t in toks[2:]:
        if "=" in t:
            k, v = t.split("=", 1)
            rec[k] = v
    return rec


class Client:
    def __init__(self, name, argv, work, stdin=False):
        self.name = name
        self.log = open(os.path.join(work, name + ".log"), "w")
        self.p = subprocess.Popen(argv, stdin=subprocess.PIPE if stdin else subprocess.DEVNULL,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.lines = []
        self.cv = threading.Condition()
        self.eof = False
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        for raw in self.p.stdout:
            raw = raw.rstrip("\n")
            self.log.write(raw + "\n")
            self.log.flush()
            with self.cv:
                self.lines.append(parse_line(raw))
                self.cv.notify_all()
        with self.cv:
            self.eof = True
            self.cv.notify_all()

    def send(self, text):
        try:
            self.p.stdin.write(text + "\n")
            self.p.stdin.flush()
            return True
        except (BrokenPipeError, OSError, ValueError):
            return False

    def mark(self):
        with self.cv:
            return len(self.lines)

    def wait_line(self, pred, timeout, start=0):
        deadline = time.monotonic() + timeout
        with self.cv:
            i = start
            while True:
                while i < len(self.lines):
                    if pred(self.lines[i]):
                        return self.lines[i]
                    i += 1
                left = deadline - time.monotonic()
                if left <= 0 or self.eof:
                    return None
                self.cv.wait(left)

    def select(self, pred, start=0):
        with self.cv:
            return [r for r in self.lines[start:] if pred(r)]

    def wait_exit(self, timeout):
        try:
            return self.p.wait(timeout)
        except subprocess.TimeoutExpired:
            return None

    def stop(self):
        if self.p.poll() is None:
            self.p.kill()
            self.p.wait()


def proc_resources(pid):
    try:
        fds = len(os.listdir("/proc/%d/fd" % pid))
        with open("/proc/%d/maps" % pid) as f:
            maps = sum(1 for line in f if "/memfd:" in line)
        status = {}
        with open("/proc/%d/status" % pid) as f:
            for line in f:
                k, _, v = line.partition(":")
                status[k] = v.strip()
        kib = lambda k: int(status.get(k, "0 kB").split()[0])
        return {"fds": fds, "maps": maps, "rss": kib("VmRSS"), "hwm": kib("VmHWM")}
    except OSError:
        return None


def describe_exit(code):
    if code is None:
        return "still running"
    if code < 0:
        try:
            return "killed by %s" % signal.Signals(-code).name
        except ValueError:
            return "killed by signal %d" % -code
    return "exit status %d" % code


# ---------------------------------------------------------------- the run

class Run:
    def __init__(self, args):
        self.a = args
        self.work = os.path.abspath(args.work)
        self.frames = os.path.join(self.work, "frames")
        self.sock = os.path.join(self.work, "display.sock")
        os.makedirs(self.frames, exist_ok=True)
        for n in os.listdir(self.frames):       # frames of an earlier run in the same WORK
            if re.fullmatch(r"\.?frame-\d+\.(ppm|tmp)", n):
                os.remove(os.path.join(self.frames, n))
        self.results = []           # (phase, ok or None for a note, text)
        self.checkpoints = []       # (phase, name, seq, surfaces)
        self.cadence_rows = []
        self.amodel = AModel()
        self.next_tag = 1
        self.server = None
        self.A = self.B = None

    # -- reporting
    def ok(self, phase, cond, text):
        self.results.append((phase, bool(cond), text))
        return bool(cond)

    def note(self, phase, text):
        self.results.append((phase, None, text))

    def progress(self, text):
        print("--", text, flush=True)

    def bin(self, name):
        return os.path.join(self.a.bin, name)

    # -- the server
    def server_alive(self, phase):
        code = self.server.poll()
        if code is None:
            return True
        self.ok(phase, False, "server died: %s" % describe_exit(code))
        return False

    def start(self):
        self.progress("starting %s" % self.a.server)
        self.server_log = open(os.path.join(self.work, "server.log"), "w")
        self.server = subprocess.Popen([self.a.server, "--socket", self.sock, "--out", self.frames],
                                       stdout=self.server_log, stderr=subprocess.STDOUT)
        deadline = time.monotonic() + 5
        while not os.path.exists(self.sock):
            if self.server.poll() is not None or time.monotonic() > deadline:
                self.ok("setup", False, "the server did not create %s within 5 s (%s)" %
                        (self.sock, describe_exit(self.server.poll())))
                return False
            time.sleep(0.02)
        self.A = Client("A", [self.bin("good-anim"), self.sock], self.work, stdin=True)
        self.B = Client("B", [self.bin("good-alpha"), self.sock], self.work, stdin=True)
        wa = self.A.wait_line(lambda r: r["_what"] == "welcome", 5)
        wb = self.B.wait_line(lambda r: r["_what"] == "welcome", 5)
        sa = self.A.wait_line(lambda r: r["_what"] == "shown", 3)
        sb = self.B.wait_line(lambda r: r["_what"] == "shown", 3)
        good = self.ok("setup", wa and wb, "A and B got welcome")
        if wa:
            good &= self.ok("setup", (wa.get("version"), wa.get("width"), wa.get("height"), wa.get("refresh_mhz"))
                            == ("1", "320", "240", "60000"),
                            "A asked for version 1: welcome version=%s width=%s height=%s refresh_mhz=%s" %
                            (wa.get("version"), wa.get("width"), wa.get("height"), wa.get("refresh_mhz")))
        if wb:
            v = int(wb.get("version", "0"))
            good &= self.ok("setup", 1 <= v <= 7, "B asked for version 7: welcome version=%d (must be 1 to 7)" % v)
        good &= self.ok("setup", sa and sb, "A and B received their first frame_done")
        if not good or not self.server_alive("setup"):
            return False
        time.sleep(0.3)
        self.base = proc_resources(self.server.pid)
        self.note("setup", "baseline with A and B connected: %d fds, %d memfd mappings, VmRSS %d KiB" %
                  (self.base["fds"], self.base["maps"], self.base["rss"]))
        return True

    # -- A as the barrier
    def change_a(self, phase, kind):
        """Ask A for a new picture; return the seq at which the frames have
        settled (three vblanks after the change was shown), or None."""
        tag, self.next_tag = self.next_tag, self.next_tag + 1
        start = self.A.mark()
        self.A.send("tag %d %s" % (tag, kind))
        rec = self.A.wait_line(lambda r: r["_what"] == "shown" and r.get("tag") == str(tag), 3, start)
        if not rec:
            self.ok(phase, False, "A's picture %d (%s) was not shown within 3 s" % (tag, kind))
            return None
        seq = int(rec["seq"])
        if kind == "full":
            self.amodel.full(tag)
        else:
            self.amodel.partial(tag)
        if not self.A.wait_line(lambda r: r["_what"] == "done" and int(r["seq"]) >= seq + 3, 3, start):
            self.ok(phase, False, "A got no frame_done after seq %d" % seq)
            return None
        return seq + 3

    def checkpoint(self, phase, name, seq, surfs):
        self.checkpoints.append((phase, name, seq, surfs))

    def resources(self, phase):
        cur = None
        for _ in range(5):
            cur = proc_resources(self.server.pid)
            if cur and cur["fds"] == self.base["fds"] and cur["maps"] == self.base["maps"]:
                break
            time.sleep(0.05)
        if not cur:
            return self.ok(phase, False, "cannot read /proc/%d" % self.server.pid)
        self.ok(phase, cur["fds"] == self.base["fds"],
                "server fds back to the baseline: %d (baseline %d)" % (cur["fds"], self.base["fds"]))
        self.ok(phase, cur["maps"] == self.base["maps"],
                "memfd mappings back to the baseline: %d (baseline %d)" % (cur["maps"], self.base["maps"]))
        return cur

    def cadence(self, phase, t0, t1):
        ds = self.A.select(lambda r: r["_what"] == "done" and t0 <= int(r["recv_us"]) <= t1)
        vblanks = (t1 - t0) / PERIOD_US
        if len(ds) < 2:
            self.ok(phase, False, "cadence: A received %d frame_done events in %.0f ms (about %.0f vblanks)" %
                    (len(ds), (t1 - t0) / 1000, vblanks))
            self.cadence_rows.append((phase, vblanks, len(ds), None, None, None, None))
            return
        seqs = [int(r["seq"]) for r in ds]
        recv = [int(r["recv_us"]) for r in ds]
        vbl = [int(r["vblank_us"]) for r in ds]
        skipped = sum(max(0, b - a - 1) for a, b in zip(seqs, seqs[1:]))
        lat = sorted((r - v) / 1000 for r, v in zip(recv, vbl))
        late = sum(1 for x in lat if x > self.a.late_ms)
        gap = max((b - a) / 1000 for a, b in zip(recv, recv[1:]))
        off_grid = sum(1 for i in range(1, len(ds))
                       if abs((vbl[i] - vbl[0]) - (seqs[i] - seqs[0]) * PERIOD_US) > 1000)
        p99 = lat[min(len(lat) - 1, int(0.99 * len(lat)))]
        self.cadence_rows.append((phase, vblanks, len(ds), skipped, late,
                                  (statistics.median(lat), p99, lat[-1]), gap))
        self.ok(phase, skipped <= self.a.max_skipped and late <= self.a.max_late and min(lat) > -1.0,
                "cadence: %d frame_done in %.0f ms, %d vblank(s) skipped (max %d), %d later than %.0f ms (max %d), "
                "lateness median %.2f / p99 %.2f / max %.2f ms, largest gap %.1f ms" %
                (len(ds), (t1 - t0) / 1000, skipped, self.a.max_skipped, late, self.a.late_ms, self.a.max_late,
                 statistics.median(lat), p99, lat[-1], gap))
        self.ok(phase, off_grid == 0,
                "frame_done times on the 60 Hz grid: %d of %d off by more than 1 ms" % (off_grid, len(ds) - 1))

    # -- phases
    def phase_good(self):
        p = "good"
        t0 = now_us()
        time.sleep(1.0)
        s = self.change_a(p, "full")
        if s:
            self.checkpoint(p, "A redrawn in full, with B over it", s, [self.amodel.surface()] + B_SURFS)
        s = self.change_a(p, "partial")
        if s:
            self.checkpoint(p, "A partly redrawn (damage 40,30 32x24)", s, [self.amodel.surface()] + B_SURFS)
        self.cadence(p, t0, now_us())

    def phase_crash(self):
        p = "crash"
        t0 = now_us()
        C = Client("C", [self.bin("bad-crash"), self.sock], self.work)
        vis = C.wait_line(lambda r: r["_what"] == "visible", 3)
        code = C.wait_exit(3)
        C.stop()
        self.ok(p, vis, "C was shown (frame_done seq %s)" % (vis["seq"] if vis else "none"))
        self.note(p, "C ended: %s" % describe_exit(code))
        if not self.server_alive(p):
            return False
        if vis:
            self.checkpoint(p, "C on screen before it crashed", int(vis["seq"]),
                            [self.amodel.surface()] + B_SURFS + [C_SURF])
        s = self.change_a(p, "full")
        if s:
            self.checkpoint(p, "C gone after the crash", s, [self.amodel.surface()] + B_SURFS)
        self.resources(p)
        self.cadence(p, t0, now_us())
        return self.server_alive(p)

    def phase_noread(self):
        p = "noread"
        t0 = now_us()
        # First the client that shut its read side: every reply to it fails with EPIPE.
        R = Client("R", [self.bin("bad-noread"), self.sock, "shutrd"], self.work)
        time.sleep(0.5)
        if R.p.poll() is None:
            R.p.send_signal(signal.SIGTERM)
        R.wait_exit(2)
        R.stop()
        end = R.wait_line(lambda r: r["_what"] == "end", 1)
        if not self.server_alive(p):
            self.note(p, "the server died while R (read side shut) was connected: SIGPIPE?")
            return False
        self.ok(p, end and end.get("how", "").startswith("disconnected"),
                "R (read side shut, so replies fail with EPIPE) was disconnected: %s" %
                (end["_raw"].split(" ", 2)[2] if end else "no end line"))
        # Then the flood.
        N = Client("N", [self.bin("bad-noread"), self.sock, "flood"], self.work)
        time.sleep(self.a.noread_seconds)
        res = proc_resources(self.server.pid) if self.server.poll() is None else None
        if N.p.poll() is None:
            N.p.send_signal(signal.SIGTERM)
        code = N.wait_exit(2)
        N.stop()
        end = N.wait_line(lambda r: r["_what"] == "end", 1)
        if not self.server_alive(p):
            return False
        if end:
            how, blocked = end.get("how", "?"), int(end.get("last_send_blocked_ms", "0"))
            sent = int(end.get("sent_bytes", "0"))
            if how.startswith("disconnected"):
                self.note(p, "the server disconnected N after N wrote %d bytes (%s)" % (sent, how))
            elif blocked >= 100:
                self.note(p, "the server stopped reading N: N wrote %d bytes, then waited %d ms in sendmsg" %
                          (sent, blocked))
            else:
                self.note(p, "N was still writing when stopped (%d bytes, last write blocked %d ms): "
                          "the server keeps reading it; where do its events go?" % (sent, blocked))
        else:
            self.note(p, "N printed no end line (%s)" % describe_exit(code))
        if res:
            growth = (res["hwm"] - self.base["rss"]) / 1024
            self.ok(p, growth <= self.a.max_growth_mib,
                    "server memory: VmHWM %d KiB, %.1f MiB above the baseline VmRSS (max %d MiB)" %
                    (res["hwm"], growth, self.a.max_growth_mib))
        s = self.change_a(p, "full")
        if s:
            self.checkpoint(p, "N gone", s, [self.amodel.surface()] + B_SURFS)
        self.resources(p)
        self.cadence(p, t0, now_us())
        return self.server_alive(p)

    def phase_damage(self):
        p = "damage"
        t0 = now_us()
        D = Client("D", [self.bin("bad-damage"), self.sock], self.work)
        states = [D.wait_line(lambda r, k=k: r["_what"] == "shown" and r.get("state") == str(k), 3)
                  for k in range(3)]
        D.wait_exit(3)
        D.stop()
        errors = D.select(lambda r: r["_what"] in ("unexpected", "no-frame-done"))
        self.ok(p, not errors, "D got no error and every frame_done" +
                ("" if not errors else ": " + errors[0]["_raw"]))
        if not self.server_alive(p):
            return False
        a = self.amodel.surface()
        pics = [lambda x, y: pattern_d(0, x, y), pattern_d1, lambda x, y: pattern_d(1, x, y)]
        names = ["D state 0 (full damage)", "D state 1 (only the clipped damage copied)", "D state 2 (full damage)"]
        for k in range(3):
            if states[k]:
                self.checkpoint(p, names[k], int(states[k]["seq"]),
                                [a] + B_SURFS + [Surf("D", 16, 160, 3, 64, 64, pics[k])])
            else:
                self.ok(p, False, "D did not reach state %d" % k)
        s = self.change_a(p, "full")
        if s:
            self.checkpoint(p, "D gone", s, [self.amodel.surface()] + B_SURFS)
        self.resources(p)
        self.cadence(p, t0, now_us())
        return self.server_alive(p)

    def phase_unsealed(self):
        p = "unsealed"
        t0 = now_us()
        U = Client("U", [self.bin("bad-unsealed"), self.sock], self.work)
        U.wait_exit(8)
        U.stop()
        if not self.server_alive(p):
            for r in U.select(lambda r: r["_what"] in ("verdict", "shrunk")):
                self.note(p, "U said: " + r["_raw"])
            return False
        for variant in ("unsealed", "too-small"):
            r = U.wait_line(lambda r: r["_what"] == "verdict" and r.get("variant") == variant, 0.1)
            got = r["_raw"].split(" ", 2)[2] if r else "no verdict line"
            self.ok(p, r and r.get("result") == "error" and r.get("code") == "6",
                    "%s buffer refused with error bad_buffer (6): %s" % (variant, got))
            after = U.wait_line(lambda r: r["_what"] == "after-error" and r.get("variant") == variant, 0.1)
            if after:
                self.ok(p, after.get("state") == "closed",
                        "%s: connection closed after the error (%s)" % (variant, after.get("state")))
        s = self.change_a(p, "full")
        if s:
            self.checkpoint(p, "U never shown", s, [self.amodel.surface()] + B_SURFS)
        self.resources(p)
        self.cadence(p, t0, now_us())
        return self.server_alive(p)

    def finish(self):
        p = "end"
        alive = self.server is not None and self.server.poll() is None
        for c in (self.A, self.B):
            if c:
                c.send("quit")
        if self.A and not alive:
            self.note(p, "A's summary not checked: the server is not running")
        elif self.A:
            sa = self.A.wait_line(lambda r: r["_what"] == "summary", 3)
            if sa:
                at, rel = int(sa["attaches"]), int(sa["releases"])
                fr, dn = int(sa["frame_requests"]), int(sa["dones"])
                self.ok(p, rel == at, "A: %d attaches, %d releases" % (at, rel))
                self.ok(p, fr - 1 <= dn <= fr, "A: %d frame requests, %d frame_done (one may be outstanding)" % (fr, dn))
            else:
                self.ok(p, False, "A printed no summary")
        for c in (self.A, self.B):
            if c:
                c.wait_exit(3)
                c.stop()
        if self.server and self.server.poll() is None:
            self.server.send_signal(signal.SIGTERM)
            try:
                code = self.server.wait(3)
            except subprocess.TimeoutExpired:
                self.server.kill()
                code = self.server.wait()
                self.ok(p, False, "server did not exit within 3 s of SIGTERM; killed")
            else:
                self.ok(p, code == 0, "server exited after SIGTERM: %s" % describe_exit(code))

    # -- frames, compared after the run so that the comparison costs no cadence
    def compare_frames(self):
        files = {}
        for n in os.listdir(self.frames):
            m = re.fullmatch(r"frame-(\d+)\.ppm", n)
            if m:
                files[int(m.group(1))] = n
        seqs = sorted(files)
        self.note("frames", "%d frame files written" % len(seqs))
        for i, (phase, name, seq, surfs) in enumerate(self.checkpoints):
            older = [s for s in seqs if s <= seq]
            if not older:
                self.ok(phase, False, "frame for %s: no frame file at or before seq %d" % (name, seq))
                continue
            path = os.path.join(self.frames, files[older[-1]])
            try:
                w, h, pix = read_ppm(path)
            except (OSError, ValueError, IndexError) as e:
                self.ok(phase, False, "frame for %s: %s: %s" % (name, os.path.basename(path), e))
                continue
            if (w, h) != (W, H) or len(pix) < W * H * 3:
                self.ok(phase, False, "frame for %s: %s is %dx%d" % (name, os.path.basename(path), w, h))
                continue
            exp, owner = compose(surfs)
            bad = []
            for j in range(W * H):
                e, k = exp[j], 3 * j
                if abs(pix[k] - e[0]) > 1 or abs(pix[k + 1] - e[1]) > 1 or abs(pix[k + 2] - e[2]) > 1:
                    bad.append(j)
            text = "frame for %s: %s" % (name, os.path.basename(path))
            if not bad:
                self.ok(phase, True, text + " matches")
                continue
            where = {}
            for j in bad:
                where[owner[j]] = where.get(owner[j], 0) + 1
            j = bad[0]
            first = "first at (%d, %d): got %s, expected %s" % (
                j % W, j // W, tuple(pix[3 * j:3 * j + 3]), tuple(exp[j]))
            out = os.path.join(self.work, "expected-%02d.ppm" % i)
            write_ppm(out, exp)
            self.ok(phase, False, "%s: %d pixels differ by more than 1 (%s); %s; expected image in %s" %
                    (text, len(bad), ", ".join("%s %d" % kv for kv in sorted(where.items())), first, out))

    def report(self):
        print()
        print("== results")
        order = ["setup"] + PHASES + ["end", "frames"]
        passed = failed = 0
        for phase in order:
            rows = [r for r in self.results if r[0] == phase]
            if not rows:
                continue
            print("-- %s" % phase)
            for _, ok, text in rows:
                mark = "  note" if ok is None else "  ok  " if ok else "  FAIL"
                print("%s %s" % (mark, text))
                passed += ok is True
                failed += ok is False
        print()
        print("== cadence (client A)")
        print("%-9s %8s %7s %8s %6s %24s %9s" % ("phase", "vblanks", "A done", "skipped", "late",
                                                  "lateness med/p99/max ms", "max gap"))
        for phase, vbl, n, sk, late, lat, gap in self.cadence_rows:
            if lat is None:
                print("%-9s %8.0f %7d %8s %6s %24s %9s" % (phase, vbl, n, "-", "-", "-", "-"))
            else:
                print("%-9s %8.0f %7d %8d %6d %24s %8.1f" % (phase, vbl, n, sk, late,
                                                           "%.2f / %.2f / %.2f" % lat, gap))
        print()
        print("== %s: %d checks passed, %d failed" % ("PASS" if failed == 0 else "FAIL", passed, failed))
        return failed == 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--server", required=True, help="the server program")
    ap.add_argument("--bin", required=True, help="the directory with the built harness clients")
    ap.add_argument("--work", required=True, help="a directory for the socket, logs and frames")
    ap.add_argument("--only", action="append", choices=PHASES[1:],
                    help="run only this bad-client phase (repeatable); good always runs")
    ap.add_argument("--late-ms", type=float, default=8.0,
                    help="a frame_done later than this after its vblank counts as late (default 8)")
    ap.add_argument("--max-late", type=int, default=3, help="late frame_done events allowed per phase (default 3)")
    ap.add_argument("--max-skipped", type=int, default=2, help="vblanks A may miss per phase (default 2)")
    ap.add_argument("--noread-seconds", type=float, default=2.0, help="how long bad-noread runs (default 2)")
    ap.add_argument("--max-growth-mib", type=int, default=16,
                    help="allowed growth of the server's VmHWM during noread (default 16)")
    args = ap.parse_args()

    run = Run(args)
    phases = [p for p in PHASES[1:] if not args.only or p in args.only]
    try:
        if run.start():
            run.progress("phase good")
            run.phase_good()
            for p in phases:
                run.progress("phase %s" % p)
                if not getattr(run, "phase_" + p)():
                    rest = phases[phases.index(p) + 1:]
                    if rest:
                        run.note(p, "not run because the server is dead: %s" % ", ".join(rest))
                    break
    finally:
        run.finish()
        for c in (run.A, run.B):
            if c:
                c.stop()
        if run.server and run.server.poll() is None:
            run.server.kill()
    run.compare_frames()
    if run.server and run.server.returncode not in (0, None):
        with open(os.path.join(run.work, "server.log")) as f:
            tail = f.readlines()[-15:]
        print("== last lines of server.log")
        sys.stdout.writelines(tail)
    sys.exit(0 if run.report() else 1)


if __name__ == "__main__":
    main()
