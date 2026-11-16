# minilang-spec: a specification and conformance suite

[Back to the overview](../../README.md) | [Roadmap (zh-TW)](../../research/03-design-your-own-dynamic-language.zh-TW.md) | [Stage 2: minilang](../minilang/README.md) | [Stage 4: minilang-vm](../minilang-vm/README.md) | [Stage 5: minilang-c](../minilang-c/README.md) | [Traditional Chinese README](README.zh-TW.md)

Runnable example for stage 8 of the research roadmap, specification and process. Three engines run minilang today, and stages 4 and 5 found that they agree partly by copying what the tree-walker happens to do. This folder writes the language down and checks the engines against the text:

- [SPEC.md](SPEC.md) ([zh-TW](SPEC.zh-TW.md)): the minilang 1.0 reference manual, with a decision for every implementation-defined or surprising behaviour (specified, implementation-defined, or a proposed change kept as legacy for now), two conformance levels, the reference engines' implementation-defined behaviour, nine proposed changes for the owner, and the versioning, change process and release gate.
- [tests/](tests/): 201 conformance tests, one directory per SPEC section 2 to 14, each a small program that states its expected output and ending and cites the sections it checks.
- `conformance.py`: runs every test on every engine and prints a matrix per engine and section. Python standard library only.
- `test_conformance.py`: tests of the suite itself, including deliberate deviations planted in the tree-walker that the suite must catch.

Nothing here changes an engine. Engines that fail a specified rule are reported below, not fixed. Verified on Windows 11 with Python 3.14.6 and WinLibs GCC 16.2.0.

## Running

```bash
cd dynamic-language-design-research/examples/minilang-spec
gcc -std=c11 -Wall -Wextra -Werror -O2 -o ../minilang-c/mlvm ../minilang-c/mlvm.c -lm   # the C VM, as in minilang-c (mlvm.exe on Windows; ignored by git)
python3 conformance.py                     # tree, vm and c through their Python APIs, about 3 s
python3 conformance.py --cli               # the same through their command lines, about 30 s
python3 conformance.py --all-configs       # adds vm --opt, c --opt and c --gc-stress
python3 conformance.py --level core        # Core level: core tests only, message texts not compared
python3 conformance.py --strict            # also fail when an engine stops matching its documented behaviour
python3 conformance.py --list              # every test with its section, level and status
python3 conformance.py --mlvm PATH         # a C VM built somewhere else (default: $MLVM, then ../minilang-c/mlvm)
python3 conformance.py --cmd "mine=python3 my_impl.py"   # any implementation that follows SPEC 15.3
python3 -m unittest test_conformance -v    # 14 tests of the suite itself, about 3 s
```

On Windows, `python3` may be `python`. The exit status is the release gate of SPEC Annex D: 0 only when every selected test passes on every engine.

## Test format

A test is a minilang program followed by a comment block that starts with a line `# ---`. The block is at the end so that line numbers in the expectation are the program's own:

```text
print("before");
print(
  1 +
  "a"
);

# ---
# spec: 10.4
# title: an operator error is reported at the operator's line
# end: runtime-error 3
# message: '+' expects two numbers, two strings or two lists, got number and string
#> before
```

| Key | Meaning |
| --- | --- |
| `spec` | The SPEC sections the test checks; the first one decides the directory and the matrix row |
| `level` | `core` (default) or `full`; SPEC 15.1 |
| `status` | `specified` (default), `legacy P3` (current behaviour kept until proposal P3 is decided), or `impl` (implementation-defined) |
| `end` | `ok`, `syntax-error N`, `runtime-error N`, `fatal`, or alternatives separated by `\|` (for `impl` tests, the permitted range) |
| `message` | The error message; compared at the Full level only |
| `#>` | One expected output line each; `output: unchecked` skips the comparison (some `impl` tests) |
| `doc-tree`, `doc-vm`, `doc-c` | For `impl` tests, the outcome each engine documents (SPEC Annex B); a difference is reported as documentation drift |

The runner gives each program to an engine whole, comments included. A run ends as `ok`, `syntax-error`, `runtime-error`, `fatal` (an error that `try` cannot catch, SPEC 10.7) or `crash` (the engine itself failed), and a crash never passes.

Two ways to run an engine:

- **Python API** (default): the constructor and `run()` contract that all three engines share (`minilang.Interpreter`, `minilang_vm.VM`, `minilang_c.CVM`), as their own test suites use it. The runner first raises Python's recursion limit to 20,000, as the engines' command lines do. For the C VM it keeps one `mlvm --serve` process and captures its standard error, so a crash is reported with the VM's own message.
- **Command line** (`--cli`): `python3 minilang.py FILE`, `minilang_vm.py`, `minilang_c.py`, classified by the contract in SPEC 15.3. A command line cannot tell a syntax error from a runtime error (both print `error: [line N] ...`), so either kind matches. The runner sets `PYTHONIOENCODING=utf-8` (finding 5 below).

