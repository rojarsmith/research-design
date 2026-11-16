"""Tests for the C VM. Build it first (README.md), then:
    python3 -m unittest test_minilang_c -v

1. The stage 2 suite (../minilang/test_minilang.py) and the stage 4
   differential tests (../minilang-vm/test_minilang_vm.py), imported and run
   unchanged with the C VM patched in: plain, with compiler optimizations,
   and with the collector running on every allocation (--gc-stress).
2. The bytecode file format: Python round trip, C disassembly identical to the
   Python disassembler, and the loader rejecting damaged files.
3. Numbers: float repr and str against Python for random doubles; big
   integers, mixed int/float arithmetic and num() against the tree-walker.
4. Memory management: statistics, the growth policy, cycles are collected, a
   long run does not grow the heap, and every session frees all its bytes.
"""
import functools
import glob
import math
import os
import random
import struct
import subprocess
import sys
import unittest
from unittest import mock

import minilang_c  # also puts ../minilang-vm and ../minilang on sys.path
import difftest
import minilang
import minilang_vm
import test_minilang
import test_minilang_vm

HERE = os.path.dirname(os.path.abspath(__file__))
PROGRAM_DIRS = [os.path.join(HERE, "..", "minilang", "programs"), os.path.join(HERE, "..", "minilang-vm", "programs"),
                os.path.join(HERE, "programs")]
MLVM = None


def setUpModule():
    global MLVM
    MLVM = minilang_c.default_mlvm()   # fails with the build command if mlvm is missing


def tearDownModule():
    for server in minilang_c._SERVERS.values():
        server.close()
    if minilang_c.LEAKS:
        raise AssertionError("sessions left bytes on the heap: %r" % minilang_c.LEAKS)


def program_paths():
    return sorted(p for d in PROGRAM_DIRS for p in glob.glob(os.path.join(d, "*.ml")))


def read_program(path):
    with open(path, encoding="utf-8") as f:
        src = f.read()
    return src.replace("clock()", "0")   # fib.ml prints its own running time


C_ENGINES = (
    ("C", minilang_c.CVM),
    ("C --opt", functools.partial(minilang_c.CVM, optimize=True)),
    ("C --gc-stress", functools.partial(minilang_c.CVM, gc_stress=True)),
    ("C --opt --gc-stress", functools.partial(minilang_c.CVM, optimize=True, gc_stress=True)),
)


def run_c(src, **options):
    out = []
    minilang_c.CVM(output=out.append, **options).run(src)
    return out


def outcome(make, src):
    """Printed lines and how the run ended, including Python-level errors."""
    return difftest.outcome(make, src)


# ----------------------------------- the stage 2 and stage 4 suites on C ----


class OnC:
    """Mixin: run an inherited test_minilang TestCase with the C VM in place
    of minilang.Interpreter."""

    engine = staticmethod(minilang_c.CVM)

    def setUp(self):
        patcher = mock.patch.object(minilang, "Interpreter", self.engine)
        patcher.start()
        self.addCleanup(patcher.stop)
        super().setUp()

    def test_suite_really_uses_the_c_vm(self):
        self.assertIsInstance(minilang.Interpreter(output=lambda s: None), minilang_c.CVM)


class OnOptimizedC(OnC):
    engine = staticmethod(functools.partial(minilang_c.CVM, optimize=True))


class OnStressedC(OnC):
    engine = staticmethod(functools.partial(minilang_c.CVM, gc_stress=True))


class LexerParserOnC(OnC, test_minilang.LexerParserTests):
    pass


class SemanticsOnC(OnC, test_minilang.SemanticsTests):
    pass


class LexerParserOnOptimizedC(OnOptimizedC, test_minilang.LexerParserTests):
    pass


class SemanticsOnOptimizedC(OnOptimizedC, test_minilang.SemanticsTests):
    pass


class SemanticsOnStressedC(OnStressedC, test_minilang.SemanticsTests):
    pass


class DifferentialOnC:
    """Mixin: stage 4's differential tests compare every engine in
    test_minilang_vm.VM_ENGINES with the tree-walker; swap in the C engines."""

    def setUp(self):
        patcher = mock.patch.object(test_minilang_vm, "VM_ENGINES", C_ENGINES)
        patcher.start()
        self.addCleanup(patcher.stop)
        super().setUp()


