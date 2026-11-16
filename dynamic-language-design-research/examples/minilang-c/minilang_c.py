#!/usr/bin/env python3
"""minilang_c: drive the C VM (mlvm.c) from Python.

The front end stays in Python: minilang's parser and minilang_vm's compiler
build a Code object, minilang_vm.dump_program() writes it as a bytecode file,
and the C VM loads and runs that file. This module provides:

    CVM      the constructor and run() contract of minilang.Interpreter, so the
             stage 2 and stage 4 test suites run on the C VM without changes
    Server   a client for `mlvm --serve`: one process, many independent VMs
    bench()  tree-walker, Python VM and C VM on programs/bench_*.ml

Build the VM first (see README.md):
    gcc -std=c11 -Wall -Wextra -Werror -O2 -o mlvm mlvm.c -lm

Usage:
    python3 minilang_c.py program.ml                 # compile, then run on the C VM
    python3 minilang_c.py --gc-stress --stats program.ml
    python3 minilang_c.py --bench [runs]             # about 2 minutes with 5 runs
Add --opt to compile with minilang_vm's optimizations.
"""
import atexit
import builtins
import os
import platform
import re
import shutil
import statistics
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "minilang-vm"))

import minilang_vm  # noqa: E402  stage 4: compiler and bytecode file writer
import minilang  # noqa: E402  stage 2: parser and the reference tree-walker

TreeWalker = minilang.Interpreter   # captured before tests patch minilang.Interpreter
EXE = ".exe" if os.name == "nt" else ""
BUILD = "gcc -std=c11 -Wall -Wextra -Werror -O2 -o mlvm mlvm.c -lm"


def default_mlvm():
    """The C VM binary: $MLVM, else mlvm next to this file."""
    path = os.environ.get("MLVM") or os.path.join(HERE, "mlvm" + EXE)
    if not os.path.exists(path):
        raise FileNotFoundError("C VM not found at %s; build it in %s with:\n    %s" % (path, HERE, BUILD))
    return path


_TMP = None


def temp_path(suffix=".mlc"):
    """A fresh file name in a per-process directory that is removed at exit."""
    global _TMP
    if _TMP is None:
        _TMP = tempfile.mkdtemp(prefix="minilang-c-")
        atexit.register(shutil.rmtree, _TMP, True)
    temp_path.count = getattr(temp_path, "count", 0) + 1
    return os.path.join(_TMP, "p%d%s" % (temp_path.count, suffix))


def compile_to_file(src, optimize=False, path=None):
    """Parse and compile with the Python front end; write the bytecode file. Raises SyntaxErr."""
    program = minilang.parse(src)
    old = sys.getrecursionlimit()
    sys.setrecursionlimit(max(old, TreeWalker.MAX_DEPTH * 12 + 500))   # as VM.run does
    try:
        script = minilang_vm.compile_program(program, optimize)
    finally:
        sys.setrecursionlimit(old)
    path = path or temp_path()
    with open(path, "wb") as f:
        f.write(minilang_vm.dump_program(script, optimized=bool(optimize)))
    return path


# ------------------------------------------------------- serve protocol ----

_ESCAPES = {"\\": "\\", "n": "\n", "r": "\r"}


def unescape(text):
    return re.sub(r"\\(.)", lambda m: _ESCAPES[m.group(1)], text)


class Opaque:
    """A value the C VM can only describe as text (a list or a function)."""

    def __init__(self, text):
        self.text = text

    def __str__(self):
        return self.text

    __repr__ = __str__


def decode_value(text):
    kind, _, rest = text.partition(" ")
    if kind in ("nil", "true", "false"):
        return {"nil": None, "true": True, "false": False}[kind]
    if kind == "int":
        return int(rest)
    if kind == "float":
        return float(rest)
    if kind == "str":
        return unescape(rest)
    return Opaque(unescape(rest))


class ServerError(RuntimeError):
    pass


