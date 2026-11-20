## Phase 3: scale (about 15 minutes)

The check now runs on every board before it is ordered. `tests/phase3_tests.cpp` generates
three boards into `data/generated/`: up to 640,000 pins, 160,000 nets and 240,000 vias. Each
test times `load_board` plus `check_board` (parsing is not timed), and each must finish
within 1.5 seconds.

- Make all phase 3 tests pass without breaking phases 1 and 2.
- Open the generated files first. One board is an LED panel wired as one long chain, one is a dense digital board with many small nets, and one has a ground plane stitched with vias.
- Measure before you optimize: the slow part may not be the code you wrote.

When you finish, or when time runs out, be ready to discuss:

- The designer edits a few traces at a time. How would you keep the result up to date without checking the whole board again?
- How would you show where a short is on the board, not just which nets it joins?
- Copper has resistance. How would you flag a power pin that is connected, but only through a long, thin path?
