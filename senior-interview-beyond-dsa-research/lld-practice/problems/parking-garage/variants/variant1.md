## Variant 1: more than one way to charge

"What if the operator wants to try other pricing plans?"

Add one method to `Garage`:

```cpp
std::string setPricing(const std::string& plan);  // "ok" or "unknown-plan"
```

The plan in force when a vehicle **leaves** decides its fee. A stay of 15 minutes or less stays free
under every plan.

| Plan | Fee |
| --- | --- |
| `hourly` | Phase 1's rule. This is the default. |
| `capped` | Cut the stay into 24-hour blocks from the entry time. Each block costs the hourly price for its started hours, but **at most 2000**. A car staying 26 hours pays 2000 + 600 = 2600. |
| `evening` | A vehicle that enters at or after 18:00 and leaves by 08:00 the next morning pays a flat **800**, whatever its type. Any other stay pays `hourly`. |

Time of day is `minute % 1440`. "By 08:00 the next morning" means the exit minute is at most
`(entry day + 1) * 1440 + 480`, where the entry day is `entry minute / 1440`.

Tests: `tests/variant1_tests.cpp`.
