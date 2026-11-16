## The problem: a multi-level parking garage

A city parking garage is replacing its ticket machines. Design and implement the core: assign a spot
when a vehicle enters, and charge it when it leaves. When you ask what is likely to change, the
operator says: "prices, and the way we assign spots. We try new ideas every few months."

Design the classes, then implement them in C++ behind the interface in `src/garage.hpp`.

### Phase 1: entering, leaving, paying

**Spots.** The garage has levels numbered from 1. Each level has some `small`, `regular` and `large`
spots. Spot ids are `<level>-<letter><number>`, with `S`, `R` or `L` for the size and numbers
starting at 1 for each size on each level: `1-S1`, `1-S2`, `1-R1`, `2-L1`, ...

**Vehicles.**

| Vehicle | Fits in | Price per started hour |
| --- | --- | --- |
| `motorcycle` | small, regular, large | 100 cents |
| `car` | regular, large | 300 cents |
| `van` | large | 500 cents |

**Entering.** `enter(plate, vehicle, minute)` assigns a spot and returns a ticket id: `T1`, `T2`, ...
in order of successful entries. The spot is chosen this way:

1. the **smallest size** that fits the vehicle and still has a free spot,
2. then the **lowest level**,
3. then the **lowest number**.

Errors, checked in this order: `bad-vehicle` (unknown vehicle type), `already-inside` (that plate
already has an active ticket), `full` (no free spot fits). A failed entry does not use a ticket id.

**Leaving.** `exit(ticket, minute)` frees the spot and returns the fee in cents, or `-1` if the ticket
is unknown or already used. The stay is `exit minute - entry minute`. A stay of 15 minutes or less is
free. Otherwise the fee is the vehicle's price times the number of **started** hours: 16 minutes is
one hour, 60 minutes is one hour, 61 minutes is two.

**Queries.** `spotOf(ticket)` returns the spot id while the vehicle is inside, else `""`.
`freeSpots(size)` returns the number of free spots of that size on all levels, or `-1` for an
unknown size.

Minutes count from the moment the garage opened, so minute 1440 is midnight at the end of day 0.

### Public interface

The tests only use `src/garage.hpp`: the `Level` struct and the `Garage` methods. Keep those
signatures. Everything behind them is your design.
