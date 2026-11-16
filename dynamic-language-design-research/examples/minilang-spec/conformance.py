#!/usr/bin/env python3
"""minilang conformance runner: every test in tests/ on every engine.

Each test is a minilang program whose expected result is written in a comment
block at the end of the file (after a line that is exactly "# ---"; see
README.md). The runner runs each test on each engine, compares the printed
output and the way the run ended with the expectation, and prints a Markdown
matrix of passes per engine and per SPEC.md section, the failures, and the
outcomes of the implementation-defined tests next to what each engine
documents. Python standard library only.

Usage:
    python3 conformance.py                      # tree, vm and c through their Python APIs
    python3 conformance.py --cli                # the same engines through their command lines
    python3 conformance.py --all-configs        # also vm --opt, c --opt and c --gc-stress
    python3 conformance.py --level core         # Core level only: core tests, messages not compared
    python3 conformance.py --cmd "mine=python3 my_impl.py"   # any engine with the command-line contract
    python3 conformance.py --mlvm PATH          # the C VM binary (default: $MLVM, else minilang-c/mlvm[.exe])
    python3 conformance.py --list               # list the tests by section
Exit status 0 when every test at the chosen level passes on every engine
(the release gate in SPEC.md, Annex D), 1 otherwise; with --strict a
difference from an engine's documented implementation-defined behaviour also
fails.
"""
import argparse
import concurrent.futures
import os
import re
import shlex
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
EXAMPLES = os.path.dirname(HERE)
TESTS = os.path.join(HERE, "tests")
LANGUAGE_VERSION = "1.0"
EXE = ".exe" if os.name == "nt" else ""

SECTIONS = {
    2: "Lexical structure", 3: "Syntax", 4: "Values and types", 5: "Truthiness, equality, ordering",
    6: "Expressions", 7: "Scoping and closures", 8: "Control flow", 9: "Functions and calls",
    10: "Errors and exceptions", 11: "Standard functions", 12: "Numbers", 13: "Strings and text",
    14: "Limits",
}
ERROR_KINDS = ("syntax-error", "runtime-error", "error")
END_KINDS = ("ok", "fatal") + ERROR_KINDS


# ---------------------------------------------------------------- tests ----


class Expect:
    """One permitted ending: a kind and, for errors, an optional line."""

    def __init__(self, text):
        parts = text.split()
        if not parts or parts[0] not in END_KINDS or len(parts) > 2:
            raise ValueError("bad end: %r" % text)
        self.kind = parts[0]
        self.line = int(parts[1]) if len(parts) == 2 else None
        if self.line is not None and self.kind not in ERROR_KINDS:
            raise ValueError("only errors take a line: %r" % text)

    def matches(self, outcome):
        if self.kind in ("ok", "fatal"):
            return outcome.kind == self.kind
        if outcome.kind == "error":          # a command line cannot tell syntax from runtime errors
            kind_ok = True
        elif self.kind == "error":
            kind_ok = outcome.kind in ("syntax-error", "runtime-error")
        else:
            kind_ok = outcome.kind == self.kind
        return kind_ok and (self.line is None or self.line == outcome.line)

    def __str__(self):
        return self.kind if self.line is None else "%s %d" % (self.kind, self.line)


def unescape(text):
    return re.sub(r"\\(.)", lambda m: {"n": "\n", "t": "\t", "\\": "\\"}[m.group(1)], text)


