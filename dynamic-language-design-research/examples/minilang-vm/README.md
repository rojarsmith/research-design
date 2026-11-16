# minilang-vm: a bytecode compiler and stack VM

[Back to the overview](../../README.md) | [Roadmap (zh-TW)](../../research/03-design-your-own-dynamic-language.zh-TW.md) | [Tree-walking minilang](../minilang/README.md) | [Traditional Chinese README](README.zh-TW.md)

Runnable example for stage 4 of the research roadmap: a bytecode compiler and stack VM for [minilang](../minilang/README.md). The lexer and parser are reused through `import minilang`, neither copied nor modified. Everything after the AST is new: the resolver, compiler, code objects, VM and disassembler. Like minilang, it is a single Python file with no third-party dependencies. It shows the parts of stage 4 and how to accept them; it is not a production VM. Verified on Python 3.14.6.

## Running

```bash
cd dynamic-language-design-research/examples/minilang-vm
python3 -m unittest test_minilang_vm -v                        # 76 tests
python3 minilang_vm.py ../minilang/programs/fib.ml             # run minilang's example programs on the VM
python3 minilang_vm.py --dis ../minilang/programs/closures.ml  # disassemble: instructions, constant pool, line table, exception table
python3 minilang_vm.py --bench                                 # three benchmarks, 5 runs each, about 2 to 3 minutes
python3 minilang_vm.py --opt programs/bench_loop.ml            # turn on the compiler optimizations
python3 minilang_vm.py                                         # REPL; statements end with ';', Ctrl-D exits
```

The 76 tests are: minilang's 13 tests run on the VM with optimizations off and on (26), 4 checks that those tests really run on the VM, and 46 VM-specific tests.

## Pipeline

```text
source -> minilang.tokenize() and Parser (reused) -> AST
       -> Resolver: scopes, local slots, which variables inner functions capture, upvalue numbering
       -> Compiler: instructions, jump patching
       -> Code: constant pool, line table, exception table, loop table, computed maximum stack depth
       -> VM: one dispatch loop; call frames live in a VM list, not on the host stack
```

## Design at a glance

| Decision | Choice in this example | Roadmap discussion |
| --- | --- | --- |
| Stack or register machine | Stack. Each frame is one Python list: local slots first, then the operand stack; length = slots + maximum stack depth | Simple to compile; the same layout as CPython's frame "localsplus" |
| Compilation path | AST to Resolver (name resolution), then Compiler (instructions): two passes | CPython builds a symbol table before generating code; Lua uses a single pass |
| Instruction encoding | Fixed width, two list items per instruction: opcode, operand. Jump targets and the line table use offsets | Similar to CPython's 16-bit code units |
| Dispatch | An `if/elif` chain inside a `while` loop, ordered by how often the benchmarks execute each opcode | The equivalent of Lua's while-switch; Python has no computed goto |
| Constant pool | Literals, global names, and the Code objects of nested functions; deduplicated by (type, repr) so that `1`, `1.0` and `true`, which are equal in Python, stay separate | Lua also keeps global names in its constant table |
| Line table | Runs of (first offset, line), a new run only when the line changes; looked up by binary search on an error or a builtin call | CPython's `co_linetable`, Lua's lineinfo |
| Maximum stack depth | Computed after compilation by walking every control-flow path, checking that paths agree where they meet | What CPython's assembler does |
| Exceptions | One exception table per Code (start, end, handler offset), innermost first. On a throw the VM searches the current frame, then pops frames and searches each caller from its CALL offset; nothing is paid when no exception is thrown | CPython's zero-cost exception tables since 3.11 |
| Closures | A variable used by an inner function gets a new cell each time its scope is entered; inner functions' upvalues point at cells and are handed through each level in between | CPython's cell design. Lua and clox keep such variables on the stack as open upvalues and move them to the heap when the scope ends; that saves allocations in C, but in Python a cell from the start is simpler and faster |
| Call frames | Managed by the VM in a list; the depth limit is minilang's 400, reported as a catchable stack overflow | Since CPython 3.11, Python-to-Python calls do not recurse on the C stack |
| Shared with the tree-walker | `VM` subclasses `minilang.Interpreter`: builtins, output injection, stringify and equality rules, and the generic operator and indexing code (called only when a fast path does not apply) | Both engines share one value model and one set of error messages; name resolution, control flow, calls, closures and exceptions are the VM's own |

