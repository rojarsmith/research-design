# tinyc lab: one C-subset front end, two backends, one simulator, one differential test

Everything here is plain Python 3 with no dependencies. A host C compiler
(`cc`) enables the native paths; a `clang` with a `riscv32` backend enables one
more. Both are optional.

## Files

| File | Role in the toolchain picture |
| --- | --- |
| `tinyc.py` | The compiler: lexer, recursive-descent parser, symbol tables and type rules, frame layout, and a target-neutral code generator that talks to a `Machine` interface with two implementations, `RV32` and `X86_64` |
| `rv32sim.py` | The processor: executes RV32I + M assembly text with a byte-addressed little-endian memory and RARS-style `ecall`s; `--check-align` enforces the psABI stack-alignment rule at calls |
| `runtime_rv32.s` | crt0 + libc, minimal: `_start`, `print_int`, `print_char` for RV32 binaries produced by a real compiler |
| `runtime.h`, `runtime.c` | The same two builtins for the host, used both by the native gcc build and by tinyc's x86-64 output |
| `examples/*.c` | Five programs covering calls and recursion, loops and division, pointers and arrays, globals, short-circuit logic |
| `run_tests.py` | Compiles every example four ways and requires identical stdout and exit status |

## Run

```sh
python3 run_tests.py                                   # the whole loop, four paths
python3 tinyc.py examples/01_fib.c                     # RV32 assembly to stdout
python3 tinyc.py examples/01_fib.c --target x86_64     # x86-64 assembly, same AST
python3 tinyc.py examples/01_fib.c --tokens            # phase 1
python3 tinyc.py examples/01_fib.c --ast               # phase 2
python3 tinyc.py examples/01_fib.c -o fib.s && python3 rv32sim.py fib.s --stats --check-align
python3 rv32sim.py fib.s --trace 2>&1 | head -40       # one line per executed instruction
```

Run tinyc's x86-64 output on the host CPU:

```sh
python3 tinyc.py examples/01_fib.c --target x86_64 -o fib_x86.s
cc fib_x86.s runtime.c -o fib_x86 && ./fib_x86; echo "exit $?"
```

