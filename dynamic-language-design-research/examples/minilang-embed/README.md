# minilang-embed: an embedding API for the minilang C VM

[Back to the overview](../../README.md) | [Roadmap (zh-TW)](../../research/03-design-your-own-dynamic-language.zh-TW.md) | [Stage 5: minilang-c](../minilang-c/README.md) | [Stage 4: minilang-vm](../minilang-vm/README.md) | [Stage 2: minilang](../minilang/README.md) | [Traditional Chinese README](README.zh-TW.md)

Runnable example for stage 6 of the research roadmap, host interoperability. It turns the stage 5 VM, [`../minilang-c/mlvm.c`](../minilang-c/README.md), into a library with a public header, [`minilang.h`](minilang.h). A C program creates VMs, loads bytecode files, calls minilang functions with arguments, registers C functions that minilang code calls, passes every minilang value type in both directions, and gets errors back as status codes. The VM is not forked: `minilang.c` includes `mlvm.c`, whose command-line build and behaviour are unchanged. The folder also has a C test program, an example host (a CSV report written in minilang) and a short Python ctypes example. It shows the parts of stage 6 and how to accept them; it is not a production API.

Verified on Windows 11 with WinLibs GCC 16.2.0 and Python 3.14.6, and on Ubuntu 24.04 (podman) with GCC 13.3.0, Python 3.12.3, AddressSanitizer, UndefinedBehaviorSanitizer and LeakSanitizer.

## Running

```bash
cd dynamic-language-design-research/examples/minilang-embed
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_embed test_embed.c minilang.c -lm    # the C tests
gcc -std=c11 -Wall -Wextra -Werror -O2 -o mlfilter mlfilter.c minilang.c -lm       # the example host
python3 -m unittest test_minilang_embed -v                                        # 10 tests, about 1 s (needs ../minilang-c/mlvm too)
python3 ../minilang-vm/minilang_vm.py --emit report.mlc programs/report.ml
./mlfilter report.mlc data/orders.csv                                             # a CSV report written in minilang
./mlfilter --gc-stress report.mlc data/orders.csv                                 # the same, collecting on every allocation
gcc -std=c11 -Wall -Wextra -Werror -O2 -shared -o minilang.dll minilang.c -lm      # optional; Linux: -shared -fPIC -o libminilang.so
python3 embed_ctypes.py                                                           # the same API from Python, standard library only
```

On Windows, `python3` may be `python`. Nothing is needed beyond what stage 5 uses: GCC (WinLibs) and Python. A host is built by compiling `minilang.c` next to it. `gcc -c minilang.c` gives an object file that exports only the `ml_` functions of `minilang.h` (checked with `nm` on both platforms), because everything from `mlvm.c` stays `static`.

The 10 tests: the C test program (13 test functions, 2,526 checks) plain, under `--gc-stress` and with a 4 KiB heap; one test that API misuse aborts by default; the example's report plain and under `--gc-stress`; a CSV file without the column the script reads; the ctypes example, which is skipped unless the shared library has been built; and two regression tests for `mlvm.c` itself, which run stage 5's command-line VM and its serve mode (see "Two fixes" below).

## Pipeline

```text
host program (C) -> minilang.h: ml_open, ml_load, ml_run, ml_call, ml_register, handles, status codes
                 -> minilang.c: handle slots and scopes, the host-function trampoline, error mapping
                    (#define MLVM_LIBRARY, #include "../minilang-c/mlvm.c")
                 -> mlvm.c: loader, verifier, interpreter, collector (stage 5) and five embedding hooks
bytecode files   <- python3 ../minilang-vm/minilang_vm.py --emit   (stage 4's front end, unchanged)
```

## What changed in mlvm.c

144 lines added and 36 changed (3,158 to 3,266 lines). The command line, the output and the one-line build are unchanged; stage 2's 13 tests, stage 4's 76 and stage 5's 94 pass unchanged, and stage 5's random-program check (1,008 programs, 6,048 C VM runs) finds no difference.

