## Phase 2: evaluate alert rules (about 25 minutes)

Implement `evaluate(timeline, rules)` in `src/alerts.cpp`. The tests are in `tests/phase2_tests.cpp`.

**Rules**

1. The timeline is already sorted by time. Walk it in order.
2. An event matches a rule when `rule_matches(rule, event)` is true (already implemented).
3. Count matching events separately for every *key*: the rule, the event's service and the event's code.
   A rule with `service=*` or `code=*` still counts each service and code on its own.
4. When a matching event at time `t` arrives, count the key's matching events with timestamps in `(t - window, t]`, including this one.
   If the count is at least `threshold` and the key is not silenced, emit an `Alert` with that count and `at_ms = t`.
5. After an alert, the key is silenced until `t + cooldown`: events with timestamps `< t + cooldown` cannot fire it again.
   Silenced events still count toward later windows.
6. Output order: for each event in timeline order, the alerts it fires, in the order the rules appear.

**Before you prompt the AI**, say out loud what you will store per key, how you will drop events that leave the window,
and the time complexity you expect. Then use the AI for the implementation, and read every line it gives you before you run it.
