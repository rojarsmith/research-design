## Phase 1: fix the bug (about 15 minutes)

The release team says the nightly summary printed by `buildplan` shows critical-path
times that are too short for some manifests. Some tests in `tests/phase1_tests.cpp` fail.

1. Run the tests and read the failures.
2. Find the root cause and fix it. Do not change the tests.
3. Before you edit anything, say out loud where the bug is and why it produces exactly the wrong numbers you see.
4. Also say out loud: what is the time complexity of `topo_order`, and why does it use a heap instead of a plain queue?

Suggested: do this phase without asking the AI. Interviewers often look for how you read code you did not write.
