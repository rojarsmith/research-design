# gwroute

`gwroute` is the routing layer of the company's API gateway. Today it normalizes request
paths and prints a traffic summary from access logs for the on-call channel. Next it needs
to pick the upstream service for each request from the route table.

## Route table

```
# method  pattern                  upstream
route GET    /users                users
route GET    /users/:id            users
route ANY    /users/:id/orders     orders
route GET    /files/*path          files
```

- `route <METHOD> <pattern> <upstream>`. METHOD is `GET`, `POST`, `PUT`, `PATCH`, `DELETE` or `ANY`.
- A pattern is `/` followed by segments separated by `/`. The pattern `/` alone is the root.
  - A literal segment, such as `users`, is compared with the decoded path segment, case-sensitive.
  - `:name` stands for any one segment.
  - `*name` stands for the rest of the path and may only be the last segment.
  - Parameter names use lowercase letters, digits and `_`, and do not repeat within a pattern.
- Empty segments, `.` and `..` are not allowed in patterns.

## Paths

Every request target is normalized before anything looks at it (`src/pathnorm.*`):

1. Everything from the first `?` or `#` is dropped (query string and fragment).
2. The rest is split on `/`.
3. Each piece is percent-decoded (`%7E` becomes `~`). A `%` that is not followed by two hex digits makes the target invalid.
4. Empty pieces and `.` are dropped. `..` removes the previous segment; at the root it does nothing.

The result is a list of segments. An escaped slash stays inside its segment: `/files/q3%2Freport.pdf`
has two segments, `files` and `q3/report.pdf`, and is not the same path as `/files/q3/report.pdf`.

`format_path` turns segments back into a path for reports. It escapes `%`, `/`, `?`, `#`, space and any
byte outside printable ASCII as `%XX` with uppercase hex digits.

## Access log

One request per line: `<METHOD> <target> <status> <bytes> <ms>`, for example
`GET /users/42?expand=orders 200 512 12`. Lines that do not parse are counted as malformed and skipped.

## Layout

| Path | What it does |
| --- | --- |
| `src/pathnorm.*` | Normalize request targets into segments, format segments as a path |
| `src/routes.*` | Parse the route table and its patterns |
| `src/accesslog.*` | Parse access log lines |
| `src/traffic.*` | Traffic summary for the on-call channel |
| `src/router.*` | Pick the route for a request (not implemented yet) |
| `app/main.cpp` | `gwroute report <access.log>` and `gwroute route <routes> <requests>` |
| `tests/` | Tests. Run them with the drill tool or the VS Code test task. |
| `data/` | Sample route table, access log and requests |

## Conventions

- Route indices follow the order of lines in the route file.
- Methods are compared exactly: `get` is not `GET`.
- Errors in the route file throw `RouteError("line N: ...")`. Bad targets throw `PathError`.
