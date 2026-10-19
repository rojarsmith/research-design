**English** | [繁體中文](README.zh-TW.md)

# C, Compilers and Processors: From Origins to Verifiable Understanding

Research date: 2026-10-19. The complete research is in Traditional Chinese; this page is an overview. The lab code and its technical README are in English.

## Questions

1. How was the C language created, and why does it look the way it does?
2. How are compilers created, and who compiled the first compiler?
3. How do you evaluate whether a processor can run software compiled from C?
4. How do these three fit together?

## Findings in brief

- **C describes an abstract machine.** Ritchie abstracted the PDP-11 of 1972 (byte addressing, a stack, indirect access, integer arithmetic) into a language; the ISO standard later wrote that machine down. Pointers, array decay and undefined behavior all follow from "must translate cheaply to the machine instructions of the day".
- **A compiler translates that abstract machine to one ISA under one ABI.** Lexing, parsing, semantic analysis, IR, optimization, code generation, assembling, linking and startup code each own a slice. The first compiler came from bootstrapping and cross compilation. Today "writing a C compiler for a new processor" usually means writing a GCC or LLVM backend.
- **Whether a processor can run C is mostly a toolchain question.** Hardware sets the cost (registers, addressing modes, multiply and divide, stack, unified address space); the toolchain decides feasibility (backend, ABI, runtime library, libc, crt0, debugging). The verdict is "yes", "yes with a cost", or "no", backed by a probe program and cost measurements. Measured across 19 clang targets: AVR has a 16-bit `int` and a 4-byte `double`, Cortex-M0 and rv32i divide through library calls, single-core MCUs get atomics only from the system, and BPF rejects signed division outright.
- **Integration comes from seeing language, compiler, ABI, ISA and microarchitecture as a stack of contracts.** The lab prints the same `fib` program at every layer.

## Lab

`labs/tinyc/` contains a C-subset compiler (about 1000 lines of Python) with a shared front end and two backends behind a small `Machine` interface, RV32IM and x86-64 System V, plus an assembly-level RV32IM simulator, minimal runtimes, five example programs and a differential test. `python3 run_tests.py` compiles each example four ways (tinyc to the simulator, host `cc` natively, tinyc's x86-64 output natively on the host, `clang --target=riscv32 -O2` to the simulator) and requires identical output and exit status. All five agree in this environment; tinyc's RV32 output assembles to real machine code with clang's integrated assembler and its x86-64 output runs next to the host libc. `labs/probe/` holds an implementation probe, a processor evaluation checklist, and `compare_targets.py`, which runs the evaluation protocol across every clang backend on the machine and writes the raw results to `results/`.

## Documents (Traditional Chinese)

| Document | Content |
| --- | --- |
| [Main report](README.zh-TW.md) | Conclusions, document map, quick start, scope |
| [01 Origins of C](research/01-c-language-origins.zh-TW.md) | CPL to BCPL to B to C, Unix, standardization K&R to C23, language-to-hardware mapping |
| [02 How compilers are built](research/02-how-compilers-are-built.zh-TW.md) | Pipeline, front/back end split, bootstrapping, cross compilation, Trusting Trust, what a new backend needs |
| [03 Processor C-readiness](research/03-processor-c-readiness.zh-TW.md) | Minimum hardware requirements, toolchain checks, a one-day evaluation protocol, difficult processor classes |
| [04 Integration path](research/04-integration-path.zh-TW.md) | The contract stack and a six-stage hands-on learning path |
| [05 Sources](research/05-sources.zh-TW.md) | Primary literature, standards, toolchain docs, ISA and ABI specs, test log |
| [06 Target comparison](research/06-target-comparison.zh-TW.md) | The same C programs compiled by clang for 19 targets (AVR, MSP430, Cortex-M, RISC-V variants, x86, MIPS, m68k, SPARC, wasm, PTX, BPF): implementation-defined contract, code size, runtime libcalls, verdicts |
| [tinyc README](labs/tinyc/README.md) | Four verification paths, backend difference table, calling conventions, language subset, exercises (English) |
| [Evaluation checklist](labs/probe/checklist.zh-TW.md) | Fill-in table for a candidate processor |
