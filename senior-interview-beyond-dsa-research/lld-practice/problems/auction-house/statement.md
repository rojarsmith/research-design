## The problem: an online auction house

A small marketplace runs timed auctions. Sellers open auctions, buyers bid, and everyone involved
gets notified when something happens. The product team already knows people complain about too
many notifications, so expect the notification rules to change.

Design the classes, then implement them in C++ behind the interface in `src/auction_house.hpp`.

### Phase 1: bids, watchers, closing

**Users and auctions.** `addUser(id)` registers a user. `open(seller, item, startPrice, increment,
endsAt)` opens an auction and returns `A1`, `A2`, ... in order. It returns `""` and opens nothing if the
seller is unknown or `startPrice` or `increment` is less than 1. Times are minutes; prices are cents.

**Bidding.** `bid(auction, user, amount, minute)` returns `"ok"` or the first error that applies:

1. `unknown-auction`
2. `unknown-user`
3. `closed`: the auction was closed, or `minute >= endsAt`
4. `seller-cannot-bid`
5. `too-low`: the first bid must be at least `startPrice`; every later bid at least the current
   price plus `increment`. The current leader may raise their own bid under the same rule.

A successful bid makes the bidder the leader at that amount. A failed bid changes nothing.

**Watchers.** An auction's watchers are its seller (from the start), every user who called
`watch(auction, user)`, and every user who placed a successful bid. Each user is a watcher at most once.
`watch` returns `"ok"`, `unknown-auction`, `unknown-user` or `closed`.

**Closing.** `tick(minute)` closes every open auction whose `endsAt` is at most `minute`, in the order
the auctions were opened. A closed auction's status is `sold` (it had a bid) or `unsold`.

**Notifications.** `inbox(user)` returns the messages a user received, oldest first. Each event sends
each user at most one message:

| Event | Who | Message |
| --- | --- | --- |
| a successful bid | the previous leader, if any and not the bidder | `A1 outbid 1500` |
| | every other watcher except the bidder | `A1 new-high 1500` |
| closing with a winner | the winner | `A1 won 1500` |
| | the seller | `A1 sold 1500` |
| | every other watcher | `A1 ended 1500` |
| closing without bids | the seller | `A1 unsold` |
| | every other watcher | `A1 ended unsold` |

The amount is always the auction's price after the event.

**Queries.** `leader(auction)` is the leading user (`""` if none or unknown), `price(auction)` the
current price (`0` if no bids, `-1` if unknown), and `status(auction)` is `open`, `sold`, `unsold`, or
`""` for an unknown auction.

### Public interface

The tests only use `src/auction_house.hpp`. Keep those signatures. Everything behind them is your design.
