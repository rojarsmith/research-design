## The problem: seat holds for a concert venue

A small venue sells tickets online. When a customer picks seats, the seats are held for a few
minutes while they pay. Many web workers call the service **at the same time**, so it must never sell
a seat twice and never leave a hold half made.

Design the classes, then implement them in C++ behind the interface in `src/seat_holds.hpp`.

### Phase 1: hold, confirm, release, expire

**Seats.** The constructor gets the seat ids and how long a hold lasts, in seconds.

**Time.** Every call passes `now` in seconds. The service remembers the largest `now` it has seen; a
call with a smaller `now` is treated as if it came at that largest time. (Web workers' clocks differ a
little; time never goes backwards inside the service.)

**Holding.** `hold(customer, seats, now)` holds **all** the seats for the customer, or **none** of them,
and returns a hold id: `H1`, `H2`, ... unique and increasing in the order holds succeed. Errors, first
that applies:

1. `bad-request`: the list is empty or names a seat twice
2. `unknown-seat`
3. `unavailable`: a seat is held by a live hold, or sold

A hold made at time `t` expires at `t + holdSeconds`. From that moment its seats are free again.

**Confirming and releasing.** `confirm(hold, customer, now)` sells the held seats; `release(hold,
customer, now)` frees them. Both return `"ok"` or the first of:

1. `unknown-hold`
2. `not-yours`
3. `done`: the hold was already confirmed or released
4. `expired`

**Queries.** `seatState(seat, now)` returns `free`, `held`, `sold` or `unknown-seat`. `freeCount(now)`
returns the number of free seats.

**Threads.** Every method can be called from many threads at once. The tests start 8 to 16 threads
together and check that no seat is ever in two live holds, that holds are all or nothing, that ids are
unique, and that nothing deadlocks. Say out loud what your lock protects and why it cannot deadlock.

### Public interface

The tests only use `src/seat_holds.hpp`. Keep those signatures. Everything behind them is your design.
