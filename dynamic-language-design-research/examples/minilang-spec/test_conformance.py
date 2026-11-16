"""Tests of the conformance suite itself. Run with: python3 -m unittest test_conformance -v

1. Every test file reads, and cites sections that exist in both SPEC files.
2. Planted deviations: the tree-walker with one rule changed must fail the
   tests written for that rule, and pass the rest at Core level when only a
   message text changed. This is how we know the suite can fail at all.
3. The command-line contract is classified as documented.
"""
import math
import os
import re
import sys
import unittest
from unittest import mock

import conformance

HERE = os.path.dirname(os.path.abspath(__file__))
TESTS = conformance.load_tests()
minilang, _, _ = conformance._import_engines()


def failing(make, level="full"):
    """Ids of the tests that a tree-walker variant fails at `level`."""
    engine = conformance.ApiEngine("tree")
    engine.make = make
    chosen = [t for t in TESTS if level == "full" or t.level == "core"]
    return {t.id for t in chosen if conformance.check(t, engine.run(t), level)}


class SuiteFiles(unittest.TestCase):
    def test_every_section_has_tests(self):
        self.assertEqual({t.section for t in TESTS}, set(conformance.SECTIONS))

    def test_cited_sections_exist_in_both_spec_files(self):
        for name in ("SPEC.md", "SPEC.zh-TW.md"):
            with open(os.path.join(HERE, name), encoding="utf-8") as f:
                headings = set(re.findall(r"^#{2,4} (\d+(?:\.\d+)?) ", f.read(), re.M))
            for t in TESTS:
                for sec in t.spec:
                    self.assertIn(sec, headings, "%s cites %s, missing from %s" % (t.id, sec, name))

    def test_legacy_tests_name_a_proposal(self):
        for t in TESTS:
            if t.status == "legacy":
                self.assertRegex(t.proposal, r"^P\d+$", t.id)
            if t.status == "impl":
                self.assertEqual(set(t.docs), {"tree", "vm", "c"}, t.id)

    def test_reference_tree_walker_passes(self):
        self.assertEqual(failing(lambda out: minilang.Interpreter(output=out)), set())


class PlantedDeviations(unittest.TestCase):
    def assertCaught(self, make, expected, level="full"):
        got = failing(make, level)
        self.assertTrue(expected <= got, "not caught: %s" % sorted(expected - got))
        return got

    def test_call_depth_one_short(self):
        class Shallow(minilang.Interpreter):
            MAX_DEPTH = 399
        self.assertCaught(lambda out: Shallow(output=out),
                          {"14-limits/call-depth-400", "14-limits/call-depth-boundary", "10-errors/throw-unwinds-calls"})

    def test_throw_nil_binds_nil(self):
        class NilThrow(minilang.Interpreter):
            def exec(self, node, env):
                if node.kind == "Throw":
                    value = self.eval(node.value, env)
                    err = minilang.RuntimeErr(minilang.stringify(value), node.line)
                    err.value = value
                    raise err
                return super().exec(node, env)
        got = self.assertCaught(lambda out: NilThrow(output=out), {"10-errors/throw-nil"})
        self.assertEqual(got, {"10-errors/throw-nil"})

    def test_truncated_remainder(self):
        class Truncating(minilang.Interpreter):
            def binary(self, node, left, right):
                if node.op == "%" and right != 0 and minilang.is_number(left) and minilang.is_number(right):
                    r = math.fmod(left, right)
                    return int(r) if isinstance(left, int) and isinstance(right, int) else r
                return super().binary(node, left, right)
        self.assertCaught(lambda out: Truncating(output=out), {"12-numbers/remainder"})

    def test_booleans_equal_numbers(self):
        with mock.patch.object(minilang, "values_equal", lambda a, b: a is b if isinstance(a, list) else a == b):
            self.assertCaught(lambda out: minilang.Interpreter(output=out), {"05-equality/cross-type-equality"})

    def test_six_digit_float_text(self):
        original = minilang.stringify

        def short(value):
            if isinstance(value, float) and not value.is_integer():
                return "%g" % value
            return original(value)
        with mock.patch.object(minilang, "stringify", short):
            self.assertCaught(lambda out: minilang.Interpreter(output=out),
                              {"12-numbers/float-text-core", "12-numbers/division"})

    def test_break_stops_at_function_boundary(self):
        class LexicalBreak(minilang.Interpreter):
            def call(self, callee, args, line):
                try:
                    return super().call(callee, args, line)
                except minilang.BreakSignal:
                    raise minilang.RuntimeErr("break outside a loop", line) from None
        full = self.assertCaught(lambda out: LexicalBreak(output=out),
                                 {"08-control/break-across-call", "08-control/break-across-two-calls",
                                  "08-control/break-from-loop-condition"})
        self.assertTrue(all(i.startswith("08-control/break") for i in full), full)
        self.assertEqual(failing(lambda out: LexicalBreak(output=out), "core"), set())

    def test_message_text_matters_only_at_full_level(self):
        class Reworded(minilang.Interpreter):
            def binary(self, node, left, right):
                if node.op in ("/", "%") and minilang.is_number(right) and right == 0:
                    raise minilang.RuntimeErr("divide by zero", node.line)
                return super().binary(node, left, right)
        self.assertCaught(lambda out: Reworded(output=out),
                          {"12-numbers/division-by-zero-message", "10-errors/runtime-error-messages"})
        self.assertEqual(failing(lambda out: Reworded(output=out), "core"), set())

    def test_wrong_error_line(self):
        class FirstLine(minilang.Interpreter):
            def run(self, src):
                try:
                    return super().run(src)
                except minilang.RuntimeErr as err:
                    err.line = 1
                    raise
        self.assertCaught(lambda out: FirstLine(output=out),
                          {"10-errors/line-of-operator", "10-errors/line-of-call", "10-errors/line-inside-function"},
                          "core")


class CommandLineContract(unittest.TestCase):
    def outcome(self, script):
        engine = conformance.CliEngine("fake", [sys.executable, "-c", script])
        return engine.run(TESTS[0])

    def test_classification(self):
        ok = self.outcome("print('hi')")
        self.assertEqual((ok.kind, ok.output), ("ok", "hi\n"))
        err = self.outcome("import sys; sys.stderr.write('error: [line 3] two\\nlines\\n'); sys.exit(1)")
        self.assertEqual((err.kind, err.line, err.message), ("error", 3, "two\nlines"))
        fatal = self.outcome("raise OverflowError('x')")
        self.assertEqual((fatal.kind, fatal.detail), ("fatal", "OverflowError"))
        crash = self.outcome("import sys; sys.stderr.write('boom\\n'); sys.exit(70)")
        self.assertEqual(crash.kind, "crash")

    def test_cli_error_matches_either_error_kind(self):
        outcome = conformance.Outcome("error", line=2)
        self.assertTrue(conformance.Expect("syntax-error 2").matches(outcome))
        self.assertTrue(conformance.Expect("runtime-error 2").matches(outcome))
        self.assertFalse(conformance.Expect("runtime-error 3").matches(outcome))
        self.assertFalse(conformance.Expect("fatal").matches(outcome))


if __name__ == "__main__":
    unittest.main()
