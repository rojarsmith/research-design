// Variant 2 tests: anti-sniping extensions.
#include <string>
#include <vector>

#include "auction_house.hpp"
#include "check.hpp"

namespace {

using Lines = std::vector<std::string>;

void add_users(AuctionHouse& h) {
    for (const char* u : {"sam", "ann", "bob", "cat", "dan"}) h.addUser(u);
}

template <class H>
long long end_time(const H& h, const std::string& a) {
    if constexpr (requires { h.endTime(a); }) return h.endTime(a);
    else return check::missing<long long>("long long AuctionHouse::endTime(const std::string& auctionId) const");
}

template <class H>
std::string set_preference(H& h, const std::string& user, const std::string& mode) {
    if constexpr (requires { h.setPreference(user, mode); }) return h.setPreference(user, mode);
    else return check::missing<std::string>("std::string AuctionHouse::setPreference(const std::string& userId, const std::string& mode)");
}

template <class H>
void flush_digests(H& h) {
    if constexpr (requires { h.flushDigests(); }) h.flushDigests();
    else check::missing_void("void AuctionHouse::flushDigests()");
}

}  // namespace

TEST("a bid 5 or more minutes before the end does not extend", 3) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(end_time(h, a), 60LL);
    EXPECT_EQ(h.bid(a, "ann", 1000, 55), std::string("ok"));
    EXPECT_EQ(end_time(h, a), 60LL);
    EXPECT_EQ(h.inbox("sam"), (Lines{"A1 new-high 1000"}));
    EXPECT_EQ(end_time(h, "A9"), -1LL);
}

TEST("a late bid moves the end, and the watchers hear about it", 3) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.watch(a, "cat"), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 1000, 10), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 1100, 56), std::string("ok"));
    EXPECT_EQ(end_time(h, a), 61LL);
    EXPECT_EQ(h.inbox("ann"), (Lines{"A1 outbid 1100", "A1 extended 61"}));
    EXPECT_EQ(h.inbox("cat"), (Lines{"A1 new-high 1000", "A1 new-high 1100", "A1 extended 61"}));
    EXPECT_EQ(h.inbox("sam"), (Lines{"A1 new-high 1000", "A1 new-high 1100", "A1 extended 61"}));
    EXPECT_EQ(h.inbox("bob"), Lines{});
}

TEST("extensions repeat, and tick uses the current end", 3) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.bid(a, "ann", 1000, 58), std::string("ok"));  // end 63
    EXPECT_EQ(h.bid(a, "bob", 1100, 62), std::string("ok"));  // end 67
    EXPECT_EQ(end_time(h, a), 67LL);
    h.tick(66);
    EXPECT_EQ(h.status(a), std::string("open"));
    EXPECT_EQ(h.bid(a, "cat", 5000, 67), std::string("closed"));
    h.tick(67);
    EXPECT_EQ(h.status(a), std::string("sold"));
    EXPECT_EQ(h.leader(a), std::string("bob"));
    EXPECT_EQ(h.inbox("ann"), (Lines{"A1 outbid 1100", "A1 extended 67", "A1 ended 1100"}));
}

TEST("failed late bids do not extend", 3) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.bid(a, "ann", 900, 59), std::string("too-low"));
    EXPECT_EQ(end_time(h, a), 60LL);
    EXPECT_EQ(h.inbox("sam"), Lines{});
}

TEST("important users skip extended; digest users collect it", 3) {
    AuctionHouse h;
    add_users(h);
    EXPECT_EQ(set_preference(h, "cat", "important"), std::string("ok"));
    EXPECT_EQ(set_preference(h, "dan", "digest"), std::string("ok"));
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.watch(a, "cat"), std::string("ok"));
    EXPECT_EQ(h.watch(a, "dan"), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 1000, 57), std::string("ok"));
    EXPECT_EQ(h.inbox("cat"), Lines{});
    flush_digests(h);
    EXPECT_EQ(h.inbox("dan"), (Lines{"digest: A1 new-high 1000 | A1 extended 62"}));
}
