// Phase 1 tests: bids, watchers, closing. They only use the public interface in auction_house.hpp.
#include <string>
#include <vector>

#include "auction_house.hpp"
#include "check.hpp"

namespace {

using Lines = std::vector<std::string>;

void add_users(AuctionHouse& h) {
    for (const char* u : {"sam", "ann", "bob", "cat", "dan"}) h.addUser(u);
}

}  // namespace

TEST("opening auctions", 1) {
    AuctionHouse h;
    add_users(h);
    EXPECT_EQ(h.open("sam", "lamp", 1000, 100, 60), std::string("A1"));
    EXPECT_EQ(h.open("zed", "lamp", 1000, 100, 60), std::string(""));
    EXPECT_EQ(h.open("sam", "lamp", 0, 100, 60), std::string(""));
    EXPECT_EQ(h.open("sam", "lamp", 1000, 0, 60), std::string(""));
    EXPECT_EQ(h.open("ann", "vase", 500, 50, 90), std::string("A2"));
    EXPECT_EQ(h.status("A1"), std::string("open"));
    EXPECT_EQ(h.leader("A1"), std::string(""));
    EXPECT_EQ(h.price("A1"), 0LL);
    EXPECT_EQ(h.status("A9"), std::string(""));
    EXPECT_EQ(h.price("A9"), -1LL);
    EXPECT_EQ(h.leader("A9"), std::string(""));
}

TEST("the start price, the increment, and raising your own bid", 1) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.bid(a, "ann", 900, 1), std::string("too-low"));
    EXPECT_EQ(h.bid(a, "ann", 1000, 1), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 1050, 2), std::string("too-low"));
    EXPECT_EQ(h.bid(a, "bob", 1100, 2), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 1250, 3), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 1300, 4), std::string("too-low"));
    EXPECT_EQ(h.bid(a, "ann", 1350, 4), std::string("ok"));
    EXPECT_EQ(h.leader(a), std::string("ann"));
    EXPECT_EQ(h.price(a), 1350LL);
}

TEST("bid errors, in order", 1) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.bid("A9", "zed", 1000, 1), std::string("unknown-auction"));
    EXPECT_EQ(h.bid(a, "zed", 1000, 60), std::string("unknown-user"));
    EXPECT_EQ(h.bid(a, "sam", 1, 60), std::string("closed"));
    EXPECT_EQ(h.bid(a, "sam", 1, 10), std::string("seller-cannot-bid"));
    EXPECT_EQ(h.bid(a, "ann", 1000, 10), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 5000, 60), std::string("closed"));
    h.tick(30);  // not yet
    EXPECT_EQ(h.status(a), std::string("open"));
    h.tick(60);
    EXPECT_EQ(h.bid(a, "bob", 5000, 20), std::string("closed"));
    EXPECT_EQ(h.price(a), 1000LL);
}

TEST("bids notify the previous leader and the other watchers", 1) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.bid(a, "ann", 1000, 1), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 1100, 2), std::string("ok"));
    EXPECT_EQ(h.watch(a, "cat"), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 1200, 3), std::string("ok"));
    EXPECT_EQ(h.inbox("sam"), (Lines{"A1 new-high 1000", "A1 new-high 1100", "A1 new-high 1200"}));
    EXPECT_EQ(h.inbox("ann"), (Lines{"A1 outbid 1100"}));
    EXPECT_EQ(h.inbox("bob"), (Lines{"A1 outbid 1200"}));
    EXPECT_EQ(h.inbox("cat"), (Lines{"A1 new-high 1200"}));
    EXPECT_EQ(h.inbox("dan"), Lines{});
}

TEST("the leader raising their own bid is not outbid", 1) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.bid(a, "ann", 1000, 1), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 1100, 2), std::string("ok"));
    EXPECT_EQ(h.inbox("ann"), Lines{});
    EXPECT_EQ(h.inbox("sam"), (Lines{"A1 new-high 1000", "A1 new-high 1100"}));
}

TEST("closing with a winner", 1) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.bid(a, "ann", 1000, 1), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 1100, 2), std::string("ok"));
    EXPECT_EQ(h.watch(a, "cat"), std::string("ok"));
    h.tick(59);
    EXPECT_EQ(h.status(a), std::string("open"));
    h.tick(75);
    EXPECT_EQ(h.status(a), std::string("sold"));
    EXPECT_EQ(h.leader(a), std::string("bob"));
    EXPECT_EQ(h.price(a), 1100LL);
    EXPECT_EQ(h.inbox("bob"), (Lines{"A1 won 1100"}));
    EXPECT_EQ(h.inbox("sam"), (Lines{"A1 new-high 1000", "A1 new-high 1100", "A1 sold 1100"}));
    EXPECT_EQ(h.inbox("ann"), (Lines{"A1 outbid 1100", "A1 ended 1100"}));
    EXPECT_EQ(h.inbox("cat"), (Lines{"A1 ended 1100"}));
    h.tick(200);  // closes only once
    EXPECT_EQ(h.inbox("cat"), (Lines{"A1 ended 1100"}));
}

TEST("closing without bids", 1) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.watch(a, "cat"), std::string("ok"));
    h.tick(100);
    EXPECT_EQ(h.status(a), std::string("unsold"));
    EXPECT_EQ(h.leader(a), std::string(""));
    EXPECT_EQ(h.price(a), 0LL);
    EXPECT_EQ(h.inbox("sam"), (Lines{"A1 unsold"}));
    EXPECT_EQ(h.inbox("cat"), (Lines{"A1 ended unsold"}));
}

TEST("tick closes auctions in the order they were opened", 1) {
    AuctionHouse h;
    add_users(h);
    std::string a1 = h.open("sam", "lamp", 1000, 100, 50);
    std::string a2 = h.open("sam", "vase", 500, 50, 40);
    EXPECT_EQ(h.watch(a2, "ann"), std::string("ok"));
    EXPECT_EQ(h.watch(a1, "ann"), std::string("ok"));
    h.tick(60);
    EXPECT_EQ(h.inbox("ann"), (Lines{"A1 ended unsold", "A2 ended unsold"}));
    EXPECT_EQ(h.inbox("sam"), (Lines{"A1 unsold", "A2 unsold"}));
}

TEST("watching: errors, and no user twice", 1) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.watch("A9", "ann"), std::string("unknown-auction"));
    EXPECT_EQ(h.watch(a, "zed"), std::string("unknown-user"));
    EXPECT_EQ(h.watch(a, "ann"), std::string("ok"));
    EXPECT_EQ(h.watch(a, "ann"), std::string("ok"));
    EXPECT_EQ(h.watch(a, "sam"), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 1000, 1), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 1100, 2), std::string("ok"));
    EXPECT_EQ(h.inbox("ann"), (Lines{"A1 outbid 1100"}));
    EXPECT_EQ(h.inbox("sam"), (Lines{"A1 new-high 1000", "A1 new-high 1100"}));
    h.tick(60);
    EXPECT_EQ(h.watch(a, "cat"), std::string("closed"));
}

TEST("a failed bid sends nothing", 1) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.watch(a, "cat"), std::string("ok"));
    EXPECT_EQ(h.bid(a, "ann", 999, 1), std::string("too-low"));
    EXPECT_EQ(h.bid(a, "sam", 2000, 1), std::string("seller-cannot-bid"));
    EXPECT_EQ(h.inbox("sam"), Lines{});
    EXPECT_EQ(h.inbox("cat"), Lines{});
    EXPECT_EQ(h.inbox("ann"), Lines{});
}
