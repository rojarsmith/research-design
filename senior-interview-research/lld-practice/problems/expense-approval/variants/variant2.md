## Variant 2: delegation while a manager is away

"What if a manager goes on vacation?"

Add two methods to `ExpenseWorkflow`:

```cpp
std::string delegate(const std::string& managerId, const std::string& delegateId);
std::string endDelegation(const std::string& managerId);
```

- After `delegate(bob, carl)`, carl may do everything bob may do **as a manager**: approve and reject
  the reports of bob's direct reports. Bob can still act himself.
- A delegate acts in their own name, and the history records both:
  `approve by carl for bob: submitted -> approved`. When bob acts himself, the line is unchanged.
- One delegate per manager: a new `delegate` call replaces the previous one.
  `endDelegation(bob)` ends it (`"ok"` even if bob had no delegate).
- Delegation is not transitive. If bob delegates to carl and carl delegates to dana, dana cannot act
  for bob.
- Nobody may approve or reject their own report, delegate or not.
- Only manager permissions are delegated. Finance permissions (finance review, `pay`) are not.
- Results: `"unknown-person"` if either id is unknown; `"not-allowed"` for `delegate(x, x)`;
  otherwise `"ok"`.

Tests: `tests/variant2_tests.cpp`. Until both methods exist, these tests fail with a "missing from your
facade" message instead of a compile error.
