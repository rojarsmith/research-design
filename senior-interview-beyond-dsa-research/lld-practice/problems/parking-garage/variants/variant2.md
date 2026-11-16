## Variant 2: spread the cars out

"What if the lowest level gets crowded and the operator wants to spread vehicles over the levels?"

Add one method to `Garage`:

```cpp
std::string setAllocation(const std::string& policy);  // "ok" or "unknown-policy"
```

| Policy | How a spot is chosen |
| --- | --- |
| `lowest-level` | Phase 1's rule. This is the default. |
| `spread` | Choose the **size** exactly as before (the smallest size that fits and has a free spot). Then choose the **level with the most free spots of that size** (ties: the lower level). Then the lowest number on that level. |

The policy applies to vehicles that enter after it is set. Fees and everything else stay the same.

Tests: `tests/variant2_tests.cpp`.