class Test:
    def __init__(self, path):
        self.path = path
        self.id = os.path.relpath(path, TESTS).replace(os.sep, "/")[:-3]
        with open(path, encoding="utf-8") as f:
            self.source = f.read()
        lines = self.source.split("\n")
        try:
            start = len(lines) - 1 - lines[::-1].index("# ---")
        except ValueError:
            raise ValueError("%s: no '# ---' line before the expectations" % self.id) from None
        meta, out = {}, []
        for raw in lines[start + 1:]:
            if raw == "#>" or raw.startswith("#> "):
                out.append(raw[3:])
            elif raw.startswith("# ") and ":" in raw:
                key, _, value = raw[2:].partition(":")
                meta.setdefault(key.strip(), []).append(value.strip())
            elif raw.strip():
                raise ValueError("%s: cannot read expectation line %r" % (self.id, raw))
        self.spec = meta.get("spec", [""])[0].split()
        if not self.spec:
            raise ValueError("%s: no spec section" % self.id)
        self.section = int(self.spec[0].split(".")[0])
        self.title = meta.get("title", [""])[0]
        self.level = meta.get("level", ["core"])[0]
        status = meta.get("status", ["specified"])[0].split()
        self.status = status[0]
        self.proposal = status[1] if len(status) > 1 else ""
        self.ends = [Expect(alt) for alt in meta.get("end", ["ok"])[0].split("|")]
        self.message = unescape(meta["message"][0]) if "message" in meta else None
        self.check_output = meta.get("output", ["checked"])[0] != "unchecked"
        self.output = "".join(line + "\n" for line in out)
        self.docs = {k[4:]: v[0] for k, v in meta.items() if k.startswith("doc-")}
        if self.level not in ("core", "full") or self.status not in ("specified", "legacy", "impl"):
            raise ValueError("%s: bad level or status" % self.id)
        if self.status == "legacy" and self.level != "full":
            raise ValueError("%s: legacy behaviour is a Full-level requirement" % self.id)


def load_tests():
    tests = []
    for root, _, files in os.walk(TESTS):
        for name in sorted(files):
            if name.endswith(".ml"):
                tests.append(Test(os.path.join(root, name)))
    tests.sort(key=lambda t: (t.section, t.id))
    return tests


# ------------------------------------------------------------- outcomes ----


class Outcome:
    """How one run ended. kind: ok, syntax-error, runtime-error, error (a
    command line's error report, syntax or runtime), fatal (ended by an error
    that try cannot catch), crash (the engine itself failed)."""

    def __init__(self, kind, output="", line=None, message=None, detail=""):
        self.kind, self.output, self.line, self.message, self.detail = kind, output, line, message, detail

    def summary(self):
        if self.kind in ERROR_KINDS:
            return "%s %s" % (self.kind, self.line)
        if self.kind in ("fatal", "crash"):
            return ("%s %s" % (self.kind, self.detail)).strip()
        return self.kind


def first_word_error(text):
    """The host error type named in a fatal error's report, if any."""
    names = re.findall(r"\b([A-Z]\w*(?:Error|Signal))\b", text)
    return names[-1] if names else (text.strip().splitlines() or [""])[-1][:80]


# ------------------------------------------------------- API engines ----


def _import_engines():
    for sub in ("minilang", "minilang-vm", "minilang-c"):
        path = os.path.join(EXAMPLES, sub)
        if path not in sys.path:
            sys.path.insert(0, path)
    import minilang
    import minilang_vm
    import minilang_c
    return minilang, minilang_vm, minilang_c