class ScopingOnC(DifferentialOnC, test_minilang_vm.ScopingTests):
    pass


class ControlFlowOnC(DifferentialOnC, test_minilang_vm.ControlFlowTests):
    pass


class ErrorLinesOnC(DifferentialOnC, test_minilang_vm.ErrorLineTests):
    pass


class OperatorMatrixOnC(DifferentialOnC, test_minilang_vm.OperatorMatrixTests):
    pass


class ExampleProgramsOnC(DifferentialOnC, test_minilang_vm.ExampleProgramTests):
    pass


class Differential(unittest.TestCase):
    def assertSame(self, src, engines=C_ENGINES):
        expected = outcome(minilang_c.TreeWalker, src)
        for name, make in engines:
            self.assertEqual(outcome(make, src), expected, "%s differs on:\n%s" % (name, src))
        return expected


class AllProgramsOnC(Differential):
    def test_every_program_file(self):
        paths = program_paths()
        self.assertGreaterEqual(len(paths), 8)
        for path in paths:
            with self.subTest(os.path.basename(path)):
                self.assertSame(read_program(path))


# ------------------------------------------------- the bytecode file ----


def emit(src, optimize=False):
    script = minilang_vm.VM(optimize=optimize).compile(src)
    return script, minilang_vm.dump_program(script, optimized=optimize)


def run_mlvm(*args, data=None):
    """Run the mlvm binary on a file (or on bytes written to a temp file)."""
    if data is not None:
        path = minilang_c.temp_path()
        with open(path, "wb") as f:
            f.write(data)
        args = args + (path,)
    proc = subprocess.run([MLVM, *args], capture_output=True)
    return proc.returncode, proc.stdout.decode("utf-8").replace("\r\n", "\n"), proc.stderr.decode("utf-8")


class FormatTests(unittest.TestCase):
    def test_python_round_trip_is_exact(self):
        for path in program_paths():
            for optimize in (False, True):
                script, data = emit(read_program(path), optimize)
                back = minilang_vm.load_program(data)
                self.assertEqual(minilang_vm.disassemble(back), minilang_vm.disassemble(script))
                self.assertEqual(minilang_vm.dump_program(back, optimized=optimize), data)

    def test_loaded_code_runs_on_the_python_vm(self):
        src = read_program(os.path.join(PROGRAM_DIRS[0], "closures.ml"))
        out = []
        vm = minilang_vm.VM(output=out.append)
        vm.execute(minilang_vm.load_program(emit(src)[1]))
        self.assertEqual(out, ["1 2 10 3", "[1, 4, 9]"])

    def test_header(self):
        data = emit("print(1);")[1]
        self.assertEqual(data[:4], b"MLBC")
        self.assertEqual(struct.unpack("<HHHH", data[4:12]), (1, 0, len(minilang_vm.OPCODES), 0))
        self.assertEqual(struct.unpack("<HHHH", emit("print(1);", True)[1][4:12])[3], 1)

    def test_constants_of_every_kind(self):
        src = 'print(nil, true, false, 7, -9223372036854775808, 10000000000000000000000, 2.5, "h\\u00e9", fn() {});'
        src = src.replace("\\u00e9", "\u00e9")
        script, data = emit(src)
        consts = minilang_vm.load_program(data).consts
        self.assertIn(10000000000000000000000, consts)
        self.assertIn("h\u00e9", consts)
        self.assertEqual(outcome(minilang_c.CVM, src), outcome(minilang_c.TreeWalker, src))

    def test_python_reader_rejects_other_versions(self):
        data = bytearray(emit("print(1);")[1])
        for offset, value, message in ((0, b"XLBC", "not a minilang"), (4, b"\x02\x00", "version"),
                                       (6, b"\x01\x00", "version"), (8, b"\x05\x00", "opcodes")):
            bad = bytes(data[:offset]) + value + bytes(data[offset + len(value):])
            with self.assertRaisesRegex(ValueError, message):
                minilang_vm.load_program(bad)


