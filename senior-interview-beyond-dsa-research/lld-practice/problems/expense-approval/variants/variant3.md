## Variant 3: tell people when something needs them

"What if people get a notification whenever a report changes?"

Add one method to `ExpenseWorkflow`:

```cpp
std::vector<std::string> inbox(const std::string& personId) const;  // oldest first; empty if unknown
```

After every **successful** action, in this order:

1. The owner receives `"<report> is now <state>"`, unless the owner did the action.
2. If the new state waits on someone, they receive `"<report> needs you"`:
   - `submitted`: the owner's manager, then that manager's current delegate (variant 2), if any
   - `finance-review` and `approved`: every person with role `finance` except the owner, in the order
     they were added
   Skip the person who did the action. States `draft`, `rejected` and `paid` wait on nobody.

Failed actions notify nobody. Examples: when ann submits R1, bob gets `R1 needs you` and ann gets
nothing. When bob approves a small R1, ann gets `R1 is now approved`, then fay and fin each get
`R1 needs you`.

Tests: `tests/variant3_tests.cpp`.
