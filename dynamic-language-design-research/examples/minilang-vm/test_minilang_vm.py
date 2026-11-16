"""Tests for minilang_vm. Run with: python3 -m unittest test_minilang_vm -v

1. The stage 2 suite, examples/minilang/test_minilang.py, imported and run
   unchanged with minilang.Interpreter replaced by the VM, once without and
   once with the compiler optimizations.
2. Differential tests: the same program on the tree-walker and on the VM must
   print the same lines and end the same way (value, error text, or escaped
   host signal).
3. VM internals: code objects, constant pool, line table, maximum stack
   depth, jump patching, closures and upvalues, the disassembler.
"""
import contextlib
import functools
import glob
import inspect
import io
import os
import sys
import unittest
from unittest import mock

import minilang_vm  # also puts ../minilang on sys.path
import minilang
import test_minilang

HERE = os.path.dirname(os.path.abspath(__file__))
MINILANG_PROGRAMS = os.path.join(HERE, "..", "minilang", "programs")

VM_ENGINES = (
    ("vm", minilang_vm.VM),
    ("vm --opt", functools.partial(minilang_vm.VM, optimize=True)),
)


# ------------------------------------------- the stage 2 suite on the VM ----


class OnVM:
    """Mixin: run an inherited test_minilang TestCase with the VM in place of
    minilang.Interpreter. test_minilang looks the class up at call time."""

    engine = staticmethod(minilang_vm.VM)

    def setUp(self):
        patcher = mock.patch.object(minilang, "Interpreter", self.engine)
        patcher.start()
        self.addCleanup(patcher.stop)
        super().setUp()

    def test_suite_really_uses_the_vm(self):
        self.assertIsInstance(minilang.Interpreter(output=lambda s: None), minilang_vm.VM)


class OnOptimizedVM(OnVM):
    engine = staticmethod(functools.partial(minilang_vm.VM, optimize=True))


class LexerParserOnVM(OnVM, test_minilang.LexerParserTests):
    pass


class SemanticsOnVM(OnVM, test_minilang.SemanticsTests):
    pass


class LexerParserOnOptimizedVM(OnOptimizedVM, test_minilang.LexerParserTests):
    pass


class SemanticsOnOptimizedVM(OnOptimizedVM, test_minilang.SemanticsTests):
    pass


# ------------------------------------------------- differential testing ----


def outcome(make, src):
    """Printed lines plus how the run ended, in a form both engines share."""
    out = []
    try:
        result = make(out.append).run(src)
    except minilang.LangError as err:
        return out, ("error", type(err).__name__, str(err))
    except minilang.ReturnSignal as sig:
        return out, ("escaped ReturnSignal", minilang.stringify(sig.value))
    except minilang.BreakSignal:
        return out, ("escaped BreakSignal",)
    return out, ("value", minilang.stringify(result))


class Differential(unittest.TestCase):
    def assertSame(self, src):
        """Assert every VM mode matches the tree-walker; return the tree-walker's outcome."""
        expected = outcome(minilang_vm.TreeWalker, src)
        for name, make in VM_ENGINES:
            self.assertEqual(outcome(make, src), expected, "%s differs on:\n%s" % (name, src))
        return expected

    def assertPrints(self, src, lines):
        out, end = self.assertSame(src)
        self.assertEqual(out, lines)
        return end


