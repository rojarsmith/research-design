# minilang: a single-file tree-walking interpreter

[Back to the overview](../../README.md) | [Traditional Chinese README](README.zh-TW.md)

Runnable example for stage 2 of the research roadmap: a complete pipeline for a small dynamic language in one Python file with no third-party dependencies. It demonstrates how the pieces fit together; it is not a production language. Verified on Python 3.13.

```bash
cd dynamic-language-design-research/examples/minilang
python3 -m unittest test_minilang -v   # 13 tests
python3 minilang.py programs/fib.ml
python3 minilang.py programs/closures.ml
python3 minilang.py programs/errors.ml  # ends with a deliberate uncaught error, exit code 1
python3 minilang.py                     # REPL; statements end with ';', Ctrl-D exits
```

Language: numbers, strings, booleans, `nil`, lists, first-class functions and closures, `let`, `if`/`else`, `while`, `break`, `return`, `throw`, `try`/`catch`. Only `nil` and `false` are falsy. Runtime errors carry line numbers. Call depth is capped at 400 and reported as a stack overflow.

The Traditional Chinese README maps each section of the file to the roadmap stage it illustrates and lists the steps for moving on to a bytecode VM.
