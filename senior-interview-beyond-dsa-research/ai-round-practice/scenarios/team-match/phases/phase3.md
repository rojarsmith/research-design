## Phase 3: scale (about 15 minutes)

The matching now runs for the whole company's program. `tests/phase3_tests.cpp` generates
three rosters into `data/generated/`: up to 110,000 interns, 6,000 teams and 720,000
interviews. Each test times `match_interns` (parsing is not timed), and each must finish
within 1.5 seconds.

- Make all phase 3 tests pass without breaking phases 1 and 2.
- Open the generated files first. One roster has a dozen very large teams, one has thousands of small teams, and in one a thousand applicants list almost every team.
- Measure before you optimize: the slow part may not be the code you wrote.

When you finish, or when time runs out, be ready to discuss:

- This procedure gives every intern the best team they can get in any stable matching. What changes if the teams apply to the interns instead, and who would prefer that?
- A team withdraws after the results are published. How would you refill its seats while changing as few other placements as possible?
- How would you tell an intern why they did not get their first choice, using only data the tool already has?
