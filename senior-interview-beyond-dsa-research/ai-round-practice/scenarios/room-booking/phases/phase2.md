## Phase 2: assign rooms automatically (about 25 minutes)

Implement `allocate(rooms, requests)` in `src/allocator.cpp`. The tests are in `tests/phase2_tests.cpp`.

**Rules**

1. Any request with `size < 1` or `end <= start` throws `std::invalid_argument`.
2. Process requests in order of start time; equal start times go by request id (byte-wise, ascending).
3. A room *suits* a request if its capacity is at least the request size and it has every feature the request needs.
4. A room is *free* for a request if none of the bookings already assigned to it overlap `[start, end)`.
   A booking that ends at 10:00 does not block one that starts at 10:00.
5. Among suitable free rooms, pick the smallest capacity, then the fewest features, then the smallest room id.
6. If no suitable room is free, put the request on the waitlist with reason `no-room` if no room in the building
   suits it at all, or `busy` otherwise.
7. `assigned` and `waitlist` list requests in the order they were processed.

**Before you prompt the AI**, say out loud how you will know a room is free, how you will find the best room,
and the time complexity you expect. Then use the AI for the implementation, and read every line it gives you before you run it.
