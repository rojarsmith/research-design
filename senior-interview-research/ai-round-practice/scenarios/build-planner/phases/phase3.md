## Phase 3: scale (about 15 minutes)

The CI farm now runs manifests with up to 220,000 targets and hundreds of thousands of
dependency edges. `tests/phase3_tests.cpp` generates three large manifests into
`data/generated/`. Each test times `build_graph` plus `simulate` (parsing is not timed),
and each must finish within 1.5 seconds.

- Make all phase 3 tests pass without breaking phases 1 and 2.
- Open the generated files first. Their shapes differ, and the shape decides what is slow.
- Measure before you optimize: the slow part may not be the code you wrote.

When you finish, or when time runs out, be ready to discuss:

- What would change if the farm had 10,000 workers, or if workers had different speeds?
- Durations are estimates. How would you plan when a target can take 2x longer than expected?
- How would you test this scheduler beyond the cases given here?