class ApiEngine:
    """An engine run in this process through the constructor and run()
    contract of minilang.Interpreter, which all three engines share."""

    def __init__(self, name, mlvm=None):
        self.name = name
        self.minilang, self.minilang_vm, self.minilang_c = _import_engines()
        # The engines' command lines raise Python's recursion limit to 20000
        # before parsing; run() raises it only after parsing, so without this
        # 70 nested parentheses already fail in the parser (see README).
        sys.setrecursionlimit(max(sys.getrecursionlimit(), 20000))
        self.mlvm = mlvm
        self.note = ""
        if name.startswith("c"):
            self.mlvm = mlvm or os.environ.get("MLVM") or os.path.join(EXAMPLES, "minilang-c", "mlvm" + EXE)
            if not os.path.exists(self.mlvm):
                raise FileNotFoundError("C VM binary not found at %s (build it, then pass --mlvm)" % self.mlvm)
            self.note = "mlvm %s" % self.mlvm

    def _server(self):
        """Keep a serve process whose stderr is captured, so a crash can be
        reported with the VM's own message instead of printing over the report."""
        mc = self.minilang_c
        server = mc._SERVERS.get(self.mlvm)
        if server is None or not server.alive():
            server = mc.Server.__new__(mc.Server)
            server.mlvm = self.mlvm
            server.errfile = tempfile.TemporaryFile()
            server.proc = subprocess.Popen([self.mlvm, "--serve"], stdin=subprocess.PIPE,
                                           stdout=subprocess.PIPE, stderr=server.errfile)
            mc._SERVERS[self.mlvm] = server
        return server

    def make(self, out):
        n = self.name
        if n == "tree":
            return self.minilang.Interpreter(output=out)
        if n in ("vm", "vm-opt"):
            return self.minilang_vm.VM(output=out, optimize=n == "vm-opt")
        try:
            self._server()
        except AttributeError:   # minilang_c's internals changed: run uncaptured, crashes print to stderr
            pass
        return self.minilang_c.CVM(output=out, optimize=n == "c-opt", gc_stress=n == "c-stress", mlvm=self.mlvm)

    def run(self, test):
        ml = self.minilang
        out = []
        engine = None
        try:
            engine = self.make(out.append)
            engine.run(test.source)
            result = Outcome("ok")
        except ml.SyntaxErr as err:
            result = Outcome("syntax-error", line=err.line, message=err.message)
        except ml.RuntimeErr as err:
            result = Outcome("runtime-error", line=err.line, message=err.message)
        except self.minilang_c.ServerError as err:
            if str(err).startswith("load error"):   # the C loader rejected the file with a diagnosis
                result = Outcome("fatal", detail="LoadError", message=str(err))
            else:
                result = Outcome("crash", detail=self._crash_text(err))
        except Exception as err:   # noqa: BLE001  a host exception that minilang code cannot catch
            result = Outcome("fatal", detail=type(err).__name__, message=str(err)[:200])
        finally:
            if engine is not None and hasattr(engine, "close"):
                try:
                    engine.close()
                except Exception:   # noqa: BLE001  the server may be gone after a crash
                    pass
        result.output = "".join(s + "\n" for s in out)
        return result

    def _crash_text(self, err):
        server = self.minilang_c._SERVERS.get(self.mlvm)
        errfile = getattr(server, "errfile", None)
        if errfile is not None:
            errfile.seek(0)
            lines = errfile.read().decode("utf-8", "replace").strip().splitlines()
            if lines:
                return "%s; %s" % (err, lines[-1])
        return str(err)

    def leaks(self):
        return list(self.minilang_c.LEAKS) if self.name.startswith("c") else []


# --------------------------------------------------- command-line engines ----


class CliEngine:
    """An engine run as `COMMAND file.ml`, with the command-line contract of
    SPEC.md section 15.3: printed lines on stdout in UTF-8, exit status 0 when
    the program completes, 1 when it ends with an error, the report of an
    uncaught syntax or runtime error as the stderr text "error: [line N] message",
    any other exit status an implementation failure."""

    def __init__(self, name, command, env=None, timeout=120):
        self.name, self.command, self.env, self.timeout = name, command, env, timeout
        self.note = " ".join(command)

    def run(self, test):
        try:
            proc = subprocess.run(self.command + [test.path], capture_output=True, env=self.env, timeout=self.timeout)
        except subprocess.TimeoutExpired:
            return Outcome("crash", detail="timeout after %d s" % self.timeout)
        out = proc.stdout.decode("utf-8", "replace").replace("\r\n", "\n")
        err = proc.stderr.decode("utf-8", "replace").replace("\r\n", "\n")
        if proc.returncode == 0:
            return Outcome("ok", out)
        if proc.returncode == 1:
            m = re.match(r"error: \[line (\d+)\] (.*)\Z", err[:-1] if err.endswith("\n") else err, re.S)
            if m:
                return Outcome("error", out, int(m.group(1)), m.group(2))
            return Outcome("fatal", out, detail=first_word_error(err), message=err.strip()[-200:])
        last = (err.strip().splitlines() or [""])[-1]
        return Outcome("crash", out, detail="exit status %d; %s" % (proc.returncode, last[:120]))

    def leaks(self):
        return []