class ScopingTests(Differential):
    """Corners of minilang's 'environment filled as let runs' scoping."""

    def test_closure_sees_a_local_declared_after_it(self):
        src = """
        let x = "global";
        fn outer() {
          let f = fn() { return x; };
          let before = f();
          let x = "local";
          return [before, f()];
        }
        print(outer());
        """
        self.assertPrints(src, ["[global, local]"])

    def test_assignment_from_closure_follows_the_same_rule(self):
        src = """
        let y = "g";
        fn late() { let set = fn(v) { y = v; }; let y = 0; set(5); return y; }
        fn early() { let set = fn(v) { y = v; }; set(7); let y = 0; return y; }
        print(late(), y);
        print(early(), y);
        """
        self.assertPrints(src, ["5 g", "0 7"])

    def test_same_function_reads_outer_until_its_own_let(self):
        src = """
        let x = 1;
        fn f() { let a = x; let x = 2; return [a, x]; }
        print(f());
        { print(x); let x = x + 10; print(x); }
        print(x);
        """
        self.assertPrints(src, ["[1, 2]", "1", "11", "1"])

    def test_mutual_recursion_of_local_functions(self):
        src = """
        fn parity(n) {
          fn is_even(k) { if (k == 0) { return true; } return is_odd(k - 1); }
          fn is_odd(k) { if (k == 0) { return false; } return is_even(k - 1); }
          return is_even(n);
        }
        print(parity(10), parity(7));
        """
        self.assertPrints(src, ["true false"])

    def test_recursive_function_through_let(self):
        src = """
        fn make() {
          let fact = fn(n) { if (n < 2) { return 1; } return n * fact(n - 1); };
          return fact(6);
        }
        print(make());
        """
        self.assertPrints(src, ["720"])

    def test_loop_body_gets_a_fresh_scope_each_iteration(self):
        src = """
        fn make() {
          let fns = []; let shared = []; let i = 0;
          while (i < 3) {
            let j = i;
            push(fns, fn() { return j; });
            push(shared, fn() { return i; });
            i = i + 1;
          }
          return [fns[0](), fns[1](), fns[2](), shared[0](), shared[2]()];
        }
        print(make());
        """
        self.assertPrints(src, ["[0, 1, 2, 3, 3]"])

    def test_redeclaration_and_parameters(self):
        src = """
        fn dup(a, a) { return a; }
        fn rebind(a) { let a = a + 1; return a; }
        fn captured_param(n) { return fn() { n = n + 1; return n; }; }
        let c = captured_param(10);
        let x = 1; let x = x + 1;
        print(dup(1, 2), rebind(1), c(), c(), x);
        """
        self.assertPrints(src, ["2 2 11 12 2"])

    def test_three_levels_of_closures(self):
        src = """
        fn a() {
          let x = 1;
          fn b() { fn c() { x = x + 1; return x; } return c; }
          return [b(), b()];
        }
        let pair = a();
        print(pair[0](), pair[1](), pair[0]());
        """
        self.assertPrints(src, ["2 3 4"])

    def test_catch_variable_can_be_captured(self):
        src = """
        let saved = nil;
        try { throw "boom"; } catch (e) { saved = fn() { return e; }; }
        print(saved());
        """
        self.assertPrints(src, ["boom"])

    def test_repl_globals_persist_between_runs(self):
        for _, make in (("tree-walker", minilang_vm.TreeWalker),) + VM_ENGINES:
            interp = make(output=lambda s: None)
            interp.run("let a = 1; fn inc() { a = a + 1; return a; }")
            self.assertEqual(interp.run("inc(); inc();"), 3)


class ControlFlowTests(Differential):
    def test_throw_nil_is_caught_as_the_string_nil(self):
        # RuntimeErr stores the message when the thrown value is None, so the
        # tree-walker hands `catch` the string "nil". The VM reproduces it.
        src = """
        try { throw nil; } catch (e) { print(type(e), e); }
        try { throw 0; } catch (e) { print(type(e), e); }
        """
        self.assertPrints(src, ["string nil", "number 0"])

    def test_errors_unwind_through_calls(self):
        src = """
        fn deep(n) { if (n == 0) { throw "bottom"; } return deep(n - 1); }
        fn forever() { return forever(); }
        try { deep(50); } catch (e) { print("caught", e); }
        try { forever(); } catch (e) { print(e); }
        try { forever(); } catch (e) { print(e); }
        fn safe() { try { return deep(3); } catch (e) { return "inner " + e; } }
        print(safe());
        """
        self.assertPrints(src, ["caught bottom", "stack overflow (max call depth 400)",
                                "stack overflow (max call depth 400)", "inner bottom"])

    def test_call_depth_limit_is_the_same(self):
        src = """
        fn d(n) { if (n == 0) { return 0; } return d(n - 1); }
        print(d(399));
        try { d(400); } catch (e) { print(e); }
        """
        self.assertPrints(src, ["0", "stack overflow (max call depth 400)"])

    def test_break_inside_a_called_function_stops_the_callers_loop(self):
        # The tree-walker's break is a host exception, so it crosses calls.
        src = """
        fn stop() { break; }
        fn check(n) { if (n > 2) { break; } return true; }
        let i = 0;
        while (true) { i = i + 1; if (i == 3) { stop(); } }
        let k = 0;
        while (check(k)) { k = k + 1; }
        let n = 0;
        while (true) { try { n = n + 1; stop(); } catch (e) { print("not reached"); } }
        print(i, k, n);
        """
        self.assertPrints(src, ["3 3 1"])

    def test_top_level_return_and_stray_break_escape_like_the_tree_walker(self):
        self.assertEqual(self.assertSame("print(1); return 2; print(3);"),
                         (["1"], ("escaped ReturnSignal", "2")))
        self.assertEqual(self.assertSame("try { return 5; } catch (e) {}")[1], ("escaped ReturnSignal", "5"))
        self.assertEqual(self.assertSame("break;")[1], ("escaped BreakSignal",))
        self.assertEqual(self.assertSame("fn f() { break; }\nf();")[1], ("escaped BreakSignal",))

    def test_nested_loops_break_and_try(self):
        src = """
        let out = [];
        let i = 0;
        while (i < 3) {
          let j = 0;
          while (true) {
            j = j + 1;
            try { if (j == 2) { throw "skip"; } if (j > 3) { break; } push(out, i * 10 + j); }
            catch (e) { push(out, e); }
          }
          i = i + 1;
        }
        print(out);
        """
        self.assertSame(src)

    def test_logical_operators_return_operands(self):
        self.assertPrints('print(nil or "d", 0 and "x", false or nil, 1 and nil, not 0, not nil);',
                          ["d x nil nil false true"])


