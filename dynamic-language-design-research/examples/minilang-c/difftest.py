#!/usr/bin/env python3
"""difftest: differential testing of the C VM against the tree-walker.

Every program file, and a corpus of randomly generated programs, runs on
minilang's tree-walker (the reference) and on the C VM; everything observable
must match: each printed line and how the run ended (the trailing value, a
runtime error's full text with its line, an escaped top-level return or
break, or a Python-level error such as OverflowError).

The generator writes programs that terminate: loops count to at most 4, user
functions spend a global `_fuel` and return early when it runs out, and the
only unbounded recursion is a deliberate one that hits the depth limit of
400. They use closures (including over names declared later and over loop
variables), shadowing, try/catch/throw, break across calls, lists, strings,
64-bit boundaries, big integers, floats, num() on odd strings, and errors
caught and uncaught, so the collector has cyclic and acyclic garbage to free.

Usage:
    python3 difftest.py                            # program files + 1,000 random programs
    python3 difftest.py --count 10000 --seed 1 --heap stress
    python3 difftest.py --show 3                   # print three generated programs
Options:
    --count N         random programs (default 1000)
    --seed S          first seed; program i uses seed S + i (default 1)
    --heap H,...      default (1 MiB before the first collection), small (4 KiB),
                      stress (collect on every allocation); default: all three
    --modes M,...     plain and/or opt bytecode (default: both)
    --save DIR        write mismatching programs to DIR
    --mlvm PATH       the C VM binary
"""
import argparse
import collections
import glob
import os
import random
import sys
import time

import minilang_c  # also puts ../minilang-vm and ../minilang on sys.path
import minilang

HERE = os.path.dirname(os.path.abspath(__file__))
PROGRAM_DIRS = ("../minilang/programs", "../minilang-vm/programs", "programs")
HEAPS = {"default": {}, "small": {"min_heap": 4096}, "stress": {"gc_stress": True}}


# ------------------------------------------------------------- outcomes ----


def outcome(make, src):
    """(printed lines, ending); Python-level errors are part of the outcome."""
    out = []
    try:
        ending = ("value", make(out.append).run(src))
    except minilang.LangError as err:
        return out, ("error", type(err).__name__, str(err))
    except minilang.ReturnSignal as sig:
        ending = ("escaped ReturnSignal", sig.value)
    except minilang.BreakSignal:
        return out, ("escaped BreakSignal",)
    except RecursionError:   # the message depends on where Python's stack ran out
        return out, ("host", "RecursionError")
    except (OverflowError, ValueError) as err:
        return out, ("host", type(err).__name__, str(err))
    try:   # the value itself may not print: a list inside itself, an integer of 4301 digits
        return out, (ending[0], minilang.stringify(ending[1]))
    except RecursionError:
        return out, ("host", "RecursionError")
    except ValueError as err:
        return out, ("host", "ValueError", str(err))


# ------------------------------------------------------------ generator ----

SMALL = ["0", "1", "2", "3", "7", "10", "42", "100", "-1", "-5"]
BIG = ["9223372036854775807", "9223372036854775808", "4611686018427387904", "18446744073709551616",
       "10000000000000000000000", "123456789012345678901234567890", "3037000500"]
FLOATS = ["0.5", "2.5", "0.1", "3.0", "1.25", "100.0", "0.0", "0.3", "10000000000000000.0", "1.5"]
STRINGS = ["", "a", "ab", "xyz", "hé", "42", "-7", "3.5", " 12 ", "1e3", "inf", "nan", "1_000", "--5",
           "abc", "日本", "0.1", "-0", "+8", "x y"]
