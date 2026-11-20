// Variant 1 tests: notification preferences.
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

TEST("important: only outbid, won, sold and unsold", 2) {
    AuctionHouse h;
    add_users(h);
    EXPECT_EQ(set_preference(h, "cat", "important"), std::string("ok"));
    EXPECT_EQ(set_preference(h, "ann", "important"), std::string("ok"));
    EXPECT_EQ(set_preference(h, "sam", "important"), std::string("ok"));
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.watch(a, "cat"), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 1000, 1), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 1100, 2), std::string("ok"));
    h.tick(60);
    EXPECT_EQ(h.inbox("cat"), Lines{});
    EXPECT_EQ(h.inbox("ann"), (Lines{"A1 outbid 1100"}));
    EXPECT_EQ(h.inbox("sam"), (Lines{"A1 sold 1100"}));
    EXPECT_EQ(h.inbox("bob"), (Lines{"A1 won 1100"}));  // bob kept the default
}

TEST("digest: messages wait for flushDigests", 2) {
    AuctionHouse h;
    add_users(h);
    EXPECT_EQ(set_preference(h, "dan", "digest"), std::string("ok"));
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.watch(a, "dan"), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 1000, 1), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 1100, 2), std::string("ok"));
    EXPECT_EQ(h.inbox("dan"), Lines{});
    flush_digests(h);
    EXPECT_EQ(h.inbox("dan"), (Lines{"digest: A1 new-high 1000 | A1 new-high 1100"}));
    flush_digests(h);  // nothing new is waiting
    EXPECT_EQ(h.inbox("dan"), (Lines{"digest: A1 new-high 1000 | A1 new-high 1100"}));
    EXPECT_EQ(h.inbox("ann"), (Lines{"A1 outbid 1100"}));
}

TEST("a digest can hold messages from several auctions and kinds", 2) {
    AuctionHouse h;
    add_users(h);
    EXPECT_EQ(set_preference(h, "ann", "digest"), std::string("ok"));
    std::string a1 = h.open("sam", "lamp", 1000, 100, 60);
    std::string a2 = h.open("sam", "vase", 500, 50, 30);
    EXPECT_EQ(h.bid(a1, "ann", 1000, 1), std::string("ok"));
    EXPECT_EQ(h.bid(a2, "ann", 500, 2), std::string("ok"));
    EXPECT_EQ(h.bid(a1, "bob", 1100, 3), std::string("ok"));
    h.tick(40);
    flush_digests(h);
    EXPECT_EQ(h.inbox("ann"), (Lines{"digest: A1 outbid 1100 | A2 won 500"}));
}

TEST("switching away from digest delivers what is waiting", 2) {
    AuctionHouse h;
    add_users(h);
    EXPECT_EQ(set_preference(h, "dan", "digest"), std::string("ok"));
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.watch(a, "dan"), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 1000, 1), std::string("ok"));
    EXPECT_EQ(set_preference(h, "dan", "all"), std::string("ok"));
    EXPECT_EQ(h.inbox("dan"), (Lines{"digest: A1 new-high 1000"}));
    EXPECT_EQ(h.bid(a, "bob", 1100, 2), std::string("ok"));
    EXPECT_EQ(h.inbox("dan"), (Lines{"digest: A1 new-high 1000", "A1 new-high 1100"}));
    EXPECT_EQ(set_preference(h, "dan", "digest"), std::string("ok"));
    EXPECT_EQ(set_preference(h, "dan", "important"), std::string("ok"));  // nothing waiting: nothing sent
    EXPECT_EQ(h.inbox("dan").size(), (size_t)2);
}

TEST("preference errors", 2) {
    AuctionHouse h;
    add_users(h);
    EXPECT_EQ(set_preference(h, "zed", "all"), std::string("unknown-user"));
    EXPECT_EQ(set_preference(h, "ann", "loud"), std::string("unknown-mode"));
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.bid(a, "ann", 1000, 1), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 1100, 2), std::string("ok"));
    EXPECT_EQ(h.inbox("ann"), (Lines{"A1 outbid 1100"}));
}
