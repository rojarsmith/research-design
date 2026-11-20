## Phase 1: fix the bug (about 15 minutes)

The hardware team says the pin report hides unconnected pins: a prototype came back with a
floating pin on the EEPROM that the report never listed. Some tests in `tests/phase1_tests.cpp` fail.

1. Run the tests and read the failures.
2. Find the root cause and fix it. Do not change the tests.
3. Before you edit anything, say out loud which pins are missing from the report and what they have in common.
4. Also say out loud: how does `load_board` find a via by its name, and what does one lookup cost?

Suggested: do this phase without asking the AI.