class ErrorLineTests(Differential):
    """Runtime errors must name the same line in both engines."""

    PROGRAMS = (
        'let a = 1;\nprint(a +\n  "x");',
        "fn f() {\n  return g();\n}\nf();",
        "let s = fn() {\n  missing\n    =\n    1;\n};\ns();",   # name, '=' and value on three lines
        "let xs = [1, 2];\nprint(len(xs),\n  xs[\n    5]);",
        "print(len(\n  5));",
        "len(1,\n  2);",
        'let t = "abc";\nt[0]\n  = "z";',
        'print(-\n  "a");',
        "let x = nil;\nx(\n);",
        'fn f(a) {\n  throw\n    "up";\n}\nf(1);',
        "let k = 1;\nk[0];",
        "print(1 / 0);",
        'print(1 <\n "a");',
        "let xs = [1];\nxs[1.5];",
        'num("x" +\n"y");',
        "fn f(a, b) {}\nf(1);",
        "undefined_fn(1, 2);",
    )

    def test_error_lines_match(self):
        for src in self.PROGRAMS:
            end = self.assertSame(src)[1]
            self.assertEqual(end[0], "error", src)
            self.assertTrue(end[2].startswith("[line "), end)

    def test_some_lines_explicitly(self):
        self.assertEqual(self.assertSame(self.PROGRAMS[2])[1][2], "[line 3] assignment to undefined variable 'missing'")
        self.assertEqual(self.assertSame(self.PROGRAMS[3])[1][2], "[line 3] index 5 out of range for length 2")
        self.assertEqual(self.assertSame(self.PROGRAMS[9])[1][2], "[line 2] up")