Optional, with a riscv32-capable clang (Ubuntu's `clang` package has it):

```sh
clang --target=riscv32 -march=rv32im -mabi=ilp32 -c fib.s -o fib.o   # real machine code from tinyc output
llvm-objdump -d fib.o | head -30
clang --target=riscv32 -march=rv32im -mabi=ilp32 -O2 -S -include runtime.h examples/01_fib.c -o fib_clang.s
cat fib_clang.s runtime_rv32.s > all.s && python3 rv32sim.py all.s --stats
```

## The four paths of `run_tests.py`

| Path | Compiler | Processor | What agreement proves |
| --- | --- | --- | --- |
| A | tinyc, RV32 backend | rv32sim | our compiler and our simulator agree with the standard |
| B | host gcc or clang | host CPU | the reference |
| C | clang -O2, riscv32 backend | rv32sim | our simulator runs code from a production compiler, so its ISA semantics are right |
| D | tinyc, x86-64 backend | host CPU | our compiler's output runs on a real processor under a real ABI, next to real libc code |

## What differs between the two backends

Everything above the `Machine` classes in `tinyc.py` is shared: lexing,
parsing, scoping, type rules, pointer scaling, control flow, the push/pop
evaluation strategy and the alignment bookkeeping. The two classes are about
120 lines each. This table is what "porting a compiler to a new processor"
means at the smallest scale:

| Concern | `RV32` (RV32IM, ILP32 psABI) | `X86_64` (System V LP64) |
| --- | --- | --- |
| Pointer size | 4 bytes, same as `int` | 8 bytes; `int *p` needs a larger slot, so frame layout asks `slot_size()` |
| Accumulator and temp | `a0`, `t1` | `rax`, `rcx` |
| Argument registers | `a0`-`a7` (8) | `rdi rsi rdx rcx r8 r9` (6); the front end checks `max_args` |
| Return address | register `ra`, saved by the prologue | pushed by `call` itself |
| Frame | `fp` set by prologue; `ra`, old `fp` saved below it | `rbp` pushed by prologue; return address above it |
| Load an `int` | `lw` | `movslq`, to keep it sign-extended in a 64-bit register |
| Compare to 0/1 | `slt`, `seqz`, `snez`, `xori` | `cmpq` then `setcc` then `movzbq` |
| Division | `div`, `rem` (M extension; defined on zero) | `cltd; idivl` (traps on zero) |
| Address of a global | `la` (absolute; `lui`/`addi` pair) | `leaq sym(%rip)` (position-independent) |
| Who calls `main` | tinyc emits `_start`: `call main; li a7, 93; ecall` | the host's crt0; tinyc emits nothing |
| Builtins | `ecall` with `a7` = 1 or 11 | ordinary calls into `runtime.c` |
| Stack alignment at calls | 16 bytes (psABI) | 16 bytes (System V); libc's `printf` crashes without it |

## Calling convention and frame layout

RV32: arguments in `a0`-`a7`, result in `a0`, return address in `ra`,
`fp`/`s0` as frame pointer, frame size rounded to 16 bytes. Locals live at
negative offsets from `fp`, below the saved `ra` and `fp`:

```
 fp -  4   saved ra
 fp -  8   saved fp (caller's)
 fp - 16   padding
 fp - 20   first local or parameter
 sp        16-byte aligned at every call
```

x86-64: arguments in `rdi rsi rdx rcx r8 r9`, result in `rax`, `rbp` as frame
pointer. The return address sits at `8(%rbp)`, the saved `rbp` at `0(%rbp)`,
locals start at `-8(%rbp)` in 8-byte slots; `int` arrays use 4 bytes per
element as the host ABI requires.

Temporaries are pushed during expression evaluation, which can leave the stack
pointer misaligned when a call happens inside a larger expression. The code
generator tracks the push depth and pads the stack around such calls
(`aligned_call`); `rv32sim.py --check-align` and path D both fail if it ever
gets this wrong.

## Language subset

`int`, `int *`, fixed-size `int` arrays (local and global), functions with up
to 6 (x86-64) or 8 (RV32) parameters, `if`/`else`, `while`, `for` (with a
declaration in the init clause), `return`, `+ - * / %`, comparisons, `&& || !`,
unary minus, `&x`, `*p`, `a[i]`, pointer arithmetic scaled by 4, pointer
difference, decimal, hex and character literals, `//` and `/* */` comments.
Two builtins: `print_int(x)` and `print_char(c)`. The value returned from
`main` is the exit status.

Not supported, in roughly the order you might add them: `char` and `lb`/`sb`,
`break`/`continue`, `switch`, `struct`, string literals, the preprocessor,
`long long` (needs register pairs on RV32), floating point (needs the F
extension or soft-float calls), an IR and any optimization.

## Exercises

- **A — Count the cost.** Compare "executed" counts from `run_tests.py` between tinyc and `clang -O2`. Find the three biggest reasons for the gap by reading both assembly files for `02_loops.c`.
- **B — Add `char`.** Extend the lexer, parser, type rules and both `Machine` classes. Pointer scaling must become 1 for `char *`. Add an example that prints a string one byte at a time.
- **C — Break the ABI on purpose.** Remove the padding in `aligned_call` and run `run_tests.py`. Path A now fails under `--check-align`; see whether path D crashes inside `printf` on your machine, and explain why the answer depends on the libc build.
- **D — Third backend.** Add an `RV32I` machine without the M extension: `mul`, `div` and `rem` become calls to `__mulsi3`, `__divsi3`, `__modsi3` that you implement in `runtime_rv32.s` with shifts and adds. Measure the cost. This is what `-march=rv32i` makes a real compiler do, and it is the "yes, with a cost" verdict from the processor-evaluation protocol made concrete.
- **E — Register allocation, lightly.** Keep the most recently assigned local in a callee-saved register and skip the reload. Measure how much of the gap in exercise A disappears.
