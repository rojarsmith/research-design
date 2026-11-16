## Phase 3: scale (about 15 minutes)

Every team's API is moving behind `gwroute`. `tests/phase3_tests.cpp` generates three route
tables and their traffic into `data/generated/`: up to 40,400 routes and 300,000 requests.
Each test times `build_table` plus `route_request` for every request (parsing is not timed),
and each must finish within 1.5 seconds.

- Make all phase 3 tests pass without breaking phases 1 and 2.
- Open the generated files first. One table spreads its routes over 400 services, one puts 25,000 routes under a single prefix, and one day of traffic includes a vulnerability scanner.
- Measure before you optimize: the slow part may not be the code you wrote.

When you finish, or when time runs out, be ready to discuss:

- Teams change their routes many times a day. How would you swap in a new table without dropping requests?
- How would you route on the host name or a header as well as on the path?
- How would you check that a new route does not quietly take traffic away from an existing one?
