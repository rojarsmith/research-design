## Phase 1: fix the bug (about 15 minutes)

On-call engineers say the merged timeline from `logalerts` is out of order: billing events
show up before api events that happened earlier, and worker events land minutes in the
future. Some tests in `tests/phase1_tests.cpp` fail.

1. Run the tests and read the failures.
2. Find the root cause and fix it. Do not change the tests.
3. Before you edit anything, say out loud which input produces the wrong value and what value it produces.
4. Also say out loud: what is the time complexity of `merge_logs` today, and when would it matter?

Suggested: do this phase without asking the AI.
