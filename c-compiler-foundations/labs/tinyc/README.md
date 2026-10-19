# tinyc lab: a C subset compiler, an RV32IM simulator, and a differential test

Everything here is plain Python 3 with no dependencies. A host C compiler
(`cc`) enables the differential test; a `clang` with a `riscv32` backend
enables the third path. Both are optional.

## Files

| File | Role in the toolchain picture |
| --- | --- |
| `tinyc.py` | The compiler: lexer, recursive-descent parser, symbol tables and type rules, code generation to RV32IM assembly |
| `rv32sim.py` | The processor: executes RV32I + M assembly text with a byte-addressed little-endian memory and RARS-style `ecall`s |
| `runtime_rv32.s` | crt0 + libc, minimal: `_start`, `print_int`, `print_char` for binaries produced by a real compiler |
| `runtime.h`, `runtime.c` | The same two builtins for the host, so examples compile unchanged with gcc or clang |
| `examples/*.c` | Five programs covering calls and recursion, loops and division, pointers and arrays, globals, short-circuit logic |
| `run_tests.py` | Compiles every example three ways and requires identical stdout and exit status |

## Run

```sh
python3 run_tests.py                                  # the whole loop
python3 tinyc.py examples/01_fib.c                    # assembly to stdout
python3 tinyc.py examples/01_fib.c --tokens           # phase 1
python3 tinyc.py examples/01_fib.c --ast              # phase 2
python3 tinyc.py examples/01_fib.c -o fib.s && python3 rv32sim.py fib.s --stats
python3 rv32sim.py fib.s --trace 2>&1 | head -40      # one line per executed instruction
```

Optional, with a riscv32-capable clang (Ubuntu's `clang` package has it):

```sh
clang --target=riscv32 -march=rv32im -mabi=ilp32 -c fib.s -o fib.o   # real machine code from tinyc output
llvm-objdump -d fib.o | head -30
clang --target=riscv32 -march=rv32im -mabi=ilp32 -O2 -S -include runtime.h examples/01_fib.c -o fib_clang.s
cat fib_clang.s runtime_rv32.s > all.s && python3 rv32sim.py all.s --stats
```

## Language subset

`int`, `int *`, fixed-size `int` arrays (local and global), functions with up
to 8 parameters, `if`/`else`, `while`, `for` (with a declaration in the init
clause), `return`, `+ - * / %`, comparisons, `&& || !`, unary minus, `&x`, `*p`,
`a[i]`, pointer arithmetic scaled by 4, pointer difference, decimal, hex and
character literals, `//` and `/* */` comments. Two builtins: `print_int(x)` and
`print_char(c)`. The value returned from `main` is the exit status.

Not supported, in roughly the order you might add them: `char` and `lb`/`sb`,
`break`/`continue`, `switch`, `struct`, string literals, the preprocessor,
`long long` (needs register pairs), floating point (needs the F extension or
soft-float calls), an IR and any optimization.

## Calling convention

tinyc follows the RISC-V ILP32 psABI closely enough for its output to
interoperate with clang-compiled code in the simulator: arguments in `a0`-`a7`,
result in `a0`, return address in `ra`, `fp`/`s0` as frame pointer, frame size
rounded to 16 bytes. Locals live at negative offsets from `fp`, below the saved
`ra` and `fp`:

```
 fp -  4   saved ra
 fp -  8   saved fp (caller's)
 fp - 12   padding
 fp - 16   padding
 fp - 20   first local or parameter
 ...
 sp        (16-byte aligned on entry)
```

Known deviation, left as an exercise: temporaries are pushed 4 bytes at a time
during expression evaluation, so a call nested inside a binary expression
(`fib(n - 1) + fib(n - 2)`) happens with `sp` misaligned by 4. The simulator
does not care; a real library using aligned vector loads would.

## Exercises

- **A — Count the cost.** Compare "executed" counts from `run_tests.py` between tinyc and `clang -O2`. Find the three biggest reasons for the gap by reading both assembly files for `02_loops.c`.
- **B — Add `char`.** Extend the lexer, parser, type rules and code generation. `sizeof` scaling must become 1 for `char *`. Add an example that prints a string one byte at a time.
- **C — Fix alignment.** Keep `sp` 16-byte aligned at every `call`. Verify by adding a check in `rv32sim.py`'s `call` handling.
- **D — Second backend.** Emit x86-64 assembly for the same AST and run it with the host `as`/`cc`. Decide which parts of `CodeGen` are target-independent.
- **E — Drop the M extension.** Replace `mul`/`div`/`rem` with calls to `__mulsi3`/`__divsi3`/`__modsi3`, implement them in `runtime_rv32.s` with shifts and adds, and measure the cost. This is what `-march=rv32i` makes a real compiler do.