class LoaderTests(unittest.TestCase):
    def test_c_disassembly_matches_python(self):
        for path in program_paths():
            for optimize in (False, True):
                script, data = emit(read_program(path), optimize)
                code, out, err = run_mlvm("--dis", data=data)
                self.assertEqual(code, 0, err)
                self.assertEqual(out, minilang_vm.disassemble(script) + "\n", path)

    def test_command_line_file_mode(self):
        path = minilang_c.compile_to_file(read_program(os.path.join(PROGRAM_DIRS[0], "errors.ml")))
        code, out, err = run_mlvm(path)
        self.assertEqual(code, 1)
        self.assertTrue(out.startswith("ok: 42\n"))
        self.assertEqual(err.strip(), "error: [line 21] index 10 out of range for length 3")

    def assertRejected(self, data, message):
        code, out, err = run_mlvm(data=bytes(data))
        self.assertEqual(code, 2, (message, out, err))
        self.assertIn(message, err)
        self.assertEqual(out, "")

    def test_header_checks(self):
        data = bytearray(emit("print(1);")[1])
        self.assertRejected(b"PK\x03\x04" + data[4:], "not a minilang bytecode file")
        self.assertRejected(data[:4] + b"\x02\x00" + data[6:], "unsupported bytecode version 2.0")
        self.assertRejected(data[:6] + b"\x01\x00" + data[8:], "unsupported bytecode version 1.1")
        self.assertRejected(data[:8] + b"\x2d\x00" + data[10:], "bytecode has 45 opcodes, this VM has 46")
        self.assertRejected(data + b"\x00", "trailing bytes")

    def test_every_truncation_is_rejected(self):
        data = emit("fn f(a) { let s = [a, \"x\", 2.5]; try { return s[a]; } catch (e) { return e; } }\nprint(f(1));")[1]
        for cut in range(len(data)):
            code, out, err = run_mlvm(data=data[:cut])
            self.assertEqual(code, 2, (cut, out, err))
            self.assertTrue("truncated" in err or "not a minilang" in err or "count larger" in err, (cut, err))

    def patched(self, src, offset, op, arg, **fields):
        """Bytecode of src with the instruction at offset replaced, and other
        Code fields of the script overridden."""
        script = minilang_vm.VM().compile(src)
        if offset is not None:
            script.code[offset:offset + 2] = [op, arg]
        for name, value in fields.items():
            setattr(script, name, value)
        return minilang_vm.dump_program(script)

    def test_operand_checks(self):
        V = minilang_vm
        src = "let x = 1;\nprint(x + 2);"   # 0 LOAD_CONST 1, 2 DEFINE_GLOBAL x, 4 LOAD_GLOBAL print, ...
        self.assertEqual(minilang_vm.VM().compile(src).code[:2], [V.LOAD_CONST, 0])
        self.assertRejected(self.patched(src, 0, V.LOAD_CONST, 99), "constant index out of range")
        self.assertRejected(self.patched(src, 0, 99, 0), "unknown opcode 99")
        self.assertRejected(self.patched(src, 0, V.LOAD_GLOBAL, 0), "not a string constant")
        self.assertRejected(self.patched(src, 0, V.ADD_CONST, 1), "not a numeric constant")
        self.assertRejected(self.patched(src, 0, V.MAKE_CLOSURE, 0), "not a code constant")
        self.assertRejected(self.patched(src, 0, V.LOAD_LOCAL, 0), "slot out of range")
        self.assertRejected(self.patched(src, 0, V.LOAD_UPVAL, 0), "upvalue out of range")
        self.assertRejected(self.patched(src, 0, V.LOAD_CHAIN, 0), "chain out of range")
        self.assertRejected(self.patched(src, 0, V.JUMP, 3), "bad jump target")
        self.assertRejected(self.patched(src, 0, V.POP, 0), "stack underflow")
        self.assertRejected(self.patched(src, 0, V.CALL, 0), "stack underflow")
        last = len(minilang_vm.VM().compile(src).code) - 2   # HALT
        self.assertRejected(self.patched(src, last, V.POP, 0), "runs off the end")
        loop = "let i = 0;\nwhile (i < 3) { i = i + 1; }"
        code = minilang_vm.VM().compile(loop).code
        back_edge = max(at for at in range(0, len(code), 2) if code[at] == V.JUMP)
        self.assertRejected(self.patched(loop, back_edge, V.JUMP, back_edge - 2), "stack depth")
        self.assertRejected(self.patched(src, None, 0, 0, linetable=[(2, 1)]), "line table must start at offset 0")
        self.assertRejected(self.patched(src, None, 0, 0, handlers=[(0, 4, 99)]), "bad exception table entry")
        self.assertRejected(self.patched(src, None, 0, 0, loops=[(0, 3)]), "bad loop table entry")
        self.assertRejected(self.patched(src, None, 0, 0, params=["p"], nslots=1, slot_names=["p"]),
                            "takes no parameters")

    def test_max_stack_from_the_file_is_checked(self):
        src = "print(1 + 2 * 3);"
        self.assertEqual(minilang_vm.VM().compile(src).max_stack, 4)
        self.assertRejected(self.patched(src, None, 0, 0, max_stack=3), "max_stack 3 is smaller than the computed 4")

    def test_inner_functions_are_verified(self):
        src = "fn f(a) { return a; }\nprint(f(1));"
        script = minilang_vm.VM().compile(src)
        inner = script.consts[0]
        inner.code[0:2] = [minilang_vm.LOAD_LOCAL, 5]
        self.assertRejected(minilang_vm.dump_program(script), "f: slot out of range")
        script = minilang_vm.VM().compile("fn f() { let x = 1; return fn() { return x; }; }")
        inner = next(c for c in script.consts[0].consts if isinstance(c, minilang_vm.Code))
        inner.upval_specs = ((True, 9),)
        self.assertRejected(minilang_vm.dump_program(script), "upvalue 0 of <anonymous> out of range")

    def test_strings_must_be_utf8(self):
        script, data = emit('print("ok");')
        bad = data.replace(b"\x02\x00\x00\x00ok", b"\x02\x00\x00\x00\xffk")
        self.assertRejected(bad, "not valid UTF-8")


