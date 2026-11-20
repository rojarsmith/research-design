## Phase 2: simulate a parallel build (about 25 minutes)

Implement `simulate(g, workers)` in `src/scheduler.cpp`. The tests are in `tests/phase2_tests.cpp`.

**Rules**

1. `workers < 1` throws `std::invalid_argument`. A graph with a cycle throws `GraphError`.
2. A target can start once all of its dependencies have finished. It occupies one worker for `duration_ms`.
3. Priority: a target's *tail* is its own duration plus the largest tail among the targets that depend on it (0 if none).
   Higher tail goes first. Equal tails: the smaller name goes first.
4. Time jumps from event to event. At each moment `t`, repeat until nothing changes:
   1. every target that finishes at `t` frees its worker, which may make other targets ready;
   2. free workers take ready targets, highest priority first.

   A target with duration 0 starts and finishes at the same `t`, so its dependents can start at that same `t` if a worker is free.
5. Fill in `Schedule`:
   - `start_ms[i]`: when target `i` starts
   - `order`: target indices in the order they were started (within one moment, the order you assigned them)
   - `makespan_ms`: the latest finish time, or 0 for an empty graph

**Before you prompt the AI**, say out loud which data structures you will use and the time complexity you expect.
Then use the AI for the implementation, and read every line it gives you before you run it.
