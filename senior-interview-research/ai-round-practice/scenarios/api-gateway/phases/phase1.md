## Phase 1: fix the bug (about 15 minutes)

The on-call channel says the gateway's traffic report cannot be trusted: downloads show up
under a path nobody requested, some paths look cut short, and a search request is counted
as an invalid target although it looks fine. Some tests in `tests/phase1_tests.cpp` fail.

1. Run the tests and read the failures.
2. Find the root cause and fix it. Do not change the tests.
3. Before you edit anything, say out loud which targets come out wrong and what they have in common.
4. Also say out loud: what is the time complexity of `normalize_path` for a target of length L, and what kind of target makes it slow?

Suggested: do this phase without asking the AI. Interviewers often look for how you read code you did not write.