# ------------------------------------------------------------- numbers ----


class FloatTextTests(unittest.TestCase):
    def test_repr_and_str_of_doubles_match_python(self):
        rng = random.Random(5)
        values = [0.0, -0.0, 1.0, -1.5, 0.1, 0.3, 1 / 3, 2 / 3, 1e16, 1e15, 9007199254740993.0, 1e-4, 1e-5,
                  123456789012345680.0, 1e22, 1e23, 5e-324, 2.2250738585072014e-308, 1.7976931348623157e308,
                  float("inf"), float("-inf"), float("nan")]
        values += [2.0 ** k for k in range(-1074, 1024, 7)]
        values += [struct.unpack("<d", struct.pack("<Q", rng.getrandbits(64)))[0] for _ in range(20000)]
        values += [rng.uniform(-1e6, 1e6) for _ in range(5000)]
        values += [float(rng.randint(-10 ** 6, 10 ** 6)) / rng.choice([1, 3, 7, 10, 100]) for _ in range(5000)]
        lines = "".join("%016x\n" % struct.unpack("<Q", struct.pack("<d", v))[0] for v in values)
        proc = subprocess.run([MLVM, "--selftest-float"], input=lines.encode(), capture_output=True, check=True)
        got = proc.stdout.decode().split("\n")
        for value, line in zip(values, got):
            expected = "%s %s" % (repr(value), minilang.stringify(value))
            self.assertEqual(line, expected, value.hex() if math.isfinite(value) else value)
        self.assertEqual(len(got) - 1, len(values))


