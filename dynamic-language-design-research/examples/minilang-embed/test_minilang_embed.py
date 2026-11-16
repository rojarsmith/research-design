"""Tests for the embedding API. Build the two programs first (README.md):
    gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_embed test_embed.c minilang.c -lm
    gcc -std=c11 -Wall -Wextra -Werror -O2 -o mlfilter mlfilter.c minilang.c -lm
then:
    python3 -m unittest test_minilang_embed -v

The minilang programs in tests/ and programs/ are compiled with the stage 4
front end into a temporary directory. test_embed (the C tests) runs plain,
with --gc-stress and with a 4 KiB heap; two checks confirm that API misuse
aborts by default; mlfilter must print the same report plain and under
--gc-stress. If the shared library is built (see embed_ctypes.py), the
ctypes example runs too.

Nesting is a regression test for mlvm.c, which the library and the
command-line VM share: function literals nested 64 deep once overflowed the
loader's fixed temporary-root stack and aborted the process, every serve
session with it. It runs the command-line VM (../minilang-c/mlvm, built as in
that README) on programs nested 63, 64, 200 and 201 deep and on a bytecode
file that reads a non-cell slot as a cell.
"""
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "minilang-vm"))
sys.path.insert(0, os.path.join(HERE, "..", "minilang-c"))
import minilang_vm  # noqa: E402
import minilang  # noqa: E402
import minilang_c  # noqa: E402  the serve-protocol client

EXE = ".exe" if os.name == "nt" else ""
TEST_EMBED = os.path.join(HERE, "test_embed" + EXE)
MLFILTER = os.path.join(HERE, "mlfilter" + EXE)
LIBRARY = os.path.join(HERE, "minilang.dll" if os.name == "nt" else "libminilang.so")
MLVM = os.path.join(HERE, "..", "minilang-c", "mlvm" + EXE)
LEVELS = (63, 64, 200, 201)
BUILD = "gcc -std=c11 -Wall -Wextra -Werror -O2 -o %s %s.c minilang.c -lm"
OUT = None

REPORT = """\
Chen & Sons     TW  1200.00
Garnet Labs     TW  860.75
Acme Trading    TW  250.50
Fjord Tools     NO  100.00
total 2411.25 in 4 orders of at least 100
"""


def write_program(src, path, patch=None):
    old = sys.getrecursionlimit()
    sys.setrecursionlimit(max(old, 20000))   # the parser recurses once per nesting level
    try:
        script = minilang_vm.compile_program(minilang.parse(src))
    finally:
        sys.setrecursionlimit(old)
    if patch:
        patch(script)
    with open(path, "wb") as f:
        f.write(minilang_vm.dump_program(script))


def compile_program(src_path, out_dir):
    with open(src_path, encoding="utf-8") as f:
        src = f.read()
    name = os.path.splitext(os.path.basename(src_path))[0]
    write_program(src, os.path.join(out_dir, name + ".mlc"))


def nested_source(levels):
    """f1 declares f2, which declares f3 ... down to f<levels>: code objects nested `levels` deep."""
    body = "return %d;" % levels
    for k in range(levels, 1, -1):
        body = "fn f%d() { %s } return f%d();" % (k, body, k)
    return "fn f1() { %s }\nlet r = f1();\nprint(r);\nr;\n" % body


def read_param_as_cell(script):
    """Turn f's LOAD_LOCAL into LOAD_CELL: the verifier accepts it, the slot holds an int."""
    f = next(c for c in script.consts if isinstance(c, minilang_vm.Code))
    pc = next(i for i in range(0, len(f.code), 2) if f.code[i] == minilang_vm.LOAD_LOCAL)
    f.code[pc] = minilang_vm.LOAD_CELL


def setUpModule():
    global OUT
    for exe, name in ((TEST_EMBED, "test_embed"), (MLFILTER, "mlfilter")):
        if not os.path.exists(exe):
            raise FileNotFoundError("%s not found; build it in %s with:\n    %s" % (exe, HERE, BUILD % (name, name)))
    OUT = tempfile.mkdtemp(prefix="minilang-embed-")
    for src in glob.glob(os.path.join(HERE, "tests", "*.ml")) + glob.glob(os.path.join(HERE, "programs", "*.ml")):
        compile_program(src, OUT)
    for levels in LEVELS:
        write_program(nested_source(levels), os.path.join(OUT, "nest%d.mlc" % levels))
    write_program("fn f(a) { return a; }\nf(1);\n", os.path.join(OUT, "badcell.mlc"), read_param_as_cell)


def tearDownModule():
    shutil.rmtree(OUT, ignore_errors=True)


