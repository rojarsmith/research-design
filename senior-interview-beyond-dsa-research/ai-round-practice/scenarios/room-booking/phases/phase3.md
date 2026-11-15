## Phase 3: scale (about 15 minutes)

The company is rolling `roombook` out to every office at once. `tests/phase3_tests.cpp`
generates three days of rooms and requests into `data/generated/`: 5,000 to 12,000 rooms
and 150,000 to 200,000 requests. Each test times `allocate` (parsing is not timed), and each
must finish within 1 second.

- Make all phase 3 tests pass without breaking phases 1 and 2.
- Open the generated files first. One day has little contention, one has many requests no room can serve, one is a morning rush.
- Measure before you optimize, and check which test fails: each one stresses a different part of an allocator.

When you finish, or when time runs out, be ready to discuss:

- Requests now arrive one at a time during the day, and people cancel. What changes?
- How would you offer the waitlisted person the nearest free time instead of just "busy"?
- Two offices share some rooms. How would you avoid assigning the same room twice when both run at the same time?
