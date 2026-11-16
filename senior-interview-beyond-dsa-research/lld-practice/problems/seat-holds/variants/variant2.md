## Variant 2: a waitlist per seat

"What if people could queue for a seat someone else is holding?"

Add two methods to `SeatHolds`:

```cpp
std::string waitFor(const std::string& customer, const std::string& seat, long long now);
std::vector<std::string> holdsOf(const std::string& customer, long long now);  // live hold ids, increasing
```

- `waitFor` returns `"ok"`, or `unknown-seat`, `free` (nobody holds it: hold it instead) or `sold`.
  Waiting again for the same seat keeps your place.
- When a held seat becomes free, because its hold was **released** or **expired**, it goes straight to
  the first customer waiting for it, as a new one-seat hold. The new hold starts at the current time
  (the largest `now` seen), gets the next hold id, and expires like any other hold.
- When a hold with several seats is freed, its seats are handed over in the order they were listed in
  that hold.
- A waiting customer who would go over their limit (variant 1) is skipped and removed from that
  seat's waitlist; the seat goes to the next one.
- `holdsOf` shows a customer the holds they own, including ones they got from the waitlist.

Expiry is noticed whenever any call arrives with a time at or past it. Make sure every way a seat can
become free leads to the same hand-over.

Tests: `tests/variant2_tests.cpp`.