def cli_engine(name, mlvm):
    env = dict(os.environ, PYTHONIOENCODING="utf-8")   # the contract's UTF-8 output; see README
    if mlvm:
        env["MLVM"] = mlvm
    py = [sys.executable]
    commands = {
        "tree": py + [os.path.join(EXAMPLES, "minilang", "minilang.py")],
        "vm": py + [os.path.join(EXAMPLES, "minilang-vm", "minilang_vm.py")],
        "vm-opt": py + [os.path.join(EXAMPLES, "minilang-vm", "minilang_vm.py"), "--opt"],
        "c": py + [os.path.join(EXAMPLES, "minilang-c", "minilang_c.py")],
        "c-opt": py + [os.path.join(EXAMPLES, "minilang-c", "minilang_c.py"), "--opt"],
        "c-stress": py + [os.path.join(EXAMPLES, "minilang-c", "minilang_c.py"), "--gc-stress"],
    }
    return CliEngine(name, commands[name], env)


# --------------------------------------------------------------- checks ----


def check(test, outcome, level):
    """The reasons a result fails the test at `level` ("core" or "full"); empty if it passes."""
    if outcome.kind == "crash":
        return ["engine crashed: %s" % outcome.detail]
    problems = []
    if test.check_output and outcome.output != test.output:
        problems.append("output %r, expected %r" % (outcome.output[-300:], test.output[-300:]))
    matched = next((e for e in test.ends if e.matches(outcome)), None)
    if matched is None:
        problems.append("ended %s, expected %s" % (outcome.summary(), " | ".join(map(str, test.ends))))
    elif (level == "full" and test.message is not None and matched.kind in ERROR_KINDS
          and outcome.message != test.message):
        problems.append("message %r, expected %r" % (outcome.message, test.message))
    return problems


def doc_matches(doc, outcome):
    actual = outcome.summary()
    if outcome.kind == "error":   # command line: compare the line only
        actual_words, doc_words = actual.split(), doc.split()
        return len(doc_words) == 2 and doc_words[0] in ERROR_KINDS and doc_words[1] == actual_words[1]
    return actual == doc or actual.startswith(doc + " ")


# --------------------------------------------------------------- report ----


def table(header, rows):
    lines = ["| " + " | ".join(header) + " |", "|" + " --- |" * len(header)]
    lines += ["| " + " | ".join(str(c) for c in row) + " |" for row in rows]
    return "\n".join(lines)


def run_all(tests, engines, jobs, verbose):
    results = {}
    for engine in engines:
        start = time.perf_counter()
        if isinstance(engine, CliEngine) and jobs > 1:
            with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
                outcomes = list(pool.map(engine.run, tests))
        else:
            outcomes = [engine.run(t) for t in tests]
        for test, outcome in zip(tests, outcomes):
            results[engine.name, test.id] = outcome
            if verbose:
                print("%-8s %-50s %s" % (engine.name, test.id, outcome.summary()))
        engine.seconds = time.perf_counter() - start
    return results


