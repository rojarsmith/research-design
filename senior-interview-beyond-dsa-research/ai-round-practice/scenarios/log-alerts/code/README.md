# logalerts

`logalerts` reads log files from several services, merges them into one timeline,
prints a summary for the on-call channel, and (soon) evaluates alert rules.

## Log format

One event per line:

```
2026-11-15T10:00:00.480Z api ERROR E5003 upstream timeout
```

| Field | Notes |
| --- | --- |
| timestamp | ISO 8601 in UTC, ending in `Z`. The fraction is optional and services differ: `api` writes milliseconds (`.480`), `billing` writes tenths (`.5`), `worker` writes microseconds (`.700500`). |
| service | one word |
| level | `DEBUG`, `INFO`, `WARN`, `ERROR` or `FATAL` |
| code | one word, for example `E5003` |
| message | the rest of the line, may be empty |

Lines that do not parse are counted as malformed and skipped.

## Rules format

```
rule api-timeouts service=api level=ERROR code=E5003 threshold=3 window=10s cooldown=1m
```

`service` and `code` accept `*` for "any". `level` is the minimum level. Durations use `ms`, `s`, `m` or `h`.

## Layout

| Path | What it does |
| --- | --- |
| `src/timeparse.*` | Parse and format timestamps (epoch milliseconds) |
| `src/event.*` | Parse log lines and whole log files |
| `src/merge.*` | Merge several files into one timeline |
| `src/summary.*` | On-call summary text |
| `src/rules.*` | Parse alert rules |
| `src/alerts.*` | Evaluate alert rules (not implemented yet) |
| `app/main.cpp` | Command-line entry point: `logalerts <rules> <log> [<log> ...]` |
| `tests/` | Tests |
| `data/` | Sample logs and rules |

## Conventions

- Timestamps are milliseconds since 1970-01-01T00:00:00Z. Extra fraction digits beyond milliseconds are truncated.
- When two events have the same timestamp, the merged timeline keeps file order, then line order.