class Server:
    """One `mlvm --serve` process. Each session is a separate VM with its own
    heap, globals and options, so one process serves a whole test run."""

    def __init__(self, mlvm=None):
        self.mlvm = mlvm or default_mlvm()
        self.proc = subprocess.Popen([self.mlvm, "--serve"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    def alive(self):
        return self.proc.poll() is None

    def _send(self, line):
        try:
            self.proc.stdin.write(line.encode("utf-8") + b"\n")
            self.proc.stdin.flush()
        except OSError as err:
            raise ServerError("mlvm exited (status %s)" % self.proc.poll()) from err

    def _reply(self):
        line = self.proc.stdout.readline()
        if not line:
            raise ServerError("mlvm exited (status %s)" % self.proc.wait())
        return line.rstrip(b"\n").decode("utf-8")

    def new(self, stress=False, min_heap=None):
        self._send("new" + (" stress" if stress else "") + (" min=%d" % min_heap if min_heap else ""))
        reply = self._reply()
        if not reply.startswith("k "):
            raise ServerError(reply)
        return int(reply[2:])

    def run(self, session, path):
        """Run a bytecode file in a session: (printed lines, ending line)."""
        self._send("run %d %s" % (session, path))
        lines = []
        while True:
            reply = self._reply()
            if reply.startswith("o "):
                lines.append(unescape(reply[2:]))
            else:
                return lines, reply

    def stats(self, session):
        self._send("stats %d" % session)
        reply = self._reply()
        return {k: float(v) if "." in v else int(v) for k, v in (p.split("=") for p in reply[2:].split())}

    def free(self, session):
        """Destroy a session; returns the bytes its heap still counted (must be 0)."""
        self._send("free %d" % session)
        return int(self._reply().split("leaked=")[1])

    def close(self):
        if self.alive():
            try:
                self._send("quit")
                self.proc.stdin.close()
            except ServerError:
                pass
            self.proc.wait()
        self.proc.stdout.close()


_SERVERS = {}
LEAKS = []   # (session, bytes) for every freed session that did not return to 0


def shared_server(mlvm=None):
    mlvm = mlvm or default_mlvm()
    server = _SERVERS.get(mlvm)
    if server is None or not server.alive():
        server = _SERVERS[mlvm] = Server(mlvm)
    return server


@atexit.register
def _close_servers():
    for server in _SERVERS.values():
        server.close()


class CVM:
    """Runs minilang programs on the C VM. Same constructor and run() contract
    as minilang.Interpreter: output is a callback per print() call, run()
    returns the trailing expression's value or raises what the tree-walker
    would raise (RuntimeErr with the same text, ReturnSignal, BreakSignal, or
    the same Python exception type for errors minilang cannot catch). Globals
    persist across run() calls on one instance, as in the REPL."""

    MAX_DEPTH = 400

    def __init__(self, output=None, optimize=False, gc_stress=False, min_heap=None, mlvm=None):
        self.output = output if output is not None else (lambda s: print(s))
        self.optimize = minilang_vm.optimizations(optimize)
        self.server = shared_server(mlvm)
        self.session = self.server.new(gc_stress, min_heap)

    def run(self, src):
        path = compile_to_file(src, self.optimize)
        try:
            lines, ending = self.server.run(self.session, path)
        finally:
            os.remove(path)
        for line in lines:
            self.output(line)
        kind, _, rest = ending.partition(" ")
        if kind == "v":
            return decode_value(rest)
        if kind == "e":
            line, _, message = rest.partition(" ")
            raise minilang.RuntimeErr(unescape(message), int(line))
        if kind == "r":
            raise minilang.ReturnSignal(decode_value(rest))
        if kind == "b":
            raise minilang.BreakSignal()
        if kind == "h":
            name, _, message = rest.partition(" ")
            raise getattr(builtins, name)(unescape(message))
        raise ServerError("load error: %s" % unescape(rest))

    def stats(self):
        return self.server.stats(self.session)

    def close(self):
        if self.session is not None and self.server.alive():
            leaked = self.server.free(self.session)
            if leaked:
                LEAKS.append((self.session, leaked))
        self.session = None

    def __del__(self):
        try:
            self.close()
        except Exception:   # noqa: BLE001  interpreter shutdown or a dead server
            pass


# ------------------------------------------------------------ benchmarks ----

BENCH_DIR = os.path.join(HERE, "..", "minilang-vm", "programs")
BENCHMARKS = ("bench_fib.ml", "bench_loop.ml", "bench_concat.ml")


def _gcc_version():
    try:
        out = subprocess.run(["gcc", "--version"], capture_output=True, text=True, check=True).stdout
        return out.splitlines()[0]
    except (OSError, subprocess.CalledProcessError):
        return "gcc not found"


def _c_run(mlvm, path, extra=()):
    """One process: (wall seconds, stdout, stats dict)."""
    start = time.perf_counter()
    proc = subprocess.run([mlvm, "--stats", *extra, path], capture_output=True)
    wall = time.perf_counter() - start
    err = proc.stderr.decode("utf-8")
    line = next(x for x in err.splitlines() if x.startswith("mlvm stats: "))
    stats = {k: float(v) for k, v in (p.split("=") for p in line[len("mlvm stats: "):].split())}
    return wall, proc.stdout.decode("utf-8").replace("\r\n", "\n"), stats


def bench(runs=5, mlvm=None, mlvm_o0=None):
    """Every engine `runs` times, interleaved; prints Markdown tables of
    medians. Python engines are timed in-process including parse and compile,
    as in minilang_vm --bench. The C VM is timed twice: the whole process
    (start, load, run, exit) and the run alone, as reported by --stats."""
    mlvm = mlvm or default_mlvm()
    if mlvm_o0 is None:
        candidate = os.path.join(HERE, "mlvm-O0" + EXE)
        mlvm_o0 = candidate if os.path.exists(candidate) else None
    print("Python %s, %s, %s" % (platform.python_version(), platform.platform(), platform.processor() or platform.machine()))
    print(_gcc_version())
    print("%d runs per engine; median seconds (min-max)" % runs)
    rows, gc_rows = [], []
    for filename in BENCHMARKS:
        with open(os.path.join(BENCH_DIR, filename), encoding="utf-8") as f:
            src = f.read()
        plain = compile_to_file(src, False)
        opt = compile_to_file(src, True)
        c_engines = [("C -O2", mlvm, plain), ("C -O2 --opt", mlvm, opt)]
        if mlvm_o0:
            c_engines.append(("C -O0", mlvm_o0, plain))
        c_engines += [("C -O2 +" + o, mlvm, compile_to_file(src, [o])) for o in minilang_vm.OPTIMIZATIONS]
        times = {name: [] for name in ["tree-walker", "Python VM"] + [n for n, _, _ in c_engines]}
        walls = {name: [] for name, _, _ in c_engines}
        outputs, gc = set(), None
        for _ in range(runs):
            for name, make in (("tree-walker", TreeWalker), ("Python VM", minilang_vm.VM)):
                out = []
                start = time.perf_counter()
                make(output=out.append).run(src)
                times[name].append(time.perf_counter() - start)
                outputs.add("\n".join(out) + "\n")
            for name, exe, path in c_engines:
                wall, out, stats = _c_run(exe, path)
                walls[name].append(wall)
                times[name].append(stats["run_us"] / 1e6)
                outputs.add(out)
                if name == "C -O2":
                    gc = stats
        if len(outputs) != 1:
            raise AssertionError("engines disagree on %s: %r" % (filename, outputs))
        _, out, stress = _c_run(mlvm, plain, ["--gc-stress"])
        if out not in outputs:
            raise AssertionError("--gc-stress output differs on %s" % filename)
        rows.append((filename[:-3], outputs.pop().strip(), times, walls))
        gc_rows.append((filename[:-3], gc, stress))

    def cell(t):
        return "%.4f (%.4f-%.4f)" % (statistics.median(t), min(t), max(t))

    names = [n for n in rows[0][2] if "+" not in n]
    print()
    print("| benchmark | output | " + " | ".join(names) + " |")
    print("| --- | --- |" + " --- |" * len(names))
    for name, output, times, _ in rows:
        print("| %s | %s | %s |" % (name, output, " | ".join(cell(times[n]) for n in names)))
    print()
    print("C VM whole process (start, load, run, exit), median seconds:")
    print()
    c_names = [n for n in rows[0][3] if "+" not in n]
    print("| benchmark | " + " | ".join(c_names) + " |")
    print("| --- |" + " --- |" * len(c_names))
    for name, _, _, walls in rows:
        print("| %s | %s |" % (name, " | ".join(cell(walls[n]) for n in c_names)))
    print()
    print("Speedup of the C VM run (-O2, plain bytecode) over:")
    print()
    print("| benchmark | tree-walker | Python VM | -O2 --opt vs -O2 | -O2 vs -O0 |")
    print("| --- | --- | --- | --- | --- |")
    for name, _, times, _ in rows:
        c = statistics.median(times["C -O2"])
        o0 = statistics.median(times["C -O0"]) / c if "C -O0" in times else float("nan")
        print("| %s | %.0fx | %.0fx | %.2fx | %.2fx |" % (
            name, statistics.median(times["tree-walker"]) / c, statistics.median(times["Python VM"]) / c,
            c / statistics.median(times["C -O2 --opt"]), o0))
    print()
    print("Each optimization alone on the C VM (-O2), median seconds and speedup over plain bytecode:")
    print()
    alone = ["C -O2 +" + o for o in minilang_vm.OPTIMIZATIONS]
    print("| benchmark | plain | " + " | ".join(a[len("C -O2 "):] for a in alone) + " | all (--opt) |")
    print("| --- | --- |" + " --- |" * (len(alone) + 1))
    for name, _, times, _ in rows:
        c = statistics.median(times["C -O2"])
        cells = ["%.4f (%.2fx)" % (statistics.median(times[e]), c / statistics.median(times[e]))
                 for e in alone + ["C -O2 --opt"]]
        print("| %s | %.4f | %s |" % (name, c, " | ".join(cells)))
    print()
    print("Garbage collector, last -O2 run, and one --gc-stress run:")
    print()
    print("| benchmark | allocated MB | objects | collections | pause total ms (mark, sweep) | pause max ms "
          "| share of run | peak live KB | stress: collections | stress: run s |")
    print("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
    for name, gc, stress in gc_rows:
        print("| %s | %.1f | %d | %d | %.3f (%.3f, %.3f) | %.3f | %.0f%% | %.0f | %d | %.3f |" % (
            name, gc["allocated_bytes"] / 1e6, gc["allocated_objects"], gc["collections"],
            gc["pause_total_us"] / 1e3, gc["mark_us"] / 1e3, gc["sweep_us"] / 1e3, gc["pause_max_us"] / 1e3,
            100 * gc["pause_total_us"] / gc["run_us"], gc["peak_bytes"] / 1024, stress["collections"],
            stress["run_us"] / 1e6))
    return 0


# ------------------------------------------------------------------ main ----


def main(argv):
    optimize = "--opt" in argv
    flags = [a for a in argv if a in ("--gc-stress", "--stats")]
    args = [a for a in argv if a not in ("--opt", "--gc-stress", "--stats")]
    if args[:1] == ["--bench"]:
        return bench(int(args[1]) if len(args) > 1 else 5)
    if len(args) != 1:
        print(__doc__.split("Usage:")[1], file=sys.stderr)
        return 2
    with open(args[0], encoding="utf-8") as f:
        src = f.read()
    try:
        path = compile_to_file(src, optimize)
    except minilang.LangError as err:
        print("error: %s" % err, file=sys.stderr)
        return 1
    try:
        sys.stdout.flush()
        return subprocess.run([default_mlvm(), *flags, path]).returncode
    finally:
        os.remove(path)


if __name__ == "__main__":
    sys.setrecursionlimit(20000)
    sys.exit(main(sys.argv[1:]))