## minilang's scoping and how the VM matches it

minilang's environments are dictionaries filled as `let` statements run, and a name means the innermost environment that holds it at the moment of lookup. Within one function, source order decides this, because a scope's statements each run once, in order (a loop body gets a new scope per iteration), so the Resolver can assign slots statically. A closure is different: it may run before or after a later `let` in an enclosing scope. The VM handles the cases below, and each has a differential test.

| Behaviour | Why the tree-walker does this | What the VM does |
| --- | --- | --- |
| A closure sees an enclosing variable whose `let` comes after the closure | Names are looked up along the environment chain at call time | Such names compile to `LOAD_CHAIN`/`STORE_CHAIN`: try those cells innermost first, skip any still `UNDEFINED`, then fall back to the definite binding or the global table. Only names declared after the closure is created need this; everything else is a slot or an upvalue |
| Mutually recursive local functions, `let f = fn() { ... f() ... }` | Same | Same mechanism; works without special cases |
| A loop body is a new scope on every iteration | Each block execution creates `Environment(env)` | Captured variables get `MAKE_CELL` at block entry, so each iteration gets a new cell |
| `break` inside a function, outside any loop there, stops the caller's loop | `break` is the host exception `BreakSignal`, which crosses function calls and `try` | Compiled to `BREAK_OUT`: pop frames until a caller's CALL offset falls inside a range in its loop table, then jump to that loop's exit |
| Top-level `return`, and a `break` with no loop to stop | `ReturnSignal` or `BreakSignal` escapes `run()` as a Python exception | Raises the same Python exception |
| `throw nil` is caught as the string `"nil"` | `RuntimeErr` stores the message when the value is `None` | Uses the same `RuntimeErr`, so it behaves the same |

Most of these are side effects of how the tree-walker is written rather than deliberate language design. The VM reproduces them so that the same tests give the same results; whether to keep them belongs in the semantics draft (stage 1) or the specification (stage 8).

## How it is verified

1. **The same test suite.** `test_minilang.py` is imported and run with `minilang.Interpreter` replaced by the VM through `unittest.mock`, not copied or rewritten, once with optimizations off and once with them on.
2. **Differential tests.** The same program runs on the tree-walker and on the VM; every printed line and the way the run ends (returned value, full error message including the line, or an escaped host exception) must match. They cover the scoping and control-flow cases above, error lines for 17 programs (most spanning several lines), 11 binary operators on every pair of 15 kinds of value, unary and logical operators, indexing and index assignment, builtins, and the programs in `../minilang/programs/`.
3. **VM internals.** The opcode table is consistent; the constant pool deduplicates by type; the line table is run-length and maps offsets correctly; maximum stack depth; the stack depth check rejects inconsistent paths; every jump is patched; if/else and break jump to the right places; exception tables nest innermost first; only captured variables become cells; upvalues are handed through intermediate functions; the disassembler output matches exactly; VM calls do not recurse on the host stack.
4. **Checking the tests (one-off, not part of this folder).** Seven deliberate bugs were planted in the VM one at a time (closures ignore later enclosing `let`s, no new cells at block entry, `break` does not cross calls, binary operators report the wrong line, assignments report the wrong line, depth limit off by one, constant pool deduplicated by value only), and each one made the tests fail. Randomly generated programs (closures, shadowing, loops, `try/catch`, `throw`, `break` across calls, top-level `return`) were also compared across both engines and both optimization modes: 10,000 programs on the final version, all identical.

## Performance

The three fixed programs are in [programs/](programs/): `bench_fib.ml` (recursive fib(25), 242,785 calls), `bench_loop.ml` (a counting loop of one million iterations inside a function), `bench_concat.ml` (2,000 strings, each built from 100 pieces of `str(i % 10) + ","`). `--bench` runs every engine 5 times, interleaved, takes the median, and checks that all engines print the same output. Times include parsing and compiling (compiling each program takes about 80 µs, which is negligible).

