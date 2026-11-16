## Variant 3: buy it now

"What if a seller can name a price that ends the auction at once?"

Add two methods to `AuctionHouse`:

```cpp
std::string setBuyNow(const std::string& auctionId, long long price);
std::string buyNow(const std::string& auctionId, const std::string& userId, long long minute);
```

- `setBuyNow` returns `"ok"`, or the first of: `unknown-auction`, `closed`, `has-bids` (someone already
  bid), `too-low` (the price is below the start price). Calling it again changes the price.
- `buyNow` returns `"ok"`, or the first of: `unknown-auction`, `unknown-user`, `closed` (as for a bid),
  `seller-cannot-bid`, `no-buy-now` (no buy-now price was set, or a bid has been placed since).
- A successful `buyNow` closes the auction **at once**: status `sold`, the buyer is the leader, and the
  price is the buy-now price. The messages are exactly those of a close with a winner: the buyer gets
  `won`, the seller `sold`, every other watcher `ended`, and preferences (variant 1) apply as usual.
- After that, bids and `buyNow` return `closed`, and `tick` sends nothing more for this auction.

Tests: `tests/variant3_tests.cpp`.
