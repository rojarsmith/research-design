## Phase 3: scale (about 15 minutes)

Production traffic is far larger than the samples. `tests/phase3_tests.cpp` generates three
sets of logs into `data/generated/`, about 400,000 events each. Each test times
`merge_logs` plus `evaluate` (parsing is not timed), and each must finish within 1.5 seconds.

- Make all phase 3 tests pass without breaking phases 1 and 2.
- Open the generated files first. One has a single very hot key, one has tens of thousands of keys, one comes as four interleaved files.
- Measure before you optimize: the slow part may not be the code you wrote.

When you finish, or when time runs out, be ready to discuss:

- Logs now arrive as a stream, slightly out of order (up to 5 seconds late). What changes?
- Memory grows with the number of keys. How would you bound it?
- How would you run this across many machines, and where would the state for each key live?
