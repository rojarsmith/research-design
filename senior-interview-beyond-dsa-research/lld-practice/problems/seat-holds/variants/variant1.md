## Variant 1: a limit per customer

"What if scalpers open twenty browser tabs and hold half the venue?"

Add one method to `SeatHolds`:

```cpp
std::string setLimit(int maxSeats);  // "ok", or "bad-limit" if maxSeats < 1. Default: no limit.
```

- A customer may have at most `maxSeats` seats in **live holds plus sold seats**. Expired and released
  holds do not count.
- A `hold` that would go over returns `limit`. The order of errors becomes: `bad-request`,
  `unknown-seat`, `limit`, `unavailable`.
- The same customer may call from many threads at once (many tabs). The limit must still hold: the
  thread test lets one customer race from 16 threads and expects exactly `maxSeats` seats to be held.

Tests: `tests/variant1_tests.cpp`.
