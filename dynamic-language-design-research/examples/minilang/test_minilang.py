"""Tests for minilang. Run with: python3 -m unittest test_minilang -v"""
import unittest

import minilang


def run(src):
    """Run a program and return its printed lines."""
    out = []
    minilang.Interpreter(output=out.append).run(src)
    return out


def error_of(src):
    try:
        run(src)
    except minilang.LangError as err:
        return str(err)
    raise AssertionError("expected an error")


class LexerParserTests(unittest.TestCase):
    def test_tokens_carry_line_numbers(self):
        toks = minilang.tokenize('let a = 1;\n# comment\nlet b = "x";')
        self.assertEqual([t.kind for t in toks][:5], ["let", "IDENT", "=", "NUMBER", ";"])
        self.assertEqual(toks[5].line, 3)

    def test_precedence(self):
        self.assertEqual(run("print(1 + 2 * 3 - 4 / 2);"), ["5"])
        self.assertEqual(run("print((1 + 2) * 3);"), ["9"])
        self.assertEqual(run("print(-2 * 3);"), ["-6"])
        self.assertEqual(run("print(not false and true);"), ["true"])

    def test_syntax_error_reports_line(self):
        self.assertEqual(error_of("let a = 1;\nlet b = ;"), "[line 2] unexpected token ';'")
        self.assertIn("missing '}'", error_of("fn f() {\n  print(1);\n"))


class SemanticsTests(unittest.TestCase):
    def test_variables_and_scopes(self):
        src = """
        let x = 1;
        { let x = 2; print(x); }
        print(x);
        x = 3;
        print(x);
        """
        self.assertEqual(run(src), ["2", "1", "3"])

    def test_control_flow(self):
        src = """
        let i = 0; let total = 0;
        while (true) {
          i = i + 1;
          if (i % 2 == 0) { total = total + i; }
          else if (i > 7) { break; }
        }
        print(total, i);
        """
        self.assertEqual(run(src), ["20 9"])

    def test_recursion(self):
        src = """
        fn fib(n) { if (n < 2) { return n; } return fib(n - 1) + fib(n - 2); }
        print(fib(20));
        """
        self.assertEqual(run(src), ["6765"])

    def test_closures_capture_variables(self):
        src = """
        fn counter() {
          let n = 0;
          return fn() { n = n + 1; return n; };
        }
        let c1 = counter(); let c2 = counter();
        print(c1(), c1(), c2());
        """
        self.assertEqual(run(src), ["1 2 1"])

    def test_closures_in_loop_share_variable(self):
        # Each closure captures the same `i`, as in Python and JavaScript's `var`.
        src = """
        let fns = []; let i = 0;
        while (i < 3) { push(fns, fn() { return i; }); i = i + 1; }
        print(fns[0](), fns[2]());
        """
        self.assertEqual(run(src), ["3 3"])

    def test_lists_and_strings(self):
        src = """
        let xs = [1, 2]; push(xs, 3); xs[0] = 10;
        print(xs, len(xs), xs[-1], len("héllo"), "a" + "b", str(1.5) + "!");
        let last = pop(xs);
        print(last, xs, [1] + [2], "abc"[1], type(xs), type(nil), type(print));
        """
        self.assertEqual(run(src), ["[10, 2, 3] 3 3 5 ab 1.5!", "3 [10, 2] [1, 2] b list nil function"])

    def test_truthiness_and_equality(self):
        src = """
        print(0 == false, nil == false, 1 == 1.0, "1" == 1, [1] == [1]);
        if (0) { print("zero is truthy"); }
        if ("") { print("empty string is truthy"); }
        print(nil or "default", 0 and "x", false or nil);
        """
        self.assertEqual(run(src), ["false false true false false", "zero is truthy",
                                    "empty string is truthy", "default x nil"])

    def test_exceptions(self):
        src = """
        fn safe_div(a, b) {
          try { return a / b; }
          catch (e) { print("caught: " + e); return nil; }
        }
        print(safe_div(6, 3), safe_div(1, 0));
        try { throw [1, 2]; } catch (v) { print(type(v), v[1]); }
        try { undefined_name; } catch (e) { print(e); }
        """
        self.assertEqual(run(src), ["caught: division by zero", "2 nil", "list 2",
                                    "undefined variable 'undefined_name'"])

    def test_runtime_errors_carry_line_numbers(self):
        self.assertEqual(error_of("let a = 1;\n\nprint(a + \"x\");"),
                         "[line 3] '+' expects two numbers, two strings or two lists, got number and string")
        self.assertEqual(error_of("fn f(a) {}\nf();"), "[line 2] <fn f> expects 1 argument(s), got 0")
        self.assertIn("stack overflow", error_of("fn loop() { return loop(); }\nloop();"))
        self.assertIn("out of range", error_of("let xs = [1];\nxs[5];"))

    def test_trailing_expression_value_for_repl(self):
        interp = minilang.Interpreter(output=lambda s: None)
        self.assertEqual(interp.run("let a = 40; a + 2;"), 42)
        self.assertIsNone(interp.run("let b = 1;"))


if __name__ == "__main__":
    unittest.main()
