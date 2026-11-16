## The problem: expense report approvals

A 400-person company handles expense reports over email. You are building the approval workflow
behind a new internal tool. The finance team already has a list of changes for next quarter, so
expect the rules to change after the first version ships.

Design the classes, then implement them in C++ behind the interface in `src/expense_workflow.hpp`.

### Phase 1: the core workflow

**People.** Each person has an id, a role (`employee` or `finance`) and an optional manager (another
person's id, or `""`). Finance people can own reports too.

**Reports.** `createReport(owner, amountCents)` creates a report in state `draft` and returns its id:
`R1`, `R2`, ... in creation order. It returns `""` and creates nothing if the owner is unknown or the
amount is not positive (a failed call does not use up an id).

**Actions.**

| Action | Allowed from | Moves to | Who may do it |
| --- | --- | --- | --- |
| `submit` | `draft`, `rejected` | `submitted` | the owner |
| `withdraw` | `submitted` | `draft` | the owner |
| `approve` | `submitted` | `approved` | the owner's manager |
| `reject` | `submitted` | `rejected` | the owner's manager |
| `pay` | `approved` | `paid` | a person with role `finance` who is not the owner |

**Results.** Every action returns `"ok"` or one of these codes. If several apply, return the first
one in this list:

1. `unknown-report`: no report has that id
2. `unknown-person`: the actor is not a registered person
3. `invalid-transition`: the action is not possible from the report's current state
4. `not-allowed`: the actor may not perform this action on this report

A failed action changes nothing.

**History.** Each successful action appends one line to the report's history:
`"<action> by <actor>: <from> -> <to>"`. A `reject` with a non-empty reason appends
` (<reason>)`. Example: `reject by bob: submitted -> rejected (missing receipt)`.

`state(id)` returns the current state, or `""` for an unknown report. `history(id)` returns the
lines oldest first, or an empty list for an unknown report.

Nobody in the tests is their own manager. A person who has no manager has nobody who can approve or
reject their reports.

### Public interface

The tests only call the methods in `src/expense_workflow.hpp`. Keep those signatures. Everything
behind them is your design: add classes and files under `src/` as you see fit. Write a short demo
in `app/main.cpp` if you are in machine-coding mode.
