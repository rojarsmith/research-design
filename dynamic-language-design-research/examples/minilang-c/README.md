# minilang-c: a C VM with a mark-sweep garbage collector

[Back to the overview](../../README.md) | [Roadmap (zh-TW)](../../research/03-design-your-own-dynamic-language.zh-TW.md) | [Stage 4: minilang-vm](../minilang-vm/README.md) | [Stage 2: minilang](../minilang/README.md) | [Traditional Chinese README](README.zh-TW.md)

Runnable example for stage 5 of the research roadmap, memory management, and the C measurement that stage 4 left open. The Python front end from [minilang-vm](../minilang-vm/README.md) compiles a program and writes a bytecode file; `mlvm.c` loads that file, verifies it, runs it and manages its memory with a precise mark-sweep collector. `mlvm.c` is one C11 file with no dependencies beyond the C library; the tools around it use only the Python standard library. It shows the parts of stage 5 and how to accept them; it is not a production VM.

Verified on Windows 11 with WinLibs GCC 16.2.0 and Python 3.14.6, and on Ubuntu 24.04 (podman) with GCC 13.3.0, Python 3.12.3, AddressSanitizer and UndefinedBehaviorSanitizer.

## Running

```bash
cd dynamic-language-design-research/examples/minilang-c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o mlvm mlvm.c -lm          # the only build step (mlvm.exe on Windows)
gcc -std=c11 -Wall -Wextra -Werror -O0 -o mlvm-O0 mlvm.c -lm       # optional: the -O0 column of --bench
python3 -m unittest test_minilang_c -v                             # 94 tests, about 40 s
python3 difftest.py                                                # program files + 1,000 random programs, 6 VM configurations, about 1 min
python3 minilang_c.py ../minilang/programs/closures.ml             # compile in Python, run on the C VM
python3 ../minilang-vm/minilang_vm.py --emit fib.mlc ../minilang/programs/fib.ml
./mlvm fib.mlc                                                     # run a bytecode file without Python
./mlvm --stats --gc-stress fib.mlc                                 # collect on every allocation; statistics on stderr
./mlvm --dis fib.mlc                                               # the same listing as minilang_vm.py --dis
python3 minilang_c.py --bench                                      # tree-walker, Python VM and C VM, 5 runs each, about 2 min
```

On Windows, `python3` may be `python`. The only things to install are GCC (WinLibs) and the Python used for minilang.

The 94 tests: minilang's 13 tests on the C VM (plain and with compiler optimizations; the 10 semantics tests also under `--gc-stress`), 5 checks that those runs really use the C VM, stage 4's 24 differential tests with each program run on four C VM configurations (plain, `--opt`, `--gc-stress`, both), one test that runs every program file, and 28 tests of the file format, the loader, numbers and memory.

## Pipeline

```text
source -> minilang parser (stage 2) -> minilang_vm Resolver and Compiler (stage 4) -> Code objects
       -> minilang_vm.dump_program(): bytecode file, format 1.0          python3 minilang_vm.py --emit
       -> loader: header, bounds-checked reads, UTF-8 check, interned names
       -> verifier: operands, jump targets, tables, stack depth on every path
       -> interpreter: one switch loop; all frames share one value stack
       -> heap: every object on one list, freed only by the mark-sweep collector   mlvm.c
```

## Why the front end stays in Python

| Option | Cost | Decision |
| --- | --- | --- |
| Python front end, C loads a bytecode file | Needs Python to compile; syntax errors come from Python | Chosen |
| A second front end in C | Re-implementing the stage 4 Resolver's scoping rules (chains, cells, upvalue threading) in C, then testing two compilers and a VM at once | Not done |

One front end means one set of scoping decisions, so a difference between engines is a VM or GC bug, not a compiler disagreement. It also verifies a motivation stage 4 could not: compiled code that is cached and run without the compiler, as Lua allows builds that carry only the loader. The price is that running a source file needs Python; a standalone distribution (stage 7) would need a C front end.

## Bytecode file format 1.0

Written by `minilang_vm.dump_program()` (`--emit`), read by `minilang_vm.load_program()` and by `mlvm.c`. All integers little-endian.

