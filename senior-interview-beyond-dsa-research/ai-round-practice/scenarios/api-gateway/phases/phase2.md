## Phase 2: route requests (about 25 minutes)

Implement `build_table(routes)` and `route_request(table, method, target)` in `src/router.cpp`.
You may add fields to `RouteTable`. The tests are in `tests/phase2_tests.cpp`.

**Rules**

1. Normalize the target with `normalize_path`. If it throws, the result is status 400.
2. A route *matches the path* when its segments line up with the path's segments:
   - a literal matches one segment that is exactly equal
   - `:name` matches any one segment
   - `*name` matches one or more remaining segments, so `/files/*path` does not match `/files`
   - the pattern `/` matches only the empty path
3. A route *allows the method* if its method equals the request method or is `ANY`. Methods are case-sensitive.
4. Among the routes that match the path and allow the method, pick the most specific:
   - Compare two patterns segment by segment from the left. At the first position where the kinds differ,
     a literal beats `:name`, which beats `*name`.
   - If two patterns have the same shape, the route with the exact method beats the `ANY` route.
5. Status 200: `route` is that route's index and `params` lists every `:name` and `*name` in pattern order.
   A parameter's value is its decoded segment; a wildcard's value is the remaining segments joined with `/`.
6. If at least one route matches the path but none allows the method: status 405, and `allow` lists the
   methods of the routes that match the path, sorted, without repeats.
7. Otherwise: status 404.
8. `build_table` throws `RouteError` if two routes have the same method and the same shape: the same
   segment kinds and the same literals, ignoring parameter names.

**Before you prompt the AI**, say out loud which data structure you will use to find the most specific route,
and the time complexity you expect per request. Then use the AI for the implementation, and read every line
it gives you before you run it.