class NumberTests(Differential):
    def test_big_integer_arithmetic(self):
        rng = random.Random(11)

        def big():
            digits = rng.choice([1, 5, 18, 19, 20, 30, 60, 120, 300])
            n = rng.randrange(10 ** (digits - 1), 10 ** digits)
            return str(n) if rng.random() < 0.5 else "-" + str(n)

        lines = []
        for _ in range(150):
            a, b = big(), big()
            for op in ("+", "-", "*", "/", "%", "<", "==", ">="):
                lines.append("try { print(%s %s %s); } catch (e) { print(e); }" % (a, op, b))
            lines.append("print(-%s, str(%s), len(str(%s)), num(str(%s)) == %s);" % (a, a, a, a, a))
        self.assertSame("\n".join(lines))

    def test_int64_boundaries(self):
        src = """
        let big = 9223372036854775807; let small = -9223372036854775807 - 1;
        print(big + 1, small - 1, -small, big * 2, small * -1, small % -1, small / -1, big - big);
        print(big + 1 - 1 == big, (big + 1) / 2, 3037000499 * 3037000499, 3037000500 * 3037000500);
        let x = 1; let i = 0;
        while (i < 70) { x = x * 3; i = i + 1; }
        print(x, x % 1000003, x / 3 == x / 3.0, x - x * 2, x > 1.0e30 * 1.0, x == x + 0.0);
        """
        self.assertSame(src)

    def test_mixed_int_and_float(self):
        values = ["0", "1", "-7", "3", "2.5", "-0.5", "0.1", "1e300 * 1.0", "9007199254740993",
                  "10000000000000000000000", "1.0e22 * 1.0", "num(\"inf\")", "num(\"-inf\")", "num(\"nan\")"]
        values[7] = "1" + "0" * 300 + ".0"
        values[10] = "10000000000000000000000.0"
        lines = []
        for a in values:
            for b in values:
                for op in ("+", "-", "*", "/", "%", "<", "<=", "==", "!="):
                    lines.append("try { print(%s %s %s); } catch (e) { print(e); }" % (a, op, b))
        self.assertSame("\n".join(lines))

    def test_float_overflow_of_huge_integers_is_a_python_error(self):
        huge = "1" + "0" * 400
        for src in ("print(%s + 0.5);" % huge, "print(%s / 3);" % huge, "num(%s);" % huge,
                    "try { print(%s * 1.0); } catch (e) { print(\"not caught\"); }" % huge):
            self.assertEqual(self.assertSame(src)[1][0], "host")
        self.assertEqual(self.assertSame("print(%s / %s);" % (huge, huge[:-1])), (["10"], ("value", "nil")))

    def test_num_follows_pythons_int_and_float(self):
        texts = ["42", "-3", " 7 ", "+5", "--5", "-", "", " ", "1.5", ".5", "5.", "1e3", "1E-2", "1e", "e3", "inf",
                 "-Infinity", "nAn", "1_000", "1__0", "_1", "1_", "1_0.5_5", "0x10", "1.5.2", "007", "-007",
                 "\t12\n", "1 2", "abc", "it's", 'say "hi"', "back\\\\slash", "\u00a012\u3000", "\u00e9", "12\u00e9",
                 "9" * 30, "-" + "9" * 30, "1" * 4301, "nan(1)", "infinit", "1e400", "-1e400", "4.9e-324"]
        lines = []
        for t in texts:
            literal = t.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\t", "\\t")
            lines.append('try { let v = num("%s"); print(type(v), v); } catch (e) { print(e); }' % literal)
        for v in ("true", "false", "nil", "[1, \"a\", [2.0]]", "print", "fn() {}", "2.5", "12"):
            lines.append("try { print(num(%s)); } catch (e) { print(e); }" % v)
        lines.append('let a = [1]; push(a, a); try { num(a); } catch (e) { print(e); }')
        self.assertSame("\n".join(lines))

    def test_strings_are_counted_in_code_points(self):
        src = 'let s = "h\u00e9llo \u65e5\u672c \U0001f600!"; print(len(s), s[1], s[-2], s[6], s + s, s < "i", "\u00e9" > "z");'
        self.assertSame(src)


# ---------------------------------------------------- memory management ----


