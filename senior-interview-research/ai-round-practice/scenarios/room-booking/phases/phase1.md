## Phase 1: fix the bug (about 15 minutes)

The front desk says `roombook check` reports conflicts that are not real, so they have
stopped trusting it. Some tests in `tests/phase1_tests.cpp` fail.

1. Run the tests and read the failures.
2. Find the root cause and fix it. Do not change the tests.
3. Before you edit anything, say out loud which reported conflicts are wrong and what they have in common.
4. Also say out loud: what is the time complexity of `find_conflicts` for one room with k bookings?

Suggested: do this phase without asking the AI.