## Results

`python3 conformance.py --all-configs`, 2026-11-16, Windows 11 Pro, Python 3.14.6, `mlvm` built with WinLibs GCC 16.2.0 `-O2`:

| SPEC section | Tests | tree | vm | vm --opt | c | c --opt | c --gc-stress |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 Lexical structure | 24 | 24 | 24 | 24 | 24 | 24 | 24 |
| 3 Syntax | 18 | 18 | 18 | 18 | 18 | 18 | 18 |
| 4 Values and types | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
| 5 Truthiness, equality, ordering | 13 | 13 | 13 | 13 | 13 | 13 | 13 |
| 6 Expressions | 20 | 20 | 20 | 20 | 20 | 20 | 20 |
| 7 Scoping and closures | 20 | 20 | 20 | 20 | 20 | 20 | 20 |
| 8 Control flow | 13 | 13 | 13 | 13 | 13 | 13 | 13 |
| 9 Functions and calls | 10 | 10 | 10 | 10 | 10 | 10 | 10 |
| 10 Errors and exceptions | 20 | 20 | 20 | 20 | 20 | 20 | 20 |
| 11 Standard functions | 20 | 20 | 20 | 20 | 20 | 20 | 20 |
| 12 Numbers | 17 | 17 | 17 | 17 | 17 | 17 | 17 |
| 13 Strings and text | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
| 14 Limits | 14 | 14 | 14 | 14 | **13** | **13** | **13** |
| **Core level** | 160 | 160 | 160 | 160 | **159** | **159** | **159** |
| **Full level** | 201 | 201 | 201 | 201 | **200** | **200** | **200** |

Of the 201 tests, 168 are specified, 23 legacy and 10 implementation-defined. `--cli --all-configs` gives the same matrix and the same failure. The Python engines take about 0.2 s each in-process; through the command lines each engine takes 5 to 14 s with 8 processes in parallel.

The implementation-defined tests, as printed by the runner (D numbers are SPEC Annex A):

| Test | tree | vm, vm --opt | c, c --opt, c --gc-stress |
| --- | --- | --- | --- |
| `02-lexical/unicode-identifier`, `unicode-whitespace`, `literal-unicode-decimal-digit` (D2, D3, D5) | ok | ok | ok |
| `02-lexical/literal-superscript-digit` (D5) | fatal ValueError | fatal ValueError | fatal ValueError |
| `02-lexical/literal-4301-digits` (D6) | fatal ValueError | fatal ValueError | fatal ValueError |
| `11-builtins/num-unicode-digits` (D34) | ok, prints `3 12` | ok, prints `3 12` | runtime-error 1 |
| `14-limits/cyclic-list-text` (D49) | fatal RecursionError | fatal RecursionError | fatal RecursionError |
| `14-limits/cyclic-list-text-in-function` (D49) | ok, prints `caught: stack overflow (host recursion limit)` | **fatal RecursionError** | ok, as tree |
| `14-limits/nesting-list-text-1100` (D48) | ok | ok | fatal RecursionError |
| `14-limits/nesting-functions-200` (D48) | ok | ok | **crash** |

## Findings