| Change | Why |
| --- | --- |
| `#ifndef MLVM_LIBRARY` around the driver (`main`, `--serve`, the statistics text), the disassembler and binary stdio | The library has no `main`, and unused `static` functions would fail `-Werror` |
| Object kind `O_HOST` (`ObjHost`: name, arity, C function pointer, user data), traced, printed and typed like a builtin | Host functions are minilang values: they can be stored, passed and compared |
| `OP_CALL` calls an `ObjHost` through the `call_host` hook, then reloads its cached `stack`, `regs` and `sp` | A host function may run minilang code, which can grow and so move the value stack |
| `mark_roots()` calls the `mark_host` hook | The host's handles are roots |
| `execute()` split into frame setup and `run(vm, floor, result)`; `RETURN`, `BREAK_OUT`, exception unwinding and the recursion-error conversion stop at `floor` instead of frame 0 | A run can start above a suspended minilang call, and nothing unwinds into frames that wait on the other side of a C function |
| `mem_limit` and `over_limit`: `heap_realloc()` sets the flag; `OP_JUMP` and `OP_CALL` raise `MemoryError` | Out of memory as a status, without a failure path at every allocation |
| The temporary root stack grows (`malloc`, never the GC heap) instead of holding a fixed 64 | A bug fix, below |
| A slot that holds no cell at `LOAD_CELL`, `STORE_CELL` or `MAKE_CLOSURE` ends the run with an uncatchable `SystemError` instead of calling `fatal()` | A bug fix, below |

