## Variant 2: no more sniping

"What if people keep winning by bidding in the last second?"

Add one method to `AuctionHouse`:

```cpp
long long endTime(const std::string& auctionId) const;  // the current end minute; -1 if unknown
```

- If a bid is accepted **less than 5 minutes before the end** (`endsAt - minute < 5`), the end moves
  to `minute + 5`. This can happen any number of times.
- After the bid's own messages, every watcher except the bidder gets `A1 extended <new end>`, for
  example `A1 extended 61`. The previous leader gets this too, after their `outbid` message.
- `extended` is not an important message (variant 1): `important` users do not get it, and `digest`
  users collect it like any other message.
- Bids and `tick` use the current end.

Tests: `tests/variant2_tests.cpp`.