```text
file   := "MLBC" major:u16 minor:u16 opcode_count:u16 flags:u16 code
code   := name:str nparams:u32 param:str*  nslots:u32 max_stack:u32
          ninstr:u32 (opcode:u8 operand:u32)*        -- table offsets count 2 per instruction, as in Code.code
          nconsts:u32 const*
          nlines:u32 (offset:u32 line:u32)*           -- line table runs
          nhandlers:u32 (start:u32 end:u32 target:u32)*
          nloops:u32 (start:u32 end:u32)*
          nchains:u32 (name:str nmaybe:u32 upvalue:u32* kind:u8 final:u32)*
          nupvals:u32 (from_slot:u8 index:u32 name:str)*
          slot_name:str * nslots
const  := tag:u8, then 0 nil | 1 false | 2 true | 3 int:i64 | 4 int outside i64:str (decimal)
          | 5 float:f64 | 6 string:str | 7 code (nested)
str    := nbytes:u32 UTF-8 bytes
```

- **Versions.** A reader rejects another major version, a newer minor version, or a different opcode count; renumbering opcodes is a major change. `flags` bit 0 records that an optimization was on and is informational.
- **Debug names are kept** (parameters, slots, upvalues), so `mlvm --dis` prints exactly what `minilang_vm.py --dis` prints. That equality is the loader's main test: a file that disassembles identically was read back field by field.
- **The loader verifies before running.** Every read is bounds-checked and every count must fit in the remaining bytes, strings must be valid UTF-8, operands must be in range for their opcode (constant of the right type, slot, upvalue, chain, jump target on an instruction boundary), tables must be in order, and a stack-depth walk over every path, including exception handlers (depth 1) and loop exits (depth 0), rejects underflow, inconsistent depths, code that runs off the end, and a `max_stack` smaller than computed. What it cannot check statically, that a slot holds a cell when `LOAD_CELL` runs, is checked at run time.

## Design at a glance

| Decision | Choice in this example | Roadmap discussion |
| --- | --- | --- |
| Value representation | 16-byte tagged union: nil, bool, int64, double and builtins inline; everything else a pointer to a heap object | "Tagged union; decide between NaN-boxing and a struct": a struct, for clarity |
| Heap objects | string, integer outside 64 bits, list, cell, closure, code object; one header with kind, mark bit and size | Lua and clox use the same shape of object list |
| Integers | int64 fast path, arbitrary precision (32-bit limbs) on overflow; results that fit go back to int64 | Forced by the tree-walker, whose integers are Python's |
| Strings | Immutable UTF-8 with a cached code point count, since `len()` and indexing count code points | Same reason |
| Interning | Constants, global names and builtin names are interned in a weak table; strings built at run time are not | Globals compare keys by pointer |
| Closures | The stage 4 bytecode boxes a captured variable in a cell when its scope is entered | No open upvalues exist, see "Roots" |
| Frames | One value stack for all frames; arguments become the callee's first slots in place; 400 frames at most | Stage 4's VM copies arguments into a new list per call |
| Dispatch | `switch` in a `for` loop | Computed goto is a GCC extension, which PEP 7 rules out; Lua also uses a switch for portability |
| Errors | C functions return a status and record the error; the loop unwinds with the exception tables | Same tables as the Python VM |
| Platform | Timing and binary stdio are the only platform-specific code, in one section | Roadmap stage 7: one platform layer |

## Memory management

### Why mark-sweep

| Scheme | What it would mean here | Who uses it |
| --- | --- | --- |
| Reference counting | Every closure that calls itself, every list inside itself, would leak, because they are cycles (see below); a cycle collector would be needed on top | CPython (plus a cycle collector), QuickJS (plus cycle removal) |
| Mark-sweep | Frees cycles; objects never move, so C code can hold plain pointers to rooted objects; needs a precise root set and stops the program while it runs | Lua, clox (roadmap stage 5 table) |
| Copying or generational | Shorter pauses and no fragmentation, but objects move, so every pointer held by C code must be a handle that the collector can update | V8 (handles, `HandleScope`), PyPy incminimark |