BINARY = ["+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">="]


class Scope:
    def __init__(self, parent=None):
        self.parent = parent
        self.names = {}   # name -> kind: num, str, list, fn, any; functions as ("fn", arity)

    def all(self):
        scope, seen = self, {}
        while scope:
            for name, kind in scope.names.items():
                seen.setdefault(name, kind)
            scope = scope.parent
        return seen


class Gen:
    def __init__(self, seed):
        self.r = random.Random(seed)
        self.n = 0

    def fresh(self, prefix):
        self.n += 1
        return "%s%d" % (prefix, self.n)

    def chance(self, p):
        return self.r.random() < p

    # -- expressions --
    def literal(self, kind=None):
        r = self.r
        kind = kind or r.choice(["num", "num", "num", "str", "bool", "nil", "float", "big"])
        if kind == "num":
            return r.choice(SMALL)
        if kind == "big":
            return r.choice(BIG)
        if kind == "float":
            return r.choice(FLOATS)
        if kind == "str":
            return '"%s"' % r.choice(STRINGS)
        if kind == "bool":
            return r.choice(["true", "false"])
        if kind == "list":
            return "[%s]" % ", ".join(self.literal() for _ in range(r.randint(0, 3)))
        return "nil"

    def var(self, scope, kinds):
        names = [n for n, k in scope.all().items() if (k[0] if isinstance(k, tuple) else k) in kinds]
        return self.r.choice(names) if names else None

    def expr(self, scope, depth=0, kind=None):
        r = self.r
        if depth >= 3 or self.chance(0.25):
            if self.chance(0.6):
                name = self.var(scope, [kind] if kind else ["num", "str", "list", "any", "fn"])
                if name:
                    return name
            if self.chance(0.02):
                return "undefined_name"
            return self.literal({"num": r.choice(["num", "num", "float", "big"]), "str": "str", "list": "list"}.get(kind))
        choice = r.random()
        if kind == "list" or (kind is None and choice < 0.1):
            if self.chance(0.5):
                return "[%s]" % ", ".join(self.expr(scope, depth + 1) for _ in range(r.randint(0, 3)))
            return "%s + %s" % (self.expr(scope, depth + 1, "list"), self.expr(scope, depth + 1, "list"))
        if kind == "str" or (kind is None and choice < 0.2):
            return r.choice([
                lambda: "%s + %s" % (self.expr(scope, depth + 1, "str"), self.expr(scope, depth + 1, "str")),
                lambda: "str(%s)" % self.expr(scope, depth + 1),
                lambda: "type(%s)" % self.expr(scope, depth + 1),
                lambda: "%s[%s]" % (self.expr(scope, depth + 1, "str"), r.choice(["0", "1", "-1", "2"])),
            ])()
        if choice < 0.55 or kind == "num":
            op = r.choice(BINARY[:5]) if kind == "num" else r.choice(BINARY)
            left = self.expr(scope, depth + 1, "num" if self.chance(0.85) else None)
            right = self.expr(scope, depth + 1, "num" if self.chance(0.85) else None)
            sep = "\n    " if self.chance(0.1) else " "   # operators keep the line of their token
            return "(%s %s%s%s)" % (left, op, sep, right)
        if choice < 0.62:
            return "%s %s %s" % (self.expr(scope, depth + 1), r.choice(["and", "or"]), self.expr(scope, depth + 1))
        if choice < 0.68:
            return r.choice(["-", "not "]) + self.expr(scope, depth + 1)
        if choice < 0.76:
            if self.chance(0.5):
                return "len(%s)" % self.expr(scope, depth + 1, r.choice(["str", "list"]))
            return r.choice(["num(%s)", "len(str(%s))"]) % self.expr(scope, depth + 1)
        if choice < 0.82:
            name = self.var(scope, ["list"])
            if name:
                return "%s[%s]" % (name, r.choice(["0", "1", "-1", "len(%s) - 1" % name, "2.0", "1.5"]))
            return self.literal("list") + "[0]"
        if choice < 0.94:
            return self.call(scope, depth)
        params = [self.fresh("q") for _ in range(r.randint(0, 2))]
        inner = Scope(scope)
        for p in params:
            inner.names[p] = "any"
        body = self.expr(inner, depth + 1)
        func = "fn(%s) { return %s; }" % (", ".join(params), body)
        if self.chance(0.5):
            return "(%s)(%s)" % (func, ", ".join(self.expr(scope, depth + 1) for _ in params))
        return func

    def call(self, scope, depth):
        funcs = [(n, k) for n, k in scope.all().items() if isinstance(k, tuple)]
        if funcs and self.chance(0.8):
            name, (_, arity) = self.r.choice(funcs)
            if self.chance(0.05):
                arity += 1   # wrong number of arguments
            return "%s(%s)" % (name, ", ".join(self.expr(scope, depth + 1) for _ in range(arity)))
        return "%s(%s)" % (self.r.choice(["str", "type", "len"]), self.expr(scope, depth + 1))

    # -- statements --
    def block(self, scope, ctx, count, indent):
        inner = Scope(scope)
        return [self.stmt(inner, ctx, indent) for _ in range(count)]

    def stmt(self, scope, ctx, indent):
        r = self.r
        pad = "  " * indent
        depth_ok = ctx["depth"] < 3
        roll = r.random()
        if roll < 0.14:
            name = self.fresh("v")
            kind = r.choice(["num", "num", "str", "list", "any"])
            value = self.expr(scope, 0, None if kind == "any" else kind)
            scope.names[name] = kind
            if self.chance(0.8):   # keep most programs running past a type error
                return '%slet %s = nil; try { %s = %s; } catch (e) { print("E", e); }' % (pad, name, name, value)
            return "%slet %s = %s;" % (pad, name, value)
        if roll < 0.22:
            name = self.var(scope, ["num", "str", "list", "any"])
            if name and not name.startswith("c"):
                if self.chance(0.8):
                    return '%stry { %s = %s; } catch (e) { print("E", e); }' % (pad, name, self.expr(scope, 1))
                return "%s%s = %s;" % (pad, name, self.expr(scope, 1))
            return "%sprint(%s);" % (pad, self.expr(scope))
        if roll < 0.40:
            args = ", ".join(self.expr(scope) for _ in range(r.randint(1, 3)))
            if self.chance(0.8):
                return '%stry { print(%s); } catch (e) { print("E", e); }' % (pad, args)
            return "%sprint(%s);" % (pad, args)
        if roll < 0.47 and depth_ok:
            ctx2 = dict(ctx, depth=ctx["depth"] + 1)
            lines = ["%sif (%s) {" % (pad, self.expr(scope, 1))]
            lines += self.block(scope, ctx2, r.randint(1, 2), indent + 1)
            if self.chance(0.5):
                lines.append("%s} else {" % pad)
                lines += self.block(scope, ctx2, r.randint(1, 2), indent + 1)
            lines.append(pad + "}")
            return self.guard("\n".join(lines), pad)
        if roll < 0.55 and depth_ok:
            counter = self.fresh("c")
            loop_scope = Scope(scope)
            loop_scope.names[counter] = "num"
            ctx2 = dict(ctx, depth=ctx["depth"] + 1, loop=True)
            lines = ["%slet %s = 0;" % (pad, counter),
                     "%swhile (%s < %d) {" % (pad, counter, r.randint(1, 4)),
                     "%s  %s = %s + 1;" % (pad, counter, counter)]
            lines += self.block(loop_scope, ctx2, r.randint(1, 3), indent + 1)
            if self.chance(0.2):
                lines.append("%s  if (%s > 1) { break; }" % (pad, counter))
            lines.append(pad + "}")
            return self.guard("\n".join(lines), pad)
        if roll < 0.62 and depth_ok:
            return self.function(scope, ctx, indent)
        if roll < 0.70 and depth_ok:
            ctx2 = dict(ctx, depth=ctx["depth"] + 1)
            lines = ["%stry {" % pad]
            lines += self.block(scope, ctx2, r.randint(1, 2), indent + 1)
            if self.chance(0.5):
                lines.append("%s  throw %s;" % (pad, self.expr(scope, 1)))
            catch = self.fresh("e")
            lines.append("%s} catch (%s) {" % (pad, catch))
            inner = Scope(scope)
            inner.names[catch] = "any"
            lines.append('%s  print("caught", %s);' % (pad, catch))
            if self.chance(0.3):
                lines += self.block(inner, ctx2, 1, indent + 1)
            lines.append(pad + "}")
            return "\n".join(lines)
        if roll < 0.77:
            name = self.var(scope, ["list"])
            if name:
                return r.choice([
                    "%spush(%s, %s);" % (pad, name, self.expr(scope, 1)),
                    "%stry { %s[%s] = %s; } catch (e) { print(e); }" % (pad, name, r.choice(["0", "-1", "3", "1.0"]),
                                                                      self.expr(scope, 1)),
                    "%stry { print(pop(%s)); } catch (e) { print(e); }" % (pad, name),
                    "%spush(%s, %s);" % (pad, name, name),   # a list inside itself: cyclic garbage
                ])
            return "%sprint(%s);" % (pad, self.literal("list"))
        if roll < 0.80 and ctx["loop"]:
            return "%sif (%s) { break; }" % (pad, self.expr(scope, 1))
        if roll < 0.84 and ctx["func"]:
            return "%sif (%s) { return %s; }" % (pad, self.expr(scope, 1), self.expr(scope, 1))
        if roll < 0.88 and depth_ok:
            name = self.var(scope, ["num", "str", "list", "any"])
            if name and not name.startswith("c"):
                return "%s{ let %s = %s; print(%s); }" % (pad, name, self.expr(scope, 1), name)
        if roll < 0.95:
            return self.guard(pad + self.template(scope, ctx), pad)
        return self.guard("%s%s;" % (pad, self.call(scope, 0)), pad)

    def guard(self, text, pad):
        """Most statements that can fail are wrapped in try/catch so that the
        program keeps going; the rest end it with an uncaught error."""
        if self.chance(0.8):
            return '%stry {\n%s\n%s} catch (e) { print("E", e); }' % (pad, text, pad)
        return text

    def function(self, scope, ctx, indent):
        r = self.r
        pad = "  " * indent
        name = self.fresh("f")
        params = [self.fresh("p") for _ in range(r.randint(0, 3))]
        anonymous = self.chance(0.3)
        if not anonymous:
            scope.names[name] = ("fn", len(params))   # visible in its own body: recursion spends fuel
        inner = Scope(scope)
        for p in params:
            inner.names[p] = "any"
        ctx2 = dict(ctx, depth=ctx["depth"] + 1, loop=False, func=True)
        head = ("%slet %s = fn(%s) {" if anonymous else "%sfn %s(%s) {") % (pad, name, ", ".join(params))
        lines = [head, "%s  _fuel = _fuel - 1; if (_fuel < 0) { return nil; }" % pad]
        lines += self.block(inner, ctx2, r.randint(1, 4), indent + 1)
        lines.append("%s  return %s;" % (pad, self.expr(inner, 1)))
        lines.append(pad + ("};" if anonymous else "}"))
        if anonymous:
            scope.names[name] = ("fn", len(params))
        return "\n".join(lines)

    def template(self, scope, ctx):
        """Corner cases that random statements rarely hit on their own."""
        r = self.r
        k = self.fresh("t")
        which = r.randrange(11)
        if which == 0:   # a closure reads a name declared after it (LOAD_CHAIN)
            later = r.choice([self.fresh("late"), self.var(scope, ["num", "str", "any"]) or "late0"])
            return ("fn %s() { let get = fn() { return %s; }; let before = nil; "
                    "try { before = get(); } catch (e) { before = e; } let %s = %s; return [before, get()]; } "
                    "print(%s());" % (k, later, later, self.expr(scope, 1), k))
        if which == 1:   # closures in a loop: a fresh j per iteration, one shared i
            return ("let fs%s = []; let c%s = 0; while (c%s < 3) { let j = c%s * 2; push(fs%s, fn() { return [j, c%s]; }); "
                    "c%s = c%s + 1; } print(fs%s[0](), fs%s[2]());" % ((k,) * 10))
        if which == 2:   # break inside a called function stops the caller's loop
            return ("fn stop%s(n) { if (n > %d) { break; } return n; } let c%s = 0; "
                    "while (c%s < 6) { c%s = c%s + 1; stop%s(c%s); } print(c%s);" % (k, r.randint(0, 4), *(k,) * 7))
        if which == 3:   # unbounded recursion reaches the depth limit of 400
            return "fn deep%s(n) { return deep%s(n + 1); } try { deep%s(0); } catch (e) { print(e); }" % (k, k, k)
        if which == 4:   # counters: a captured local, two independent closures
            return ("fn mk%s() { let n = 0; return fn() { n = n + %s; return n; }; } let a%s = mk%s(); let b%s = mk%s(); "
                    "print(a%s(), a%s(), b%s());" % (k, r.choice(SMALL + FLOATS), *(k,) * 7))
        if which == 5:   # arithmetic past 64 bits and back
            return ("let b%s = %s; let c%s = 0; while (c%s < 4) { c%s = c%s + 1; b%s = b%s * %s - %s; "
                    "print(b%s, b%s %% 1000, b%s / 7, b%s > 1.5, -b%s); }"
                    % (k, r.choice(BIG + SMALL), k, k, k, k, k, k, r.choice(BIG + SMALL), r.choice(SMALL), *(k,) * 5))
        if which == 6:   # floats
            return ("let x%s = %s; let c%s = 0; while (c%s < 4) { c%s = c%s + 1; x%s = x%s * %s + %s; print(x%s, x%s / 3, x%s %% 2); }"
                    % (k, r.choice(FLOATS), k, k, k, k, k, k, r.choice(FLOATS + ["3", "-2"]), r.choice(FLOATS), k, k, k))
        if which == 7:   # strings built in a loop
            return ('let s%s = ""; let c%s = 0; while (c%s < 4) { c%s = c%s + 1; s%s = s%s + str(c%s * %s) + ","; } '
                    "print(s%s, len(s%s), s%s[1], s%s[-1]);" % (k, *(k,) * 7, r.choice(SMALL + FLOATS), k, k, k, k))
        if which == 8:   # throw unusual values, nested try
            return ("try { try { throw %s; } catch (e) { print(type(e), e); throw [e]; } } catch (e2) { print(e2); }"
                    % r.choice(["nil", "0", "[1, 2]", '"x"', "true", "fn() {}", BIG[0]]))
        if which == 9:   # recursion with a depth parameter
            return ("fn rec%s(d) { if (d < 1) { return 1; } return rec%s(d - 1) + rec%s(d - 2); } print(rec%s(%d));"
                    % (k, k, k, k, r.randint(0, 8)))
        return ('try { let v%s = num(%s); print(type(v%s), v%s, v%s + 1); } catch (e) { print(e); }'
                % (k, '"%s"' % r.choice(STRINGS), k, k, k))

    def program(self):
        scope = Scope()
        ctx = {"depth": 0, "loop": False, "func": False}
        lines = ["let _fuel = 150;"]
        for _ in range(self.r.randint(4, 14)):
            lines.append(self.stmt(scope, ctx, 0))
        ending = self.r.random()
        if ending < 0.1:
            lines.append("print(%s);" % self.r.choice(["undefined_name", "[1][5]", "1 / 0", "nil + 1", "len(3)"]))
        elif ending < 0.13:
            lines.append("return %s;" % self.expr(scope, 1))
        elif ending < 0.15:
            lines.append("break;")
        return "\n".join(lines) + "\n"


def generate(seed):
    return Gen(seed).program()


# --------------------------------------------------------------- runner ----


def program_files():
    paths = sorted(p for d in PROGRAM_DIRS for p in glob.glob(os.path.join(HERE, d, "*.ml")))
    for path in paths:
        with open(path, encoding="utf-8") as f:
            yield os.path.relpath(path, HERE), f.read().replace("clock()", "0")


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--count", type=int, default=1000)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--heap", default="default,small,stress")
    ap.add_argument("--modes", default="plain,opt")
    ap.add_argument("--save")
    ap.add_argument("--mlvm")
    ap.add_argument("--show", type=int)
    args = ap.parse_args(argv)
    sys.stdout.reconfigure(errors="backslashreplace")   # consoles that cannot show every character
    if args.show:
        sys.stdout.reconfigure(encoding="utf-8")
        for i in range(args.show):
            print("# seed %d" % (args.seed + i))
            print(generate(args.seed + i))
        return 0
    heaps = args.heap.split(",")
    modes = args.modes.split(",")
    engines = [("%s/%s" % (m, h), dict(HEAPS[h], optimize=(m == "opt"))) for m in modes for h in heaps]
    programs = list(program_files()) + [("seed %d" % (args.seed + i), generate(args.seed + i)) for i in range(args.count)]
    start = time.perf_counter()
    endings = collections.Counter()
    gc = collections.Counter()
    mismatches = 0
    lines = 0
    for name, src in programs:
        expected = outcome(minilang_c.TreeWalker, src)
        endings[expected[1][0] if expected[1][0] != "host" else "host " + expected[1][1]] += 1
        lines += len(expected[0])
        for label, options in engines:
            vm = None

            def make(out, options=options):
                nonlocal vm
                vm = minilang_c.CVM(out, mlvm=args.mlvm, **options)
                return vm

            try:
                got = outcome(make, src)
                gc[label] += vm.stats()["collections"]
                vm.close()
            except minilang_c.ServerError as err:   # the C VM crashed; the next run starts a new one
                got = ([], ("crash", str(err)))
            if got != expected:
                mismatches += 1
                print("MISMATCH %s on %s\n  tree-walker: %r\n  C VM:        %r" % (label, name, expected, got))
                if args.save:
                    os.makedirs(args.save, exist_ok=True)
                    with open(os.path.join(args.save, name.replace(" ", "_").replace("/", "_") + ".ml"), "w",
                              encoding="utf-8") as f:
                        f.write(src)
    elapsed = time.perf_counter() - start
    print("programs: %d (%d files, %d generated from seed %d)" % (
        len(programs), len(programs) - args.count, args.count, args.seed))
    print("engines per program: %d (%s); C runs: %d" % (len(engines), ", ".join(l for l, _ in engines),
                                                       len(programs) * len(engines)))
    print("tree-walker endings: %s; printed lines: %d" % (dict(sorted(endings.items())), lines))
    print("collections per engine: %s" % dict(gc))
    print("leaked sessions: %d" % len(minilang_c.LEAKS))
    print("mismatches: %d   (%.1f s)" % (mismatches, elapsed))
    return 1 if mismatches or minilang_c.LEAKS else 0


if __name__ == "__main__":
    sys.setrecursionlimit(20000)
    sys.exit(main(sys.argv[1:]))