1. **C VM bug: 64 nested function literals crash the process.** `load_code()` in `mlvm.c` pushes one temporary root per level of function nesting, and the temporary root stack holds 64 (`TEMP_ROOTS`), while the loader's own limit, `MAX_CODE_NESTING` with the message `functions nested too deeply`, is 200 and is never reached. The VM stops with `mlvm: fatal: temp root stack overflow` and exit status 70; in serve mode every other session in the process is lost. 63 levels work. SPEC 14.3 requires a diagnosed error beyond an implementation's limit, so this fails `14-limits/nesting-functions-200` in every C configuration and is the only failure in the suite. Not fixed here.
2. **The engines disagree when a list that contains itself is printed inside a function.** The tree-walker converts the host `RecursionError` into the catchable `stack overflow (host recursion limit)` at the innermost call, and the C VM copies that; the Python VM has no host call to convert at, so the error escapes as a fatal `RecursionError`. minilang-vm's README says that message can only come from extremely deeply nested expressions; printing a cyclic or very deep list is a second, ordinary way to reach it. The SPEC makes it implementation-defined (D49), and proposal P5 would print `[...]` instead.
3. **The shared front end crashes on some digits.** `²` (and other characters for which Python's `str.isdigit()` is true but `int()` fails) in a number literal stops the lexer with a Python `ValueError`, and so does an integer literal of 4,301 digits. All three engines share the lexer, so all three do it; proposal P7 would make both syntax errors or ordinary literals.
4. **Integer division is not exact for whole quotients.** `9007199254740993 / 1` is `9007199254740992`, because the quotient is rounded to a float before it is turned back into an integer. All three engines agree (the C VM ports CPython's rounding), so it is not a bug against the engines; it is kept as legacy and proposal P9 would make whole quotients exact. It was found while writing SPEC 12.3, not by any existing test.
5. **The Python command lines cannot print non-ASCII text on this machine.** With the locale's code page cp950 and standard output a pipe, `minilang.py` and `minilang_vm.py` fail with `UnicodeEncodeError` on the first `é`; `minilang_c.py` works because `mlvm` writes UTF-8 bytes. SPEC 15.3 makes UTF-8 part of the command-line contract, and the runner sets `PYTHONIOENCODING=utf-8`.
6. **Parsing runs before the recursion limit is raised.** `Interpreter.run()` and `VM.run()` raise Python's recursion limit only after parsing, so through the Python API with the default limit of 1,000, 70 nested parentheses already fail with a fatal `RecursionError` (60 work); the command lines work because they raise the limit to 20,000 first. The runner does the same, and SPEC 14.3 sets minimum nesting depths.
7. **Three engines, fewer independent implementations.** The lexer and parser exist once, and the Python VM reuses the tree-walker's standard functions, text formatting and operator slow paths. Agreement on SPEC sections 2 and 3 is one implementation agreeing with itself; only the C VM independently implements operators, numbers and text. SPEC Annex D.2 records this against the roadmap's TC39 criterion of two independent implementations.

## What the specification decides

SPEC Annex A lists 50 decisions. In short: lexical rules, values, equality, scoping by resolution at use, closures, error lines, the standard functions and Python's number semantics are **specified** as the engines behave. Unicode in identifiers and white space, `num()` of non-ASCII digits, nesting limits, the text of a cyclic list, `clock()`, fatal-error reports and output encoding are **implementation-defined**, with the reference engines documented in Annex B. Nine behaviours are **proposed changes** and stay as legacy (required at the Full level, open at Core) until the owner decides:

| # | Proposal | Replaces today's |
| --- | --- | --- |
| P1 | `throw nil` binds `nil` | caught as the string `"nil"` |
| P2 | static block scoping | a later `let` changes what a closure's name refers to |
| P3 | `break` only inside a loop of the same function, else a syntax error | `break` crosses function calls; stray `break` is fatal |
| P4 | `return` outside a function is a syntax error | fatal at run time |
| P5 | no fatal errors from ordinary operations; a cyclic list prints `[...]` | host `OverflowError`, `ValueError`, `RecursionError` escape |
| P6 | `num()` without Python leftovers | `num() cannot convert None`, `num(true)` is 1 |
| P7 | ASCII digits in literals; the front end never crashes | finding 3 |
| P8 | call depth at least 400 instead of exactly 400 | exactly 400 |
| P9 | exact integer quotients | finding 4 |

## How the suite is checked

- **Planted deviations** (`test_conformance.py`): the tree-walker with one rule changed must fail the tests for that rule. Eight variants are checked: call depth 399, `throw nil` binding `nil`, truncated `%`, booleans equal to numbers, six-digit float text, `break` stopped at the function boundary, a reworded division-by-zero message, and every runtime error reported at line 1. Each is caught. The reworded message and the lexical `break` pass the Core level and fail the Full level only, which is what the levels are for. A first version of the suite hung on the lexical `break` variant because a test looped on `while (true)`; every loop in the tests is now bounded so that a wrong engine fails instead of hanging.
- **Section references**: every section a test cites exists as a heading in both SPEC files.
- **Two harnesses**: the Python API and the command lines give the same matrix, so the classification does not depend on how an engine is driven.

## Limits and what is not verified

- One machine and one toolchain: Windows 11, Python 3.14.6, WinLibs GCC 16.2.0. Linux and other compilers were not run.
- The acceptance criterion of roadmap stage 8, that someone else can implement a subset from the manual and pass the suite, is not demonstrated: no implementation written from SPEC.md exists yet. The Core level is designed so that one does not need Python's number formatting, but that is untested.
- The tests sample each rule; they do not enumerate every message form (for example every `expected X but found K`), every Unicode category, or the REPL. The wording of fatal-error reports is deliberately not tested.
- The C adapter drives `minilang_c.CVM` and reads two internal names of `minilang_c` to capture crash messages; if stage 6 refactors that module, the runner falls back to running without capture, and the command-line mode does not depend on them.
- `clock()` is checked only for being a positive number that does not go backwards between two calls.

## Next steps

1. The owner decides P1 to P9; accepted proposals go through SPEC Annex D.2 and make minilang 2.0, with an MLBC major version where a VM instruction changes meaning.
2. Fix the C VM's nesting crash (finding 1) so the release gate passes; then make `--strict` part of the gate.
3. Write a Core implementation from SPEC.md alone, without reading the engines, to test whether the manual is enough.

Detailed decisions are in the [roadmap](../../research/03-design-your-own-dynamic-language.zh-TW.md) (Traditional Chinese).