Environment: Intel Core i7-11700 (8 cores), 64 GB RAM, Windows 11 Pro 10.0.26300, CPython 3.14.6 64-bit. Measured twice on 2026-11-16; the table shows the first run, in seconds, with the minimum to maximum of the 5 runs in parentheses.

| Benchmark | Tree-walker | VM | VM `--opt` | VM speedup | `--opt` speedup |
| --- | --- | --- | --- | --- | --- |
| Recursive Fibonacci | 1.240 (1.144 to 1.289) | 0.470 (0.418 to 0.538) | 0.461 (0.423 to 0.518) | 2.64x | 2.69x |
| Counting loop | 4.520 (4.098 to 4.834) | 2.356 (2.183 to 2.450) | 1.503 (1.340 to 1.546) | 1.92x | 3.01x |
| String concatenation | 1.293 (1.200 to 1.304) | 0.864 (0.796 to 0.915) | 0.646 (0.640 to 0.709) | 1.50x | 2.00x |

Speedups in the second run: VM 2.80x, 1.90x, 1.45x; `--opt` 2.70x, 3.19x, 1.90x. The same cell differed by up to 21% in absolute time between the two runs, and other work may have been running on the machine, so read the ratios, not the seconds.

**Conclusion: in Python the VM is faster than the tree-walker, but only by 1.45x to 2.80x.** Counts explain why. They were taken with a one-off counting script on smaller inputs; per-unit counts do not depend on input size.

| Benchmark and unit | Tree-walker node visits | VM instructions | `--opt` instructions | Tree-walker Python function calls | Time per node (derived) | Time per instruction (derived) |
| --- | --- | --- | --- | --- | --- | --- |
| Fibonacci, per call | 11.5 | 11.0 | 9.0 | 29.8 | 444 to 537 ns | 176 to 200 ns |
| Loop, per iteration | 14.0 | 17.0 | 11.0 | 33.1 | 323 to 333 ns | 139 to 145 ns |
| Concatenation, per inner iteration | 20.2 | 23.3 | 17.2 | 48.3 | 320 to 325 ns | 185 to 194 ns |

- **The VM does not take fewer steps.** It executes about as many instructions as the tree-walker visits nodes, and more in the loop.
- **What it saves is host-level overhead.** The tree-walker makes at least one Python function call per node (`exec`, `eval`, `binary`, `lookup`), raises and catches a Python exception on every `return`, creates a dictionary environment per block and call, and looks variables up through a chain of dictionaries. The VM makes almost no Python function calls while running (under 0.5 per unit in Fibonacci and the loop, compile included; 3.9 in concatenation, because `str()` itself is a Python function), and variables are list indexes.
- **Dispatch itself is still expensive.** Each instruction costs 140 to 200 ns: a chain of `op == constant` comparisons, list indexing, type checks and boxed integers. The work CPython's interpreter does for Python source (per-instruction dispatch, reference counting, object allocation) is paid again for every VM instruction, just less of it than for every tree-walker node.

### Each optimization measured separately

| Benchmark | VM | `assign-statement` only | `const-operand` only | `compare-jump` only | All (`--opt`) |
| --- | --- | --- | --- | --- | --- |
| Recursive Fibonacci | 1.00 | 1.04x, 0.94x | 0.96x, 0.87x | 1.01x, 0.93x | 1.02x, 0.97x |
| Counting loop | 1.00 | 1.64x, 1.62x | 1.00x, 0.98x | 1.02x, 1.01x | 1.57x, 1.68x |
| String concatenation | 1.00 | 1.22x, 1.30x | 1.03x, 1.02x | 1.08x, 1.03x | 1.34x, 1.30x |

Each cell gives the two runs' speedups over the unoptimized VM. `bench_fib.ml` has no assignment statements, so with `assign-statement` alone its bytecode is identical; that cell's 0.94x to 1.04x is the noise band.