def report(tests, engines, results, level):
    names = [e.name for e in engines]
    selected = [t for t in tests if level == "full" or t.level == "core"]
    print("minilang %s conformance suite: %d tests (%d Core, %d Full-only); level %s; Python %s" % (
        LANGUAGE_VERSION, len(selected), sum(t.level == "core" for t in selected),
        sum(t.level == "full" for t in selected), level, sys.version.split()[0]))
    for e in engines:
        print("  %-8s %s (%.1f s)" % (e.name, e.note or type(e).__name__, e.seconds))
    print()
    fails = {}
    for t in selected:
        for n in names:
            problems = check(t, results[n, t.id], level)
            if problems:
                fails[n, t.id] = problems
    rows = []
    for sec, title in SECTIONS.items():
        group = [t for t in selected if t.section == sec]
        if group:
            rows.append(["%d %s" % (sec, title), len(group)] +
                        ["%d/%d" % (sum((n, t.id) not in fails for t in group), len(group)) for n in names])
    core = [t for t in tests if t.level == "core"]
    rows.append(["**Core level** (messages not compared)", len(core)] +
                ["%d/%d" % (sum(not check(t, results[n, t.id], "core") for t in core), len(core)) for n in names])
    if level == "full":
        rows.append(["**Full level**", len(selected)] +
                    ["%d/%d" % (sum((n, t.id) not in fails for t in selected), len(selected)) for n in names])
    for status in ("specified", "legacy", "impl"):
        group = [t for t in selected if t.status == status]
        rows.append(["of which %s" % status, len(group)] +
                    ["%d/%d" % (sum((n, t.id) not in fails for t in group), len(group)) for n in names])
    print(table(["Section", "Tests"] + names, rows))
    print()
    if fails:
        print("Failures:")
        print()
        by_id = {t.id: t for t in tests}
        for (n, tid), problems in sorted(fails.items(), key=lambda kv: (by_id[kv[0][1]].section, kv[0][1], kv[0][0])):
            t = by_id[tid]
            print("- %s on %s (spec %s, %s): %s" % (tid, n, " ".join(t.spec), t.status, "; ".join(problems)))
        print()
    else:
        print("Failures: none")
        print()
    impl = [t for t in selected if t.status == "impl"]
    drift = []
    if impl:
        print("Implementation-defined tests: outcome on each engine (documented outcome in brackets when it differs):")
        print()
        rows = []
        for t in impl:
            row = [t.id]
            for n in names:
                outcome = results[n, t.id]
                doc = t.docs.get(n.split("-")[0])
                cell = outcome.summary()
                if outcome.output and not t.check_output:
                    cell += ", prints %r" % outcome.output.splitlines()[0][:40]
                if doc is not None and not doc_matches(doc, outcome):
                    cell += " [documented: %s]" % doc
                    drift.append((n, t.id, doc, outcome.summary()))
                row.append(cell)
            rows.append(row)
        print(table(["Test"] + names, rows))
        print()
        print("Documentation drift: %s" % ("none" if not drift else "%d (marked above)" % len(drift)))
        print()
    leaks = [(e.name, leak) for e in engines for leak in e.leaks()]
    if leaks:
        print("C VM sessions that did not free every byte: %s" % leaks)
        print()
    return fails, drift


# ----------------------------------------------------------------- main ----


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--engines", default="tree,vm,c", help="comma-separated: tree, vm, vm-opt, c, c-opt, c-stress")
    ap.add_argument("--all-configs", action="store_true", help="tree, vm, vm-opt, c, c-opt, c-stress")
    ap.add_argument("--cli", action="store_true", help="run the engines through their command lines")
    ap.add_argument("--cmd", action="append", default=[], metavar="NAME=COMMAND",
                    help="add an engine run as COMMAND file.ml (command-line contract)")
    ap.add_argument("--mlvm", help="path of the C VM binary")
    ap.add_argument("--level", choices=("core", "full"), default="full")
    ap.add_argument("--section", type=int, help="only this section")
    ap.add_argument("--test", help="only tests whose id contains this text")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4, help="parallel processes for --cli")
    ap.add_argument("--strict", action="store_true", help="also fail on documentation drift")
    ap.add_argument("--list", action="store_true", help="list the tests and exit")
    ap.add_argument("-v", "--verbose", action="store_true", help="print every result")
    args = ap.parse_args(argv)
    sys.setrecursionlimit(20000)   # as the engines' own command lines do

    tests = load_tests()
    if args.section is not None:
        tests = [t for t in tests if t.section == args.section]
    if args.test:
        tests = [t for t in tests if args.test in t.id]
    if args.list:
        for t in tests:
            print("%-4s %-46s %-5s %-10s %s" % (" ".join(t.spec), t.id, t.level, (t.status + " " + t.proposal).strip(), t.title))
        return 0

    names = "tree,vm,vm-opt,c,c-opt,c-stress" if args.all_configs else args.engines
    mlvm = args.mlvm and os.path.abspath(args.mlvm)
    engines = []
    for name in [n for n in names.split(",") if n]:
        try:
            engines.append(cli_engine(name, mlvm) if args.cli else ApiEngine(name, mlvm))
        except (FileNotFoundError, KeyError) as err:
            print("skipping engine %s: %s" % (name, err))
    for spec in args.cmd:
        name, _, command = spec.partition("=")
        engines.append(CliEngine(name, shlex.split(command, posix=os.name != "nt")))
    if not engines:
        print("no engine to run")
        return 2
    results = run_all(tests, engines, args.jobs, args.verbose)
    fails, drift = report(tests, engines, results, args.level)
    return 1 if fails or (args.strict and drift) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