def run(*args):
    return subprocess.run(list(args), capture_output=True, text=True, encoding="utf-8")


class CTests(unittest.TestCase):
    def run_tests(self, *flags):
        proc = run(TEST_EMBED, *flags, OUT)
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        last = proc.stdout.strip().splitlines()[-1]
        self.assertRegex(last, r"^13 tests, \d+ checks, 0 failed checks")
        self.assertNotIn("FAIL", proc.stdout)
        return int(re.search(r"(\d+) checks", last).group(1))

    def test_api(self):
        self.assertGreater(self.run_tests(), 2000)

    def test_api_gc_stress(self):
        self.run_tests("--gc-stress")

    def test_api_small_heap(self):
        self.run_tests("--gc-min", "4096")

    def test_misuse_aborts_by_default(self):
        for kind, message in (("stale", "ml_get_int: stale or invalid handle"),
                              ("close", "ml_close called while the VM is running")):
            with self.subTest(kind=kind):
                proc = run(TEST_EMBED, "--crash", kind, OUT)
                self.assertNotEqual(proc.returncode, 0)
                self.assertIn("minilang: API misuse: " + message, proc.stderr)
                self.assertNotIn("no abort", proc.stdout)


@unittest.skipUnless(os.path.exists(MLVM), "../minilang-c/mlvm not built")
class Nesting(unittest.TestCase):
    def test_command_line_vm(self):
        for flags in ([], ["--gc-stress"]):
            for levels in LEVELS:
                with self.subTest(levels=levels, flags=flags):
                    proc = run(MLVM, *flags, os.path.join(OUT, "nest%d.mlc" % levels))
                    if levels <= 200:
                        self.assertEqual((proc.returncode, proc.stdout, proc.stderr), (0, "%d\n" % levels, ""))
                    else:
                        self.assertEqual(proc.returncode, 2)
                        self.assertIn("functions nested too deeply", proc.stderr)
            proc = run(MLVM, *flags, os.path.join(OUT, "badcell.mlc"))
            self.assertEqual(proc.returncode, 1)
            self.assertIn("SystemError: bytecode reads a slot that holds no cell", proc.stderr)

    def test_serve_sessions_survive(self):
        server = minilang_c.Server(MLVM)
        try:
            plain, stressed = server.new(), server.new(stress=True)
            for session in (plain, stressed):
                for levels in LEVELS:
                    lines, ending = server.run(session, os.path.join(OUT, "nest%d.mlc" % levels))
                    if levels <= 200:
                        self.assertEqual((lines, ending), ([str(levels)], "v int %d" % levels))
                    else:
                        self.assertEqual((lines, ending), ([], "l functions nested too deeply"))
                lines, ending = server.run(session, os.path.join(OUT, "badcell.mlc"))
                self.assertEqual(ending, "h SystemError bytecode reads a slot that holds no cell")
            self.assertTrue(server.alive())
            self.assertEqual((server.free(plain), server.free(stressed)), (0, 0))
        finally:
            server.close()


class Example(unittest.TestCase):
    def run_filter(self, *flags):
        proc = run(MLFILTER, *flags, os.path.join(OUT, "report.mlc"), os.path.join(HERE, "data", "orders.csv"))
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertEqual(proc.stdout, REPORT)
        self.assertIn("orders.csv:6: row skipped: negative amount -15 (script line 13)", proc.stderr)
        self.assertIn("mlfilter: 8 rows, 4 kept, 1 skipped", proc.stderr)

    def test_report(self):
        self.run_filter()

    def test_report_gc_stress(self):
        self.run_filter("--gc-stress")

    def test_a_missing_column_skips_rows(self):
        bad = os.path.join(OUT, "bad.csv")
        with open(bad, "w", encoding="utf-8") as f:
            f.write("id,customer,country,amount\n1,A,TW,5\n")   # no status column
        proc = run(MLFILTER, os.path.join(OUT, "report.mlc"), bad)
        self.assertIn("row skipped: no column named 'status'", proc.stderr)
        self.assertEqual(proc.returncode, 0)
        self.assertEqual(proc.stdout, "total 0.00 in 0 orders of at least 100\n")

    @unittest.skipUnless(os.path.exists(LIBRARY), "shared library not built")
    def test_ctypes_example(self):
        proc = run(sys.executable, os.path.join(HERE, "embed_ctypes.py"))
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertEqual(proc.stdout.splitlines(), [
            "shout -> HELLO, WORLD!", "safe_shout -> CTYPES!", "safe_shout -> caught: py_upper() expects a string",
            "shout -> status 1: py_upper() expects a string"])


if __name__ == "__main__":
    unittest.main()