class OperatorMatrixTests(Differential):
    """Every operator on every kind of value: same result or same error."""

    VALUES = ("0", "1", "-3", "2.5", "0.0", '"a"', '""', '"b"', "true", "false", "nil", "[1]", "print",
              "fn() {}", "10000000000000000000000")

    def test_binary_operators(self):
        for op in ("+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">="):
            src = "\n".join("try { print(%s %s %s); } catch (e) { print(e); }" % (a, op, b)
                            for a in self.VALUES for b in self.VALUES)
            self.assertSame(src)

    def test_unary_and_logical_operators(self):
        src = "\n".join("try { print(-(%s), not %s, %s and 1, %s or 1); } catch (e) { print(e); }" % ((v,) * 4)
                        for v in self.VALUES)
        self.assertSame(src)

    def test_indexing(self):
        lines = []
        for obj in ("[1, 2, 3]", '"abc"', "5", "nil"):
            for index in ("0", "-1", "-3", "3", "1.0", "1.5", '"0"', "true", "nil"):
                lines.append("try { print(%s[%s]); } catch (e) { print(e); }" % (obj, index))
                lines.append("try { let o = %s; o[%s] = 9; print(o); } catch (e) { print(e); }" % (obj, index))
        self.assertSame("\n".join(lines))

    def test_builtins_and_calls(self):
        src = """
        let calls = ["len(1)", "push(1, 2)", "pop([])", "num(nil)", "str(1, 2)", "clock(1)"];
        try { print(len([1, 2], 3)); } catch (e) { print(e); }
        try { print(push(1, 2)); } catch (e) { print(e); }
        try { print(pop([])); } catch (e) { print(e); }
        try { print(num("x")); } catch (e) { print(e); }
        try { print(num("12") + num("-3") + num("1.5")); } catch (e) { print(e); }
        try { print(1(2)); } catch (e) { print(e); }
        try { print(fn(a) {}(1, 2)); } catch (e) { print(e); }
        print(print, fn() {}, type(fn() {}), str([1, "a", nil, true, 2.0]));
        """
        self.assertSame(src)


class ExampleProgramTests(Differential):
    def test_minilang_example_programs(self):
        paths = sorted(glob.glob(os.path.join(MINILANG_PROGRAMS, "*.ml")))
        self.assertTrue(paths)
        for path in paths:
            with open(path, encoding="utf-8") as f:
                src = f.read()
            if "clock()" in src:   # fib.ml prints its own running time; make it deterministic
                src = src.replace("clock()", "0")
            self.assertSame(src)


# --------------------------------------------------------- VM internals ----


def compile_src(src, optimize=False):
    return minilang_vm.VM(optimize=optimize).compile(src)


def all_code(co):
    yield co
    for const in co.consts:
        if isinstance(const, minilang_vm.Code):
            yield from all_code(const)


def find_code(co, name):
    return next(c for c in all_code(co) if c.name == name)


def opnames(co):
    return [minilang_vm.OPNAMES[op] for _, op, _ in co.instructions()]


class CodeObjectTests(unittest.TestCase):
    def test_opcode_table_is_consistent(self):
        for index, name in enumerate(minilang_vm.OPNAMES):
            self.assertEqual(getattr(minilang_vm, name), index, name)

    def test_constant_pool_is_deduplicated_by_type(self):
        co = compile_src('print(1, 1.0, true, 1, "1", 1.0, nil, "print");')
        self.assertEqual(co.consts, ["print", 1, 1.0, True, "1", None])
        self.assertEqual([type(c) for c in co.consts], [str, int, float, bool, str, type(None)])

    def test_line_table_is_run_length_and_maps_every_offset(self):
        src = "let a = 1;\nlet b = a +\n  2;\n\nprint(a, b);"
        co = compile_src(src)
        # 0 LOAD_CONST 1, 2 DEFINE_GLOBAL a, 4 LOAD_GLOBAL a, 6 LOAD_CONST 2, 8 ADD, 10 DEFINE_GLOBAL b, 12 ...
        self.assertEqual(co.linetable, [(0, 1), (4, 2), (6, 3), (8, 2), (12, 5)])
        self.assertLess(len(co.linetable), len(co.code) // 2)
        lines = {offset: co.line_at(offset) for offset, _, _ in co.instructions()}
        self.assertEqual(lines[2], 1)   # DEFINE_GLOBAL a
        self.assertEqual(lines[6], 3)   # the literal 2 is on line 3
        self.assertEqual(lines[8], 2)   # the ADD belongs to the '+' on line 2
        self.assertEqual(lines[10], 2)  # DEFINE_GLOBAL b belongs to the let on line 2

    def test_max_stack_depth_is_computed(self):
        co = compile_src("print(1 + 2 * (3 + 4));")
        self.assertEqual(co.max_stack, 5)   # print, 1, 2, 3, 4
        self.assertEqual(find_code(compile_src("fn f(n) { return f(n - 1) + f(n - 2); }"), "f").max_stack, 4)
        self.assertEqual(compile_src("try { 1; } catch (e) { print(e); }").max_stack, 2)

    def test_stack_depth_rejects_inconsistent_paths(self):
        co = minilang_vm.Code("bad", [])
        V = minilang_vm
        co.code = [V.LOAD_CONST, 0, V.LOAD_CONST, 0, V.JUMP_IF_FALSE, 8, V.LOAD_CONST, 0, V.HALT, 0]
        with self.assertRaises(AssertionError):
            V.stack_depth(co)

    def test_frames_are_sized_from_the_code_object(self):
        co = find_code(compile_src("fn f(a, b) { let c = a + b; return c * 2; }"), "f")
        self.assertEqual((co.nparams, co.nslots, co.max_stack), (2, 3, 2))
        self.assertEqual(len(co.padding), co.nslots - co.nparams + co.max_stack)

    def test_vm_frames_do_not_use_host_recursion(self):
        vm = minilang_vm.VM(output=lambda s: None)
        co = vm.compile("fn d(n) { if (n == 0) { return 0; } return d(n - 1); }\nd(399);")
        old = sys.getrecursionlimit()
        sys.setrecursionlimit(len(inspect.stack()) + 60)   # far below 400 nested calls
        try:
            self.assertEqual(vm.execute(co), 0)
        finally:
            sys.setrecursionlimit(old)


class JumpTests(unittest.TestCase):
    SRC = """
    fn f(x) {
      let r = 0;
      while (x > 0) {
        if (x == 5) { break; } else if (x % 2 == 0) { r = r + x; } else { r = r - 1; }
        x = x - 1;
      }
      try { if (r and x or nil) { throw r; } } catch (e) { r = e; }
      return r;
    }
    """

    def test_every_jump_is_patched_to_an_instruction(self):
        for optimize in (False, True):
            for co in all_code(compile_src(self.SRC, optimize)):
                for offset, op, arg in co.instructions():
                    if op in minilang_vm.JUMP_OPCODES:
                        self.assertNotEqual(arg, -1, "unpatched jump at %d" % offset)
                        self.assertTrue(0 <= arg < len(co.code) and arg % 2 == 0, (co.name, offset, arg))

    def test_if_else_layout(self):
        co = compile_src("if (c) { print(1); } else { print(2); }")
        code = co.code
        names = opnames(co)
        jif = names.index("JUMP_IF_FALSE") * 2
        jump = names.index("JUMP") * 2
        self.assertEqual(code[jif + 1], jump + 2)                   # false -> first instruction of else
        self.assertEqual(minilang_vm.OPNAMES[code[code[jump + 1]]], "LOAD_CONST")   # then -> after the else
        self.assertEqual(code[code[jump + 1] + 2], minilang_vm.HALT)

    def test_break_jumps_to_the_loop_exit_and_loops_are_tabled(self):
        co = compile_src("while (true) { if (x) { break; } }\nprint(1);")
        (start, end), = co.loops
        jumps = [(offset, arg) for offset, op, arg in co.instructions() if op == minilang_vm.JUMP]
        self.assertIn(end, [arg for _, arg in jumps])                # the break
        self.assertIn(start, [arg for _, arg in jumps])              # the back edge
        self.assertEqual(co.code[end], minilang_vm.LOAD_GLOBAL)      # print(1) follows the loop

    def test_exception_table(self):
        co = compile_src("try { try { a; } catch (e) { b; } } catch (e) { c; }")
        inner, outer = co.handlers
        self.assertTrue(outer[0] <= inner[0] < inner[1] <= outer[1])   # inner listed first
        self.assertEqual(co.handler_at(inner[0]), inner[2])
        self.assertEqual(co.handler_at(inner[2]), outer[2])            # an error in the inner catch goes outward


class ClosureTests(unittest.TestCase):
    def test_only_captured_variables_become_cells(self):
        co = compile_src("fn counter() { let n = 0; let unused = 1; return fn() { n = n + 1; return n; }; }")
        counter = find_code(co, "counter")
        inner = find_code(co, "<anonymous>")
        self.assertEqual(opnames(counter)[:4], ["MAKE_CELL", "LOAD_CONST", "STORE_CELL", "LOAD_CONST"])
        self.assertIn("STORE_LOCAL", opnames(counter))                # `unused` stays a plain slot
        self.assertEqual(inner.upval_specs, ((True, counter.slot_names.index("n")),))
        self.assertIn("LOAD_UPVAL", opnames(inner))
        self.assertIn("STORE_UPVAL", opnames(inner))

    def test_upvalues_are_threaded_through_intermediate_functions(self):
        co = compile_src("fn a() { let x = 1; fn b() { fn c() { return x; } return c; } return b; }")
        self.assertEqual(find_code(co, "b").upval_specs, ((True, 0),))
        self.assertEqual(find_code(co, "c").upval_specs, ((False, 0),))

    def test_captured_parameters_are_boxed_on_entry(self):
        co = find_code(compile_src("fn f(a, b) { return fn() { return b; }; }"), "f")
        self.assertEqual(co.code[:2], [minilang_vm.CELL_ARG, 1])

    def test_chain_only_for_names_declared_after_the_closure(self):
        co = compile_src("fn f() { let early = 1; let g = fn() { return early + late; }; let late = 2; return g(); }")
        g = find_code(co, "<anonymous>")
        self.assertEqual(opnames(g)[:2], ["LOAD_UPVAL", "LOAD_CHAIN"])
        self.assertEqual(g.chains, [("late", (1,), "global", "late")])

    def test_closures_are_minilang_functions(self):
        out = []
        minilang_vm.VM(output=out.append).run("fn f() {}\nprint(f, type(f), f == f, fn() {});")
        self.assertEqual(out, ["<fn f> function true <fn <anonymous>>"])


class OptimizationTests(unittest.TestCase):
    def test_optimized_code_is_shorter(self):
        src = "fn fib(n) { if (n < 2) { return n; } return fib(n - 1) + fib(n - 2); }"
        plain = find_code(compile_src(src), "fib")
        fast = find_code(compile_src(src, optimize=True), "fib")
        self.assertLess(len(fast.code), len(plain.code))
        self.assertIn("JUMP_IF_NOT_LT", opnames(fast))
        self.assertIn("SUB_CONST", opnames(fast))

    def test_each_optimization_can_be_chosen(self):
        src = "fn f(i) { while (i < 9) { i = i + 1; } return i; }"
        # optimization -> (instruction it saves, instruction it adds)
        expect = {"assign-statement": ("DUP", "STORE_LOCAL"), "const-operand": ("LOAD_CONST", "ADD_CONST"),
                  "compare-jump": ("JUMP_IF_FALSE", "JUMP_IF_NOT_LT")}
        plain = opnames(find_code(compile_src(src), "f"))
        for name, (saved, added) in expect.items():
            chosen = opnames(find_code(compile_src(src, optimize=[name]), "f"))
            self.assertLess(chosen.count(saved), plain.count(saved), name)
            self.assertIn(added, chosen, name)
            self.assertLess(len(chosen), len(plain), name)
        with self.assertRaises(ValueError):
            minilang_vm.VM(optimize=["no-such-thing"])

    def test_superinstructions_keep_error_messages(self):
        for src in ('let s = "a";\nprint(s\n - 1);', 'let s = "a";\nif (s <\n 1) {}', 'let s = true;\nprint(s + 1);'):
            expected = outcome(minilang_vm.TreeWalker, src)
            self.assertEqual(outcome(functools.partial(minilang_vm.VM, optimize=True), src), expected)


class DisassemblerTests(unittest.TestCase):
    def test_listing(self):
        text = minilang_vm.disassemble(compile_src("fn add1(x) {\n  return x + 1;\n}\nprint(add1(2));"))
        self.assertEqual(text, "\n".join([
            "== <script>  params=- slots=0 upvalues=0 max_stack=3 ==",
            "constants: 0=<code add1>, 1='add1', 2='print', 3=2",
            "    0    1  MAKE_CLOSURE         0 (<code add1>)",
            "    2    |  DEFINE_GLOBAL        1 ('add1')",
            "    4    4  LOAD_GLOBAL          2 ('print')",
            "    6    |  LOAD_GLOBAL          1 ('add1')",
            "    8    |  LOAD_CONST           3 (2)",
            "   10    |  CALL                 1",
            "   12    |  CALL                 1",
            "   14    |  HALT                 ",
            "line table: 0:1 4:4",
            "",
            "== add1  params=x slots=1 upvalues=0 max_stack=2 ==",
            "constants: 0=1, 1=None",
            "    0    2  LOAD_LOCAL           0 (x)",
            "    2    |  LOAD_CONST           0 (1)",
            "    4    |  ADD                  ",
            "    6    |  RETURN               ",
            "    8    1  LOAD_CONST           1 (None)",
            "   10    |  RETURN               ",
            "line table: 0:2 8:1",
        ]))

    def test_listing_shows_tables_upvalues_and_chains(self):
        text = minilang_vm.disassemble(compile_src(
            "fn f() { let g = fn() { return later; }; let later = 1; while (true) { try { break; } catch (e) {} } }"))
        self.assertIn("upvalues: 0=later(slot 1)", text)
        self.assertIn("LOAD_CHAIN           0 (later: upvalue 0 if defined, then global)", text)
        self.assertRegex(text, r"exception: \d+-\d+ -> \d+")
        self.assertRegex(text, r"loop: \d+-\d+")

    def test_command_line(self):
        path = os.path.join(MINILANG_PROGRAMS, "closures.ml")
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            self.assertEqual(minilang_vm.main(["--dis", path]), 0)
        self.assertIn("== make_counter  params=step", buffer.getvalue())


if __name__ == "__main__":
    unittest.main()