The command-line VM leaves every hook zero. The test of `over_limit` in `OP_JUMP` and `OP_CALL` costs nothing measurable: over two runs of 9 interleaved repetitions of stage 4's three benchmarks, the final `mlvm` took 0.95 to 1.03 times the old one's run time (medians; the two versions' minimum-to-maximum ranges overlap on every benchmark), with identical output.

### Two fixes

Stage 8's conformance tests found that function literals nested 64 deep made `mlvm` stop with `mlvm: fatal: temp root stack overflow` (exit status 70), and in `--serve` every other session in the process died with it; 63 levels worked. The loader keeps each code object it is filling on the temporary root stack, one per level of nesting, but that stack had 64 fixed entries, so the loader's own limit, 200 levels ("functions nested too deeply"), could never be reached. The stack now grows; growing uses `malloc`, so pushing a root still never collects. The same reasoning applied to the other `fatal()` that input can reach: bytecode that passes the verifier but reads a slot holding no cell as a cell (the verifier cannot rule this out statically) killed the process; it now ends that run with a `SystemError`. An embedding host must not be killed by a file it loads.

Regression tests: programs whose function literals nest 63, 64, 200 and 201 deep, and a stage 4 bytecode file with one `LOAD_LOCAL` changed to `LOAD_CELL`, run on the library (`test_embed`, plain, under `--gc-stress` and with a 4 KiB heap), on the command-line VM (plain and `--gc-stress`), and in one serve process with a plain and a stressed session. Up to 200 levels load and run, 201 is the load error "functions nested too deeply", the bad file ends with `SystemError: bytecode reads a slot that holds no cell`, and the serve process and its other session keep working. On the stage 5 executable from before the fix, 64, 200 and 201 levels and the bad file all end with exit status 70. What still calls `fatal()`: a `malloc` failure, an unknown opcode (which the verifier rules out) and `vsnprintf` failing on one of the VM's own format strings.

## The API at a glance

| Group | Functions |
| --- | --- |
| VMs | `ml_open(options)`, `ml_close` (returns the bytes the heap still counted, which must be 0), `ml_set_output`, `ml_get_stats`, `ml_collect`, `ml_api_version` |
| Programs and calls | `ml_load`, `ml_load_file`, `ml_run`, `ml_call` |
| Globals and host functions | `ml_get_global`, `ml_set_global`, `ml_new_function`, `ml_register` |
| Handles | `ml_scope_open`, `ml_scope_close` (optionally carrying one value out), `ml_pin`, `ml_release` |
| Values | `ml_new_nil`, `_bool`, `_int`, `_float`, `_string`, `_list`; `ml_typeof`, `ml_truthy`, `ml_equal`; `ml_get_bool`, `_int`, `_float`, `_string`; `ml_tostring`; `ml_list_len`, `_get`, `_set`, `_push` |
| Errors | `ml_throw`, `ml_throwf`, `ml_error_message`, `ml_error_type`, `ml_error_line`, `ml_error_value`, `ml_clear_error`, `ml_status_name` |

A host function has one signature, `ml_status fn(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud)`; the VM checks its arity (`-1` for any) before calling it, as it does for builtins. `minilang.h` follows the roadmap's principles for a C API: only opaque types and C built-in types in the header (`ml_vm` is opaque, `ml_value` is two integers), a status code from every call with the error details fetched separately, no internal structure in the header, and a version macro, `MINILANG_API_VERSION`, with `ml_api_version()` for checking at run time.

```c
ml_vm *vm = ml_open(NULL);                        /* NULL: default options */
ml_value prog, fn, arg, result;
if (ml_load_file(vm, "app.mlc", &prog) != ML_OK || ml_run(vm, prog, NULL) != ML_OK)
    fprintf(stderr, "%s: %s\n", ml_error_type(vm), ml_error_message(vm));
ml_get_global(vm, "double", &fn);
ml_new_int(vm, 21, &arg);
if (ml_call(vm, fn, 1, &arg, &result) == ML_EXCEPTION)
    fprintf(stderr, "line %d: %s\n", ml_error_line(vm), ml_error_message(vm));
ml_close(vm);                                     /* frees everything, handles included */
```

## Rooting: handles in scopes

### The rule

Host code never holds a minilang value. Every API function that produces one gives an `ml_value` handle: the index of a slot in a table the VM owns, plus a generation count. The collector marks every slot (`mark_roots()` calls `mark_handles()`), so a value stays alive exactly as long as some handle to it does. A handle is released by `ml_release`, by closing the scope it was made in, or by closing the VM.

- **Scopes.** Handles are made in the innermost open scope; `ml_scope_close` releases all of them at once and can carry one value out to the enclosing scope (V8's `EscapableHandleScope`). The VM opens a scope around every call of a host function, so a host function's arguments and temporaries need no cleanup, and a host function cannot close its caller's scopes. A host loop that makes values on every iteration opens a scope per iteration, as `mlfilter.c` does per CSV row.
- **Pins.** `ml_pin` makes a handle that belongs to no scope, for a value the host keeps between calls, such as a callback or a loaded program; only `ml_release` frees it.
- **Detection.** A slot's generation is odd while the slot is in use and changes on every release, so an old handle never matches a reused slot and the value behind a stale handle is never read. Misuse is: using a released handle, a handle from a closed scope or a handle a host function kept from its arguments; closing scopes out of order; returning from a host function with a scope open; calling the API from the print callback; closing a running VM. By default the library prints the problem and calls `abort()`; with `ml_options.misuse_returns` it returns `ML_MISUSE` instead. The tests use both.
- **The one raw pointer.** `ml_get_string` returns the string's bytes inside the heap, without copying, valid while that handle is live. Lua makes the same promise for string pointers while the value stays on its stack. Objects never move, so the pointer is good for as long as the handle lives; `mlfilter.c`'s `sort_by` relies on this while sorting.

This is cheap because stage 5's collector never moves objects: a handle is an index the collector reads, not a pointer it must update. With a moving collector the API would stay as it is, but every pointer inside the library would need the same care, which is the reason V8 uses handles everywhere.

### Why handles, not the alternatives

| Design | Who uses it | What it would mean here |
| --- | --- | --- |
| Values on a virtual stack, referred to by index | Lua: "Lua uses a virtual stack to pass values to and from C" | As safe for the collector, but every call is index arithmetic (`-1`, `-2`), and pushing a temporary shifts the indices of everything else |
| Reference-counted pointers, owned or borrowed | CPython: `Py_INCREF` and `Py_DECREF`; "using a borrowed reference after the owner has disposed of it risks using freed memory" | Needs counts this VM does not have, and a borrowed pointer is exactly the unrooted value this API is meant to rule out; a mistake shows only when the memory is reused |
| Handles in scopes, plus handles that outlive scopes | V8: when a `HandleScope` ends, "all handles created within that scope are removed from the stack"; Node-API: `napi_open_handle_scope` and `napi_ref` | Chosen. Scopes make cleanup automatic; generations turn a mistake into an immediate, named error instead of a rare crash |

Sources: [V8 docs: embed](https://v8.dev/docs/embed), [Node-API](https://nodejs.org/api/n-api.html); and, not in the topic's [source list](../../research/04-sources.zh-TW.md), [Lua 5.4 Reference Manual §4](https://www.lua.org/manual/5.4/manual.html#4) and [Python: Extending and Embedding, reference counts](https://docs.python.org/3/extending/extending.html#reference-counts).

### Does stress mode prove it?

The C tests run plain, under `--gc-stress` (collect on every allocation and fill freed objects with `0xDB`) and with a 4 KiB heap, built at -O0 and -O2 on Windows and with AddressSanitizer, UBSan and LeakSanitizer in the container. Every check passes in every configuration, and every VM ends with 0 bytes counted. The "rooting" test holds 300 strings and a 300-item list only through handles across forced collections and garbage-making calls.

As in stage 5, a one-off check, not part of this folder: five bugs were planted one at a time in copies of the code, and each copy ran `test_embed`.

| Planted bug | Windows: default heap / 4 KiB / `--gc-stress` | ASan: default heap / `--gc-stress` |
| --- | --- | --- |
| 1. `mark_roots()` does not call the handle hook | 302 failed checks / crash / crash | use after free / use after free |
| 2. Pinned handles are not marked | 2 failed checks in each | use after free / use after free |
| 3. `ml_new_function` does not root the name between its two allocations | passes / passes / the next load fails on corrupted strings | passes / use after free |
| 4. `OP_CALL` does not reload its stack pointers after a host call | passes in all three | use after free / use after free |
| 5. `ml_call` lets a collection run between the end of the run and the result's handle | crash / passes / 32 failed checks | use after free / use after free |

- Bugs 1, 2 and 5 show with the default heap only because the tests call `ml_collect()` themselves. Bug 3 shows only under stress, which collects between exactly those two allocations.
- **Bug 4 is not a rooting bug, and stress mode cannot find it.** The value stack moved (`realloc`) during a nested call, and the old memory still held the right values; only ASan saw it. Reentrancy adds this second class of bug, a stale pointer into memory that moved, and stage 5's conclusion holds again: stress mode and a sanitizer together, not either alone.

## Errors across the boundary

### Status codes and the pending error

| Status | Meaning | Pending error (`ml_error_type`) |
| --- | --- | --- |
| `ML_OK` | Success | none |
| `ML_EXCEPTION` | A minilang exception: `throw`, a runtime error, or `ml_throw`/`ml_throwf` in C; catchable by `try`/`catch` | `RuntimeErr`, with message, line and value |
| `ML_ERROR` | An error minilang cannot catch | `OverflowError`, `ValueError`, `RecursionError` (stage 5's Python-level errors), `BreakSignal`, `ReturnSignal`, `SystemError` (host bugs), `LoadError` |
| `ML_NOMEM` | The heap passed `heap_limit` | `MemoryError`, not catchable |
| `ML_TYPE_ERROR` | Wrong type, out of range, or not UTF-8 | A catchable exception such as `expected an integer, got string` |
| `ML_NOT_FOUND` | No such global | A catchable `undefined variable 'x'` |
| `ML_LOAD_ERROR` | The loader rejected a bytecode file | `LoadError`, with the loader's message |
| `ML_MISUSE` | A host bug, when `misuse_returns` is set | `SystemError` |

The type names are those of the Python exceptions the tree-walker would raise, as in stage 5's serve protocol. A failing call leaves its error pending; `ml_error_message`, `ml_error_line` and `ml_error_value` (the thrown value itself, or else the message) read it. It stays until the next failing call, `ml_clear_error`, or the start of `ml_run` or `ml_call`. Node-API works the same way: every call returns a `napi_status`, and `napi_get_last_error_info` describes the last call.

### The rule for host functions

A host function fails by returning a status other than `ML_OK` with an error pending. The simplest way is to return the status of the API call that failed:

```c
static ml_status fmt(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    double x;
    ml_status st = ml_get_float(vm, args[0], &x); /* a string: ML_TYPE_ERROR, now pending */
    if (st != ML_OK) return st;                   /* minilang sees a catchable exception at the call */
    if (x > 1e300) return ml_throwf(vm, "fmt(): number too large"); /* or raise one directly */
    ...
}
```

The VM then unwinds exactly as if a builtin had failed: an `ML_EXCEPTION` is caught by the nearest `try` around the call, and takes the call's line if it has none of its own; an `ML_ERROR` or `ML_NOMEM` goes straight out to the host that started the run. A host function that handles an error itself returns `ML_OK`, which clears it. Returning an error status with nothing pending is a host bug and becomes a `SystemError`.

### No longjmp through host frames

Lua raises errors with `longjmp`: "Internally, Lua uses the C `longjmp` facility to handle errors", and `lua_error` "does a long jump, and therefore never returns". An error inside an API call made by a C function jumps over that function's frame to the nearest `lua_pcall`, so a C function holding a `malloc`'ed buffer at that moment has to be written for it. Here nothing jumps. Errors inside the VM were already return values (stage 5's `rt_error()` returns -1 and the loop unwinds with the exception tables); the API returns them as statuses, and a host function's status goes back into the loop the same way. Every C frame between two minilang frames returns normally, and `mlfilter.c`'s `sort_by` frees its arrays on the error path like any C function.

The price is that the VM cannot stop in the middle of an allocation, which shapes the memory limit:

- **The heap limit is soft.** With `ml_options.heap_limit`, an allocation that would pass the limit collects first; if the heap is still over, the allocation succeeds anyway and sets a flag. The interpreter tests the flag at every `JUMP` (every loop's back edge) and `CALL`, which every unbounded computation passes through, and raises `MemoryError` there; API calls test it right after allocating and return `ML_NOMEM`. The heap can pass the limit by what one loop iteration or one straight stretch of code allocates. In the tests, with a 256 KiB limit, a loop that grows a list without end, the same loop inside `try`/`catch` (which does not catch it) and a recursion that builds lists without any loop all end with `ML_NOMEM`, the peak stays under twice the limit, and the VM works normally afterwards.
- **A `malloc` failure stays fatal.** If the C library returns `NULL`, the VM prints a message and exits, as in stage 5. Making that recoverable would mean a status check at hundreds of allocation sites in `mlvm.c`, the rewrite this example avoids.
- **The C stack is bounded.** Each minilang -> C -> minilang level uses C stack for the interpreter loop, the trampoline and the host function. At most 200 host functions can be active at once; the next call is a catchable `stack overflow (max host call depth 200)`, in addition to minilang's 400-frame limit. The test that recurses through C reaches the limit on Windows (at -O0 and -O2) and under ASan.

### Signals stop at C

In the tree-walker, `break` outside a loop and a top-level `return` are not errors but signals that unwind to a caller's loop or out of the script (stage 4's "forced semantics"). A C caller has no loop, so they stop there: a function called from C that runs such a `break` ends with `ML_ERROR`, type `BreakSignal`, and a script's top-level `return` ends `ml_run` with `ReturnSignal` and the returned value as the error's value. If a host function passes either on, it stays an uncatchable error and does not break the loop around the call. This is the one place where a host function behaves differently from a builtin in the tree-walker, through which the signal would pass.

## Reentrancy

```text
host: ml_call(nested_throw, 4)
  minilang nested_throw(x)        frame 0, run(floor 0), inside try
    C apply(f, y)                 its own handle scope; host depth 1
      minilang f(y)               frame 1, run(floor 1)
        C apply(g, z)             host depth 2
          minilang g(z)           frame 2, run(floor 2): throw "inner 4"
          run(floor 2) finds no handler at or above its floor and returns; ml_call returns ML_EXCEPTION
        apply returns that status; the error is still pending; OP_CALL in f fails
      run(floor 1) finds no handler in f and returns; ml_call returns ML_EXCEPTION
    apply returns it; OP_CALL in nested_throw fails
  the handler in frame 0 catches "inner 4"; the C frames all returned normally
```

- All frames share one value stack and one frame array. A nested run pushes above the suspended frames and, however it ends, returns with the frames and stack as it found them.
- The tests cover three levels (`nested`), the exception above crossing two C frames (`nested_throw`), C catching an inner error and returning a fallback, recursion through C up to the depth limit, a `break` that reaches C, a program run with `ml_run` from inside a host function called from minilang, and a waiting frame whose locals survive an inner call that moved the stack (`keeps_locals`, 300 frames deep).
- VMs are independent, like V8's isolates ("a VM instance with its own heap"); the tests interleave two. One VM is used by one thread at a time.

### What crossing costs

One-off measurement at -O2 on the machine used for stage 5, one million calls each, three repetitions: a minilang call to a minilang function took 9 to 12 ns; a minilang call to a host function that reads two integers and returns one, 25 to 29 ns; a call from C (open a scope, make two integers, `ml_call`, read the result, close the scope), 36 to 45 ns. Crossing the boundary costs about two to five minilang calls, so a host should pass a list in one call rather than call once per element.

## Example host: mlfilter

[`mlfilter.c`](mlfilter.c) reads a CSV file and lets [`programs/report.ml`](programs/report.ml) decide what to keep and how to print it. The C side parses the file, builds each row as a minilang list (numbers parsed) inside a per-row scope, calls `keep(row)`, collects the kept rows in a list, and finally calls `report(rows)`. It registers three host functions that minilang lacks: `field(row, name)`, which finds the column through the CSV header passed as user data; `fmt(number, decimals)`; and `sort_by(list, key)`, which calls the minilang key function for every item, so minilang runs inside C inside minilang. A row whose `keep()` throws is reported with the script's line and skipped.

```text
$ ./mlfilter report.mlc data/orders.csv
data/orders.csv:6: row skipped: negative amount -15 (script line 13)      (stderr)
mlfilter: 8 rows, 4 kept, 1 skipped                                      (stderr)
Chen & Sons     TW  1200.00
Garnet Labs     TW  860.75
Acme Trading    TW  250.50
Fjord Tools     NO  100.00
total 2411.25 in 4 orders of at least 100
```

The output is the same under `--gc-stress`.

[`embed_ctypes.py`](embed_ctypes.py) uses the same API from Python through `ctypes`. Python is the host: it loads the shared library, registers a Python function as a host function, and calls minilang functions that call it. When the Python function throws (with `ml_throw`), minilang catches it, or the status reaches Python. The DLL built by WinLibs links only to Windows' own C runtime, so nothing else is needed; on Linux it runs against `libminilang.so`.

## Design trade-offs against Lua, CPython and V8

| Question | minilang-embed | Lua | CPython | V8 / Node-API |
| --- | --- | --- | --- | --- |
| How C refers to a value | A handle: slot index and generation | An index into a virtual stack | A `PyObject *`, owned or borrowed | A `Local` handle; a `napi_value` |
| How it is released | Its scope closes, or `ml_release` | It is popped | `Py_DECREF` | The `HandleScope` ends; `napi_close_handle_scope` |
| Keeping a value between calls | `ml_pin` | `luaL_ref` into the registry | `Py_INCREF` | A persistent handle; a `napi_ref` |
| How errors reach C | A status and a pending error; nothing jumps | `longjmp` to the nearest `lua_pcall` | `NULL` or `-1` and the interpreter's exception state | Node-API: a `napi_status`; a pending exception is thrown in JavaScript when the native call returns |
| Collector | Non-moving mark-sweep; handles are roots | Values on the stack are roots | Reference counts plus a cycle collector | Moving; handles are required |

- **What this design pays.** Every value access goes through a slot and a generation check, and a host function's arguments are copied into handles: the costs measured above. Error codes make C code longer, as with Node-API; Lua's `longjmp` keeps C code short but makes it unsafe to hold resources across API calls.
- **What it keeps open.** Because the host only ever sees handles, a moving or generational collector could replace stage 5's without changing `minilang.h`; only `minilang.c` and `mlvm.c` would change. CPython's experience shows the opposite case: once `PyObject *` is public, changing memory management breaks extensions (roadmap: PEP 384's lesson).

Sources: [V8 docs: embed](https://v8.dev/docs/embed), [Node-API](https://nodejs.org/api/n-api.html), [PEP 384](https://peps.python.org/pep-0384/); not in the topic's source list: [Lua 5.4 Reference Manual §4](https://www.lua.org/manual/5.4/manual.html#4), [Python: Extending and Embedding](https://docs.python.org/3/extending/extending.html).

## Acceptance: the roadmap's tests for stage 6

| Roadmap criterion | Test | Result |
| --- | --- | --- |
| Write an extension function in C and call it from the language | `test_embed.c` "host functions": arity, every value type, errors raised in C caught by minilang at the call's line; `mlfilter.c`'s `field`, `fmt` and `sort_by` | Passes |
| Embed and run a script from a C host program | `mlfilter.c`; `test_embed.c` "lifecycle", "calls", "reentrancy" | Passes |
| (added) Errors cross the boundary in both directions without `longjmp` | "minilang errors", "host functions", "reentrancy" | Passes |
| (added) An unrooted value cannot be held, or its use is detected | "handles", "rooting", `--crash stale`; every test under `--gc-stress`; the planted bugs above | Passes |
| (added) Out of memory is a status, and the VM survives it | "heap limit" | Passes |
| (added) Stage 5's executable and tests are unchanged | stage 2, 4 and 5 suites, stage 5's random programs, the benchmark ratio | Passes |
| (added) No input kills the process: up to 200 nested function levels load, 201 is a load error, a non-cell slot is an error | "nesting" in `test_embed.c`; `Nesting` in `test_minilang_embed.py` (command-line VM and serve mode) | Passes; fails on stage 5's executable |

## File layout

| File | Contents | Lines |
| --- | --- | --- |
| `minilang.h` | the public API, with the rules in comments | 180 |
| `minilang.c` | handles and scopes, errors, the host-function trampoline, calls, values; includes `../minilang-c/mlvm.c` | 900 |
| `test_embed.c` | the C tests: 13 test functions | 820 |
| `mlfilter.c`, `programs/report.ml`, `data/orders.csv` | the example host, its script and data | 280 |
| `embed_ctypes.py`, `programs/shout.ml` | the ctypes example | 110 |
| `test_minilang_embed.py` | compiles the programs, writes the nesting and bad-cell files, runs all of the above | 210 |
| `tests/*.ml` | the programs the C tests call | 90 |

## Limits and what is not verified

- Built with GCC only (WinLibs 16.2.0 on Windows, 13.3.0 in the container); Clang and MSVC were not tried.
- Threads were not tested. A VM is meant for one thread at a time; VMs share no mutable state except a cached timer frequency on Windows.
- A handle does not record its VM, so a handle used with the wrong VM is not detected.
- The pointer from `ml_get_string` is not checked: using it after its handle is released is undefined behaviour, and no test does so.
- The heap limit counts object memory only, as stage 5's statistics do: not the value stack, the handle tables or `malloc` overhead, so the process can use more. A `malloc` failure still exits the process, as in stage 5.
- Bytecode files were not fuzzed with random bytes (as in stage 5), so memory safety for arbitrary input is not proven; the two crashes that input could cause and that are known are fixed above.
- A `break` or top-level `return` that reaches C does not continue into the minilang code around the host function.
- The print callback cannot call back into the VM.
- There is no ABI promise beyond `MINILANG_API_VERSION` 1; `ml_value`'s fields are visible so that it can be passed by value, not to be read.
- Hosts load bytecode only; source text still needs the Python front end (stage 7).
- The planted-bug check and the call-cost measurement were one-off; their copies and programs are not in this folder.

## Next steps

1. Stage 7: a C front end so that a host can load source text, then CI on the two platforms tested here.
2. Record the VM in each handle (a few bits of the generation) to detect a handle used with the wrong VM.
3. Measure the API against a value-stack design (Lua's) on the same VM, if call cost starts to matter.

Detailed decisions are in the [roadmap](../../research/03-design-your-own-dynamic-language.zh-TW.md) (Traditional Chinese).