class MemoryTests(unittest.TestCase):
    def stats_after(self, src, **options):
        vm = minilang_c.CVM(output=lambda s: None, **options)
        vm.run(src)
        return vm.stats()

    def test_statistics_are_consistent(self):
        s = self.stats_after(read_program(os.path.join(HERE, "..", "minilang-vm", "programs", "bench_concat.ml")))
        self.assertGreater(s["collections"], 0)
        self.assertGreaterEqual(s["allocated_bytes"], s["peak_bytes"])
        self.assertGreaterEqual(s["peak_bytes"], s["live_bytes"])
        self.assertEqual(s["allocated_objects"] - s["freed_objects"], s["live_objects"])
        self.assertGreaterEqual(s["pause_total_us"], s["pause_max_us"])

    def test_growth_policy(self):
        # The next collection starts when the heap is twice what survived the
        # last one, but never below the minimum heap size.
        s = self.stats_after("let keep = []; let i = 0; while (i < 20000) { push(keep, str(i)); i = i + 1; }",
                             min_heap=4096)
        self.assertGreater(s["collections"], 5)
        self.assertGreaterEqual(s["next_gc"], 4096)
        s2 = self.stats_after("let i = 0; while (i < 20000) { let t = str(i); i = i + 1; }", min_heap=1 << 30)
        self.assertEqual(s2["collections"], 0)

    def test_stress_mode_collects_on_every_allocation(self):
        s = self.stats_after("let i = 0; while (i < 100) { let t = [str(i)]; i = i + 1; }", gc_stress=True)
        self.assertGreaterEqual(s["collections"], 300)   # str(), the list and its items each allocate
        self.assertEqual(s["stress"], 1)

    def test_cycles_are_collected(self):
        # Every iteration makes two cycles: a list that contains itself, and a
        # local function that refers to itself (closure -> cell -> closure).
        # Reference counting alone would never free them.
        src = read_program(os.path.join(HERE, "programs", "gc_cycles.ml"))
        out = []
        vm = minilang_c.CVM(output=out.append, min_heap=64 * 1024)
        vm.run(src)
        s = vm.stats()
        self.assertEqual(out, outcome(minilang_c.TreeWalker, src)[0])
        self.assertGreater(s["freed_objects"], 50000)
        self.assertLess(s["peak_bytes"], 512 * 1024)
        self.assertLess(s["live_bytes"], 64 * 1024 * 2 + 4096)

    def test_long_run_does_not_grow_the_heap(self):
        src = read_program(os.path.join(HERE, "programs", "gc_churn.ml"))
        peaks = []
        for rounds in (2000, 20000):
            vm = minilang_c.CVM(output=lambda s: None, min_heap=256 * 1024)
            vm.run(src.replace("let rounds = 20000;", "let rounds = %d;" % rounds))
            s = vm.stats()
            peaks.append(s["peak_bytes"])
            self.assertGreater(s["allocated_bytes"], rounds * 300)
        self.assertLess(peaks[1], 2 * 256 * 1024 + 64 * 1024)   # bounded by the policy, not by the run length
        self.assertLess(peaks[1], peaks[0] * 1.25)

    def test_a_value_held_only_by_a_local_slot_survives(self):
        # The list's only reference is local slot `a` when MAKE_CELL (block
        # entry) allocates and, under --gc-stress, collects. No instruction
        # since the call has published the stack top, so this fails if
        # MAKE_CELL does not SYNC() first. Random programs did not catch that
        # planted bug; this test was written for it.
        src = """
        let g = [1, 2, 3];
        fn f() {
          let a = g;
          g = nil;
          { let x = 1; let h = fn() { return x; }; }
          return a;
        }
        print(f());
        """
        expected = outcome(minilang_c.TreeWalker, src)
        for optimize in (False, True):
            self.assertEqual(outcome(functools.partial(minilang_c.CVM, gc_stress=True, optimize=optimize), src), expected)

    def test_sessions_free_everything(self):
        server = minilang_c.shared_server()
        for options in ({}, {"stress": True}):
            session = server.new(**options)
            path = minilang_c.compile_to_file(read_program(os.path.join(PROGRAM_DIRS[0], "closures.ml")))
            lines, ending = server.run(session, path)
            self.assertEqual(ending, "v nil")
            self.assertEqual(server.free(session), 0)

    def test_deep_structures_do_not_overflow_the_c_stack(self):
        # The mark phase uses an explicit gray stack, so a list nested 200,000
        # deep is traced without recursion. Printing it is a RecursionError in
        # both engines (a Python error in the tree-walker).
        src = "let a = []; let i = 0; while (i < 200000) { a = [a]; i = i + 1; } print(len(a));"
        self.assertEqual(run_c(src, min_heap=4096), ["1"])
        self.assertEqual(outcome(minilang_c.CVM, src + " print(a);")[1], ("host", "RecursionError"))
        inside = "fn show(x) { print(x); }\n" + src + "\ntry { show(a); } catch (e) { print(e); }"
        self.assertEqual(outcome(minilang_c.CVM, inside)[0][-1], "stack overflow (host recursion limit)")


if __name__ == "__main__":
    unittest.main()