minilang makes the argument concrete: a local function that calls itself is always a cycle. `fn countdown(k) { ... countdown(k - 1); }` inside another function compiles to a cell holding the closure, and the closure's upvalue is that cell. [`programs/gc_cycles.ml`](programs/gc_cycles.ml) creates 20,000 such cycles plus 20,000 self-containing lists; the collector frees them and the test checks that the heap stays under 512 KB. Reference counting alone would keep all of them. CPython pays for a separate cycle collector for the same reason, and PEP 703 shows what reference counts cost once threads share objects; minilang is single-threaded and has no finalizers, so the deterministic timing that reference counting buys is not needed here. A moving collector would need V8-style handles throughout the C code and, later, in the stage 6 C API. Mark-sweep is the smallest design that is correct for this language: the collector itself is about 160 lines.

### Roots

The roadmap suggested "VM stack, global table and open upvalues". Building it showed what the list has to be:

| Root | Why |
| --- | --- |
| The value stack up to `vm->sp` | Every frame's slots and operands. The loop keeps `sp` in a local variable and publishes it (`SYNC()`) before anything that can allocate; the collector marks exactly `stack[0 .. vm->sp)` |
| Each frame's closure and code object | The script's code object is reachable only from its frame. A frame's closure is also on the stack, just below the arguments, so this root is redundant today (a planted bug that removed it went unnoticed, see below); it stays so that a later change such as tail calls cannot break it |
| The global table, keys and values | Global variables and the builtins |
| A temporary root stack | C code that allocates twice before the first object is reachable: a list and then its item array, the loader filling a code object, an integer constant parsed from its text |
| The value being thrown, or a top-level `return` value | Between the throw and the handler that catches it |
| Not a root: the intern table | It is weak: before the sweep, entries whose strings are unmarked are removed |
| Not a root: open upvalues | There are none. The stage 4 bytecode boxes a captured variable in a cell when its scope is entered (CPython's design), so a captured variable costs one allocation per scope entry, and no upvalue ever points into the stack. Lua's and clox's open upvalues avoid that allocation but need a "close" instruction at scope exit, which this shared bytecode does not have |

### Policy, statistics and stress mode

- **Growth policy.** Collect when the heap reaches `max(2 x what survived the last collection, 1 MiB)`; the minimum is adjustable with `--gc-min` (the tests use 4 KiB to force many collections). The factor of 2 is clox's.
- **Accounting.** All object memory goes through one function, so the VM knows the live byte count at every moment. Tables, the gray stack, text buffers and big-integer temporaries use plain `malloc` and are not counted.
- **Marking** uses an explicit gray stack, so a list nested 200,000 deep (a test) does not overflow the C stack.
- **Statistics** (`--stats`, or `stats` in the serve protocol): collections, total and longest pause, time spent marking and sweeping, bytes and objects allocated and freed, peak and current live bytes, the next threshold, load and run time.
- **`--gc-stress`** collects on every allocation and fills freed objects with `0xDB`, so a pointer that should have been a root reads garbage at once instead of failing rarely. Stage 4's differential tests, the stage 2 semantics tests, every program file and every random program also run this way.
- **Teardown** frees every object and checks that the byte count returns to exactly 0; the tests and the random-program runner fail on any session that does not.

### Acceptance: the roadmap's tests for stage 5

| Roadmap criterion | Test | Result |
| --- | --- | --- |
| Memory does not grow in a long run | [`programs/gc_churn.ml`](programs/gc_churn.ml) makes strings, lists, closures and cells every round and keeps 8 results; with a 256 KiB minimum heap, peak memory after 20,000 rounds is within 25% of the peak after 2,000 rounds and below 576 KiB | Passes |
| All tests pass with forced GC | Stage 4's differential tests, every program file and every random program run under `--gc-stress` with and without compiler optimizations; the stage 2 semantics tests run under it too | Passes |
| (added) Cycles are freed | `programs/gc_cycles.ml`: more than 50,000 objects freed, peak under 512 KB | Passes |
| (added) Nothing escapes the accounting | Every VM session ends with 0 bytes counted; LeakSanitizer finds no leaks | Passes |

### Does stress mode find rooting bugs?

One-off check, not part of this folder: seven rooting bugs were planted in copies of `mlvm.c`, one at a time, and each copy was run on 158 programs (the 8 program files and 150 random programs) with the default heap, a 4 KiB heap and `--gc-stress`, and under AddressSanitizer with the test suite.

| Planted bug | Programs that differ, Windows: default heap / 4 KiB / `--gc-stress` | ASan: failing tests | ASan: programs that differ under `--gc-stress` |
| --- | --- | --- | --- |
| 1. No `SYNC()` before the generic operator path | 0 / 0 / 31 | 0 | 32 |
| 2. No `SYNC()` before `MAKE_CELL` | 0 / 0 / 0 | 2 (a test written for this bug) | 0 |
| 3. A new list not rooted between its two allocations | 0 / 0 / 122 | 25 | 124 |
| 4. The loader does not root the code object it is filling | 0 / 134 / 158 | 50 | 158 |
| 5. Frames' closures not marked | 0 / 0 / 0 | 0 | 0 |
| 6. A code object's constants not traced | 3 / 121 / 158 | 52 | 158 |
| 7. The intern table not weak | 0 / 0 / 0 | 6 | 130 |

- **With the default heap, rooting bugs are almost invisible**: these programs rarely reach the 1 MiB threshold, so only one bug showed, in 3 of 158 programs. Stress mode is what makes them visible.
- **No single method caught all six real bugs.** Random programs under stress caught 1, 3, 4 and 6; the test suite under ASan caught 2 (only because a test was written for it after the random programs missed it), 3, 4, 6 and 7. Bug 7 leaves dead strings in the intern table, which are only read, never printed, so it shows only under ASan. Bug 1 slipped past the test suite and was caught by the random programs.
- **Bug 5 is not a bug.** A running function's closure is also on the stack, just below its arguments, so it was reachable anyway; the explicit root is kept as a guard against later changes.

## Matching the tree-walker: what Python's runtime put into minilang

The stage 4 VM could share the tree-walker's operators because both run on Python. A C VM cannot, and making outputs identical meant writing down behaviour that minilang never specified but inherits from Python. About 700 of `mlvm.c`'s lines exist only for this.

| Behaviour | Where it comes from | What the C VM does |
| --- | --- | --- |
| Integers have no size limit; `9223372036854775807 + 1` is exact | Python `int` | int64 with overflow checks, then arbitrary-precision integers with Knuth division |
| `int / int` is an int when exact, else the correctly rounded float, even for huge integers | `Interpreter.binary()` plus Python's true division | A port of CPython's `long_true_divide` rounding |
| Integer and float compare exactly (`2**53 + 1 != 2.0**53`) | Python's mixed comparison | Exact comparison through the integer value of the double |
| `%` takes the sign of the divisor, for ints and floats | Python | The same rules, including CPython's `float_rem` |
| Floats print as `repr()`, integral floats as integers (`1e22` prints 23 digits) | `stringify()` | Shortest round-trip digits; exact integer text for integral doubles |
| `num()` accepts `" 7 "`, `"1_000"`, `"inf"`, `"-Infinity"`, `"nan"`; rejects `"--5"` | `int()` and `float()` grammar, Unicode whitespace | The same grammar; only ASCII digits (see limits) |
| `num()`'s error shows Python's `repr()`: `'abc'`, `"it's"`, `[1, [...]]` | `"%r"` in the message | Python's quoting and escaping, `[...]` for a list inside itself |
| `len()` and indexing count code points | Python `str` | UTF-8 with a cached code point count |
| A huge integer mixed with a float is an `OverflowError`, not a minilang error, and `try` cannot catch it | Python's conversion | Reported as a Python-level error with the same type and message |
| Printing an integer of more than 4,300 digits is a `ValueError` | `sys.get_int_max_str_digits()` | The same limit |
| A list inside itself cannot be printed (`RecursionError`); inside a function it becomes `stack overflow (host recursion limit)` at the call | Python recursion limit plus `Interpreter.call()` | A nesting limit of 1,000, converted the same way |
| The message of `throw v` is computed at the throw, so an unprintable value fails even when a `catch` would take it | `RuntimeErr(stringify(value), ...)` | Same order |

These belong in the semantics draft (stage 1) or the specification (stage 8): either minilang specifies its numbers and text formats, or every implementation has to reproduce a particular host.

## How it is verified

1. **The same suites, unchanged.** `test_minilang.py` (stage 2) and the differential tests of `test_minilang_vm.py` (stage 4) are imported and run with the C VM patched in through `unittest.mock`: `minilang.Interpreter` for stage 2, `VM_ENGINES` for stage 4. `CVM` in `minilang_c.py` has the tree-walker's constructor and `run()` contract; it compiles in Python and runs the file in a long-lived `mlvm --serve` process, where each `CVM` is its own VM session with its own heap and globals.
2. **Differential testing** (`difftest.py`). Every program file and a corpus of generated programs run on the tree-walker and on six C VM configurations (plain and `--opt` bytecode, each with the default heap, a 4 KiB heap and `--gc-stress`); printed lines and endings must be identical, Python-level errors included.
3. **File format and loader.** Python round trip byte for byte; C disassembly identical to Python's for every program; every truncation of a file is rejected; 5 kinds of damaged header and 21 kinds of damaged code object are rejected, each with a specific message.
4. **Numbers.** `repr()` and `str()` of 30,000 doubles (random bit patterns, powers of two, boundaries) against Python; 150 random big-integer pairs through 8 operators; 9 operators on every pair of 14 kinds of number; `num()` on 44 strings and 9 other values.
5. **Memory.** The acceptance tests above, the growth policy, stress mode, deep structures, and every session freeing all its bytes.
6. **Sanitizers.** WinLibs has no AddressSanitizer or UBSan runtime on Windows (`cannot find -lasan`, `-lubsan`). On Windows the tests also pass with `-fsanitize=undefined,float-cast-overflow -fsanitize-trap=all`, which needs no runtime. In the Ubuntu 24.04 container, built with `-fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all`, the test suite and 2,000 further random programs (six configurations each) pass with no sanitizer report, including LeakSanitizer.

The full run, `python3 difftest.py --count 10000 --seed 1` on the final `mlvm.c` (599 s on Windows): 10,008 programs (the 8 files and 10,000 generated), 60,048 C VM runs, 0 mismatches, and no session with bytes left over. The tree-walker ended 4,601 programs with a value, 5,105 with an uncaught runtime error (so its text and line were compared), 139 with an escaped top-level `return`, 112 with an escaped `break` and 51 with a `RecursionError`; 222,164 printed lines were compared. The collector ran 2,337,959 times in each `--gc-stress` configuration, about 51,000 times with the 4 KiB heap, and 77 times with the default heap. An earlier version of the generator found one real difference, in the serve protocol rather than the VM: a trailing value that cannot be printed (a list inside itself) produced a malformed ending line. It was fixed and is covered by the corpus.

## Performance

The three programs are stage 4's, in [../minilang-vm/programs](../minilang-vm/programs/): recursive fib(25) (242,785 calls), a counting loop of one million iterations, and 2,000 strings built from 100 pieces each. `python3 minilang_c.py --bench` runs every engine 5 times, interleaved, and checks that all of them print the same output, including one extra `--gc-stress` run. The Python engines are timed in-process including parsing and compiling, as in stage 4. The C VM's time is the run as reported by `--stats`; loading the file takes about 7 µs and is not included, and compiling happened beforehand (`--emit`).

Environment: Intel Core i7-11700 (8 cores), 64 GB RAM, Windows 11 Pro 10.0.26300, CPython 3.14.6, WinLibs GCC 16.2.0 (UCRT). Measured twice on 2026-11-16 with the final `mlvm.c`; the table shows the second run, in seconds, median of 5 with the minimum to maximum in parentheses. Several virtual machines and other jobs were running on the machine: the Python engines' medians were up to 60% above their own minimums and above stage 4's figures for the same programs, while the C VM's times varied by under 10% across both runs and an earlier trial.

| Benchmark | Tree-walker | Python VM | C VM -O2 | C VM -O2, `--opt` bytecode | C VM -O0 |
| --- | --- | --- | --- | --- | --- |
| Recursive Fibonacci | 1.670 (1.130 to 1.803) | 0.724 (0.433 to 0.767) | 0.0058 (0.0051 to 0.0088) | 0.0061 (0.0057 to 0.0063) | 0.0326 (0.0325 to 0.0367) |
| Counting loop | 6.587 (6.233 to 6.871) | 3.352 (3.162 to 3.589) | 0.0218 (0.0212 to 0.0246) | 0.0184 (0.0181 to 0.0222) | 0.1831 (0.1657 to 0.1917) |
| String concatenation | 2.024 (1.904 to 2.087) | 1.317 (1.268 to 1.394) | 0.1067 (0.0994 to 0.1274) | 0.1053 (0.0976 to 0.1149) | 0.1740 (0.1692 to 0.1790) |

Speedup of the C VM (-O2, plain bytecode) over the Python engines, three ways, because the Python timings were noisy:

| Benchmark | vs tree-walker, run 1 / run 2 | vs Python VM, run 1 / run 2 | vs stage 4's medians (tree-walker / Python VM) |
| --- | --- | --- | --- |
| Recursive Fibonacci | 286x / 288x | 123x / 125x | 214x / 81x |
| Counting loop | 312x / 302x | 148x / 154x | 207x / 108x |
| String concatenation | 18x / 19x | 12x / 12x | 12x / 8x |

The last column divides stage 4's medians (measured on the same machine on a quieter day: 1.240, 4.520, 1.293 s and 0.470, 2.356, 0.864 s) by this run's C medians, so it is the conservative figure.

Starting the process, loading and exiting add about 20 ms on Windows: the whole-process medians were 0.026 s for Fibonacci, 0.042 s for the loop and 0.126 to 0.131 s for concatenation.

**Conclusion: on calls and loops the C VM is two orders of magnitude faster than either Python engine (81x to 154x over the same bytecode in Python); on allocation-heavy code it is one order faster (8x to 19x), and about 30% of that time is the collector.** Per instruction, using stage 4's instruction counts (derived, not measured directly), the C VM spends about 2.2 ns in Fibonacci (11 instructions per call), 1.3 ns in the loop (17 per iteration) and 23 ns in concatenation (23.3 per inner iteration, most of it allocating and copying strings), against 140 to 200 ns in the Python VM.

### The collector's cost

| Benchmark | Allocated | Objects | Collections | Pause, total (mark, sweep) | Longest pause | Share of run | `--gc-stress`: collections, run |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Recursive Fibonacci | 1.4 KB at load | 14 | 0 | 0 | 0 | 0% | 22, 0.005 s |
| Counting loop | 1.6 KB at load | 16 | 0 | 0 | 0 | 0% | 25, 0.022 s |
| String concatenation | 69.8 MB | 600,021 | 66 | 29.0 ms (0.024, 28.9) and 36.6 ms (0.031, 36.6) | 0.85 and 1.01 ms | 29% and 30% | 600,030, 0.17 s |

- **Fibonacci and the loop allocate nothing while running**: integers are immediate values and no variable is captured, so the collector never runs.
- **In concatenation, 30% of the run is the collector, and nearly all of it is sweeping.** About 1 MiB of dead strings accumulates between collections (the minimum heap); marking finds a handful of live objects (0.03 ms in total over 66 collections), and sweeping walks and frees about 9,000 dead strings each time (48 to 61 ns per object). A mark-sweep collector's cost grows with the heap, not with live data. A copying or generational nursery would touch only the live objects here, and a size-class allocator would make both allocation and freeing cheaper; that is reasoning, not a measurement, and the next step for this workload would be the allocator rather than the mark phase.
- **Pauses stay near a millisecond** because the heap is small: the longest was 1.01 ms.
- **Stress mode is cheap when little is live.** Collecting on every allocation made concatenation 600,030 collections and about 1.6 times slower, because each collection marks only the live set and sweeps one or two objects. Running the whole test suite and the random programs under stress costs little for the same reason.

### Stage 4's optimizations on the C VM

Speedup over plain bytecode, two runs:

| Benchmark | `assign-statement` only | `const-operand` only | `compare-jump` only | All (`--opt`) |
| --- | --- | --- | --- | --- |
| Recursive Fibonacci | 1.02x, 1.01x | 1.08x, 1.14x | 0.96x, 0.94x | 0.98x, 0.96x |
| Counting loop | 1.27x, 1.16x | 1.03x, 1.02x | 0.95x, 0.99x | 1.16x, 1.18x |
| String concatenation | 1.07x, 1.06x | 1.10x, 1.01x | 1.09x, 0.97x | 1.03x, 1.01x |

`bench_fib.ml` has no assignment statements, so its `assign-statement` cell (bytecode identical to plain) shows the noise: about 2%.

- **Fewer instructions still helps, less than in Python.** `assign-statement` cuts the loop from 17 to 13 instructions per iteration and gains 1.16x to 1.27x (Python VM: 1.62x to 1.64x). The constant-operand superinstructions gain up to 14% on Fibonacci.
- **The compare-and-branch superinstruction does not help in C either** (0.94x to 0.99x on Fibonacci and the loop). With all three on, Fibonacci is 2% to 4% slower than plain. In both hosts the measured winner is the change that removes instructions, not the ones that fuse them.
- **The C compiler's optimizer matters more than any of these**: -O2 is 5.6x faster than -O0 on Fibonacci, 8.2x to 8.4x on the loop, and 1.6x on concatenation, where the C library's allocator does the same work at either level.

### What this means for the roadmap

- **Stage 4's warning holds**: the Python prototype's 1.45x to 2.80x said nothing about a C VM, which runs the same bytecode 81x to 154x faster than the Python VM on calls and loops.
- **Memory management decides the allocation-heavy case.** After the port, the remaining cost in string building is allocating and sweeping, not dispatch, so the roadmap's performance order (specialize instructions, then JIT) does not apply to that workload until allocation is cheaper.

## File layout

| File | Contents | Lines |
| --- | --- | --- |
| `mlvm.c` | platform, buffers, values and objects (130), heap and collector (165), strings, tables, integers (500), number text (170), loader and verifier (390), formatting, disassembler, operators, builtins, interpreter (470), driver and serve protocol | 3,160 |
| `minilang_c.py` | `CVM`, the serve-protocol client, `--bench` | 410 |
| `difftest.py` | random program generator and differential runner | 450 |
| `test_minilang_c.py` | the tests | 520 |
| `programs/gc_cycles.ml`, `programs/gc_churn.ml` | the stage 5 acceptance programs | 36 |

## Limits and what is not verified

- Measured on one machine, built with GCC only (WinLibs 16.2.0 on Windows, 13.3.0 in the container); Clang and MSVC were not tried.
- Running a source file needs Python, because the front end is Python's. There is no C REPL.
- `num()` recognises only ASCII digits. Python's `int()` and `float()` also accept other Unicode decimal digits (Arabic-Indic, full-width), so `num()` of such text differs.
- In `num()`'s error message, Python decides which non-ASCII characters to escape from the Unicode database; the C VM uses a fixed list (controls, spaces, format characters, surrogates, private use), so unassigned or rare code points can be escaped differently.
- Printing nested lists stops at 1,000 levels and reports a `RecursionError`. In the tree-walker the limit comes from Python's stack and depends on what the stack already holds, so for nesting just above 1,000 levels the two engines can disagree; where exactly the tree-walker stops was not measured. Only cycles (both fail) and 200,000 levels (both fail) are tested.
- As in stage 4, the tree-walker's `stack overflow (host recursion limit)` for deeply nested *expressions* cannot happen in the VM.
- The loader rejected every truncated and corrupted file in the tests and runs under the sanitizers without reports, but files of random bytes were not fuzzed, so memory safety for arbitrary input is not proven.
- The collector stops the program for the whole collection; incremental and generational collection were not built. The heap statistics count object memory only, not the value stack, tables or `malloc` overhead, and process memory (RSS) was not measured.
- The random programs draw numbers and strings from fixed lists, mostly ASCII. Programs that print the time (`clock()`) are compared with `clock()` replaced by `0`.
- The planted-bug check is one-off; its seven copies of `mlvm.c` are not in this folder.

## Next steps

1. Stage 6: the `VM` struct already holds all state (the serve mode runs many VMs in one process); a C API with opaque handles and status codes can be put on top, with the root set extended by handles that C callers hold.
2. Stage 7: a C front end, so that a single executable runs source files; then CI on the two platforms tested here.
3. Measure an incremental collector (Lua 5.1's approach, needs a write barrier) only if pause times start to matter; at the sizes measured here they do not.

Detailed decisions are in the [roadmap](../../research/03-design-your-own-dynamic-language.zh-TW.md) (Traditional Chinese).
