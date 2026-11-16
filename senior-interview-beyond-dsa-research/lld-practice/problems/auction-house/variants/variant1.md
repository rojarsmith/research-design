## Variant 1: let people choose how much they hear

"What if users can choose which notifications they get?"

Add two methods to `AuctionHouse`:

```cpp
std::string setPreference(const std::string& userId, const std::string& mode);  // "ok", "unknown-user", "unknown-mode"
void flushDigests();
```

| Mode | What reaches the inbox |
| --- | --- |
| `all` | Everything, right away. This is the default. |
| `important` | Only `outbid`, `won`, `sold` and `unsold` messages. `new-high` and `ended` are dropped. |
| `digest` | Nothing right away. Messages wait. `flushDigests()` gives each digest user with waiting messages one message: `digest: ` followed by the waiting messages joined with ` \| `, oldest first, for example `digest: A1 new-high 1000 \| A1 outbid 1100`. Users with nothing waiting get nothing. |

When a digest user switches to another mode, their waiting messages are delivered right away as one
digest message, then the new mode applies.

Who gets which message does not change; only what reaches the inbox does.

Tests: `tests/variant1_tests.cpp`.
