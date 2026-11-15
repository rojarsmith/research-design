# buildplan

`buildplan` reads a build manifest and prints a build plan for the CI farm: the order
targets can be built in, how long the longest dependency chain takes, and (soon) how long
a build takes on a given number of parallel workers.

## Manifest format

```
# comments start with '#'
target core     300ms
target util     1.5s    deps=core
target app      2m      deps=util,core
```

- `target <name> <duration> [deps=<name>,<name>,...]`
- Names are unique. Every dependency must be declared somewhere in the file (order does not matter).
- Durations: a number followed by `ms`, `s`, `m` or `h`. Parts can be combined, for example `1m30s`.
  Fractions are allowed (`1.5s`, `0.25h`). Values are rounded to the nearest millisecond.

## Layout

| Path | What it does |
| --- | --- |
| `src/duration.*` | Parse and format durations |
| `src/manifest.*` | Parse manifest text into `Manifest` |
| `src/graph.*` | Build the dependency graph, detect cycles, topological order, critical path |
| `src/report.*` | Human-readable plan report |
| `src/scheduler.*` | Parallel build simulation (not implemented yet) |
| `app/main.cpp` | Command-line entry point |
| `tests/` | Tests. Run them with the drill tool or the VS Code test task. |
| `data/` | Sample manifests |

## Conventions

- Target indices follow the order targets appear in the manifest.
- When several targets could go next, ties are broken by name (byte-wise, ascending).
- Errors in input throw `ManifestError` or `GraphError` with a message that names the line or target.