- **Helps: `assign-statement`.** A statement-level `x = e;` stores directly without emitting `DUP` and `POP`. The loop drops from 17 to 13 instructions per iteration, and time falls roughly in proportion.
- **Within noise: `const-operand` (`ADD_CONST`, `SUB_CONST`, `LT_CONST`) and `compare-jump` (`JUMP_IF_NOT_LT`).** They save a `LOAD_CONST`, which is second in the if-chain and already cheap, while the new opcodes sit late in the chain behind a dozen comparisons. A one-off experiment (not part of this folder, median of 5 runs) moved these four superinstructions to the front of the chain: the unoptimized VM became 33% to 36% slower (Fibonacci 0.644 s to 0.856 s, loop 2.71 s to 3.67 s), because every other instruction paid four more comparisons, and the optimized VM did not get faster either. In a Python if-chain dispatch, a larger instruction set has a cost of its own. Both are kept in the code so the measurements can be reproduced.
- **Not done: constant folding and global caching.** Every operator in the three benchmarks has a variable operand, so nothing could be folded. `LOAD_GLOBAL` is already one dictionary lookup, and an inline cache in Python would need a version check of similar cost. Both points are reasoning, not measurements.

### What this means for the roadmap's stage order

- **"Tree-walker first, then bytecode" holds, and mainly for reasons other than speed.** This VM was written against a working reference implementation and its tests. That gave the differential tests and the random-program comparison something to judge against, and it is how the implementation-defined behaviour listed above was found.
- **Doing stage 4 in Python validates the design, not the speed.** The resolver, code objects, closures, exception tables and stack depth computation can be built, tested and corrected here before porting. The 1.45x to 2.80x is a property of the Python host and does not predict a C VM's speedup. This study did not measure a C implementation.
- **Measure on the target host.** Both the optimization that helped in Python (fewer instructions) and the ones that did not (superinstructions) depend on the if-chain dispatch. C's `switch` or computed-goto dispatch has a different cost structure, so step 2 of the roadmap's performance order (instruction specialization) needs its own measurement on a C version.

## File layout

| Section | Contents | Approximate lines |
| --- | --- | --- |
| opcodes | Instruction table (name, stack effect), optimization options | 90 |
| code object | `Code` (constant pool, line table, exception table, loop table, slot and upvalue descriptors), `Closure` | 70 |
| resolver | `Binding`, `Scope`, `FuncInfo`, `Resolver`: scopes, slots, captures, upvalues, chains | 210 |
| compiler | `Compiler`: statements and expressions, jump patching, superinstructions; `stack_depth()` | 280 |
| disassembler | `disassemble()` | 50 |
| VM | `VM.execute()`: dispatch loop, call and return, exception unwinding, `break` across calls | 340 |
| benchmarks | `--bench` | 60 |
| main | File mode, disassembly, REPL | 55 |

The tests are in `test_minilang_vm.py`, about 600 lines.

## Limits and what is not verified

- Measured on one machine with one Python version (3.14.6); speedups differed by at most 0.18 between the two runs, absolute times by up to 21%.
- When the host recursion limit is reached before the depth limit, the tree-walker reports `stack overflow (host recursion limit)`. The VM never produces that message because it does not recurse on the host stack. This can only happen with extremely deeply nested expressions and is not tested.
- Code objects are not serialized to files, so stage 4's motivation of cacheable, offline-compiled bytecode is not verified here.
- Memory is managed by Python; there is no stage 5 garbage collector.
- The REPL was checked once by piping input into it, with the same output as minilang's REPL; it has no automated test.

## Next steps

1. Serialize `Code` objects to a file and load them back to run, to verify offline precompilation.
2. Port the VM to a C subset using the same instruction table and these differential tests, and measure dispatch methods and instruction specialization on the C version.
3. After the port, move on to stage 5: the root set is the VM frames, the global table and the cells.

Detailed decisions are in the [roadmap](../../research/03-design-your-own-dynamic-language.zh-TW.md) (Traditional Chinese).
