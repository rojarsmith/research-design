## Variant 1: finance review for large reports

"What if large reports need a second approval from finance?"

Reports of **500000 cents ($5,000.00) or more** need finance to look at them after the manager.

- When the manager approves such a report, it moves to a new state `finance-review` instead of
  `approved`. Smaller reports go straight to `approved`, as before.
- From `finance-review`:

| Action | Moves to | Who may do it |
| --- | --- | --- |
| `approve` | `approved` | a person with role `finance` who is not the owner |
| `reject` | `rejected` | a person with role `finance` who is not the owner |
| `withdraw` | `draft` | the owner |

- Every other action from `finance-review` is `invalid-transition`. The result order, the history
  format and everything from phase 1 stay the same, for example
  `approve by bob: submitted -> finance-review`.

No new methods. Tests: `tests/variant1_tests.cpp`. Phase 1 tests must keep passing.
