**English** | [繁體中文](README.zh-TW.md)

# Dynamic Languages from Zero: How Python and Node.js Came to Be, How to Evaluate Portability, and How to Design Your Own

Research date: 2026-10-19. This is an English overview; the full research is in Traditional Chinese. It answers three questions for engineers: how dynamic languages such as Python and Node.js went from nothing to a working ecosystem, how to evaluate a language's portability and execution environment, and how to design a dynamic language from scratch. Historical and technical facts cite official documentation or the authors' own papers and posts; the evaluation tables, staged plan, and example are this study's proposals, not any community's standard.

One-line conclusion: a dynamic language does not start as a "language" with a "platform"; it starts as a small interpreter written in C to solve a concrete problem, leaning on an existing operating system and C compiler, and then grows an ecosystem through extensibility, a specification process, and a platform support policy. Designing your own follows the same order: a working tree-walking interpreter first, then bytecode, garbage collection, JIT, and multi-platform support only when needed.

## Documents

| Document | Purpose |
| --- | --- |
| [Main report (zh-TW)](README.zh-TW.md) | Answers to all three questions, cross-language comparison, decision principles |
| [How Python and Node.js came to be (zh-TW)](research/01-how-python-and-nodejs-came-to-be.zh-TW.md) | CPython, V8, libuv, Node.js and Lua: origins, technology stacks, milestones, and how each became institutionalised |
| [Portability and runtime evaluation (zh-TW)](research/02-portability-and-runtime-evaluation.zh-TW.md) | Support tiers, toolchains, system dependency layers, ABI stability, binary distribution, constrained targets such as WASI and microcontrollers, and a 10-item checklist |
| [Design your own dynamic language (zh-TW)](research/03-design-your-own-dynamic-language.zh-TW.md) | Nine stages from problem statement to specification process, with acceptance criteria, decision tables, and common pitfalls |
| [Runnable example: minilang](examples/minilang/README.md) | A single-file tree-walking interpreter in Python with tests, covering lexing, parsing, closures, and exceptions |
| [Sources (zh-TW)](research/04-sources.zh-TW.md) | 55 official and first-hand sources, two secondary sources, evidence boundaries, and open questions |

## Key findings

| Topic | Python (CPython) | Node.js | Lua (comparison) |
| --- | --- | --- | --- |
| Origin problem | System administration on the Amoeba OS at CWI | Scalable servers with non-blocking I/O | Configuration and data entry for Petrobras at Tecgraf |
| Start | Implementation began in the 1989 Christmas holidays; 0.9.0 posted to alt.sources on 20 February 1991 | 2009, built on V8 (open-sourced with Chrome in September 2008) and libuv | Designed mid-1993, first implementation finished July 1993 |
| Implementation | C; stack-based bytecode, reference counting plus cyclic GC, specializing interpreter since 3.11, experimental JIT since 3.13 | C++ glue over V8 (Ignition, Sparkplug, Maglev, TurboFan) and libuv (epoll, kqueue, event ports, IOCP) | Strict ANSI C; register-based VM since 5.0 |
| Specification | Language Reference plus the PEP process; no ISO standard | ECMA-262 (first edition 1997, 17th edition 2026) under TC39 | Reference manual |
| Platform policy | PEP 11 tiers 1 to 3 with target triples | BUILDING.md tiers with minimum kernel, glibc and toolchain versions | "Any platform with a standard C compiler" |

Portability is evaluated in six layers: support policy, implementation language and toolchain, system dependency layer, extension ABI, binary distribution, and constrained environments; plus three runtime dimensions: performance tier, memory model, and concurrency model.

The design roadmap has nine stages: problem and scope, syntax and semantics draft, tree-walking interpreter, standard library and modules, bytecode compiler and VM, memory management, host interoperability, platforms and distribution, specification and process. Key decisions (value model, scoping, error model, concurrency, embedded versus standalone) are made in the first two stages because they are hard to change later.

## Boundaries

Platform tiers, toolchain versions, and release schedules are snapshots as of the research date and change between versions; recheck before implementation. Performance figures are each project's own benchmark reports and were not re-measured. Stage sizes are estimates derived from public teaching material and reference implementations; only the minilang example was executed and tested.
