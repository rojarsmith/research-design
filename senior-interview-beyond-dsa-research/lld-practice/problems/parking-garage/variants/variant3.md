## Variant 3: EV charging

"What if some spots get chargers?"

Add one method to `Garage`:

```cpp
std::string addChargers(int level, const std::string& size, int count);  // "ok" or "bad-spot"
```

- It turns the **last** `count` spots of that size on that level into charging spots. With 4 regular
  spots on level 1, `addChargers(1, "regular", 2)` equips `1-R3` and `1-R4`. It returns `bad-spot` if
  the level does not exist, the size is unknown, or `count` is less than 1 or more than the number of
  such spots. It is called before any vehicle enters, at most once per level and size.
- Two new vehicle types: `ev-car` fits and is priced like `car`, and `ev-van` like `van`.

**Placement.** Split the free spots that fit the vehicle into two groups:

- An EV prefers the charging spots. If none of them fits and is free, it uses the other spots.
- Any other vehicle prefers the spots without a charger, and uses a charging spot only when nothing else
  fits.

Within the chosen group, apply the allocation policy from variant 2 (`lowest-level` or `spread`) as if
the garage had only that group's spots.

**Fee.** The pricing plan's fee for the vehicle (an `ev-car` is priced as a `car` under every plan),
**plus 200 cents per started hour** when an EV was parked on a charging spot. A stay of 15 minutes or
less is still entirely free. Non-EVs on a charging spot, and EVs on a normal spot, pay no charging fee.

`freeSpots(size)` still counts all spots of that size, with or without a charger.

Tests: `tests/variant3_tests.cpp`.
