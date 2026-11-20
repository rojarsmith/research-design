// Variant 3 tests: buy it now.
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
std::string set_buy_now(H& h, const std::string& a, long long price) {
    if constexpr (requires { h.setBuyNow(a, price); }) return h.setBuyNow(a, price);
    else return check::missing<std::string>("std::string AuctionHouse::setBuyNow(const std::string& auctionId, long long price)");
}

template <class H>
std::string buy_now(H& h, const std::string& a, const std::string& user, long long minute) {
    if constexpr (requires { h.buyNow(a, user, minute); }) return h.buyNow(a, user, minute);
    else return check::missing<std::string>("std::string AuctionHouse::buyNow(const std::string& auctionId, const std::string& userId, long long minute)");
}

template <class H>
std::string set_preference(H& h, const std::string& user, const std::string& mode) {
    if constexpr (requires { h.setPreference(user, mode); }) return h.setPreference(user, mode);
    else return check::missing<std::string>("std::string AuctionHouse::setPreference(const std::string& userId, const std::string& mode)");
}

}  // namespace

TEST("buying now closes the auction at once", 4) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(set_buy_now(h, a, 3000), std::string("ok"));
    EXPECT_EQ(h.watch(a, "cat"), std::string("ok"));
    EXPECT_EQ(buy_now(h, a, "ann", 5), std::string("ok"));
    EXPECT_EQ(h.status(a), std::string("sold"));
    EXPECT_EQ(h.leader(a), std::string("ann"));
    EXPECT_EQ(h.price(a), 3000LL);
    EXPECT_EQ(h.inbox("ann"), (Lines{"A1 won 3000"}));
    EXPECT_EQ(h.inbox("sam"), (Lines{"A1 sold 3000"}));
    EXPECT_EQ(h.inbox("cat"), (Lines{"A1 ended 3000"}));
    EXPECT_EQ(h.bid(a, "bob", 9000, 6), std::string("closed"));
    EXPECT_EQ(buy_now(h, a, "bob", 6), std::string("closed"));
    h.tick(100);
    EXPECT_EQ(h.inbox("cat"), (Lines{"A1 ended 3000"}));
}

TEST("buyNow errors, in order", 4) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(buy_now(h, "A9", "zed", 5), std::string("unknown-auction"));
    EXPECT_EQ(buy_now(h, a, "zed", 60), std::string("unknown-user"));
    EXPECT_EQ(buy_now(h, a, "sam", 60), std::string("closed"));
    EXPECT_EQ(buy_now(h, a, "sam", 5), std::string("seller-cannot-bid"));
    EXPECT_EQ(buy_now(h, a, "ann", 5), std::string("no-buy-now"));
    EXPECT_EQ(set_buy_now(h, a, 3000), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 1000, 6), std::string("ok"));
    EXPECT_EQ(buy_now(h, a, "ann", 7), std::string("no-buy-now"));
    EXPECT_EQ(h.status(a), std::string("open"));
}

TEST("setBuyNow errors, in order", 4) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(set_buy_now(h, "A9", 3000), std::string("unknown-auction"));
    EXPECT_EQ(set_buy_now(h, a, 999), std::string("too-low"));
    EXPECT_EQ(set_buy_now(h, a, 1000), std::string("ok"));
    EXPECT_EQ(set_buy_now(h, a, 4000), std::string("ok"));
    EXPECT_EQ(h.bid(a, "bob", 1000, 6), std::string("ok"));
    EXPECT_EQ(set_buy_now(h, a, 999), std::string("has-bids"));
    h.tick(60);
    EXPECT_EQ(set_buy_now(h, a, 5000), std::string("closed"));
}

TEST("the latest buy-now price is the one paid", 4) {
    AuctionHouse h;
    add_users(h);
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(set_buy_now(h, a, 3000), std::string("ok"));
    EXPECT_EQ(set_buy_now(h, a, 2500), std::string("ok"));
    EXPECT_EQ(buy_now(h, a, "bob", 30), std::string("ok"));
    EXPECT_EQ(h.price(a), 2500LL);
    EXPECT_EQ(h.inbox("bob"), (Lines{"A1 won 2500"}));
}

TEST("preferences apply to buy-now messages", 4) {
    AuctionHouse h;
    add_users(h);
    EXPECT_EQ(set_preference(h, "cat", "important"), std::string("ok"));
    std::string a = h.open("sam", "lamp", 1000, 100, 60);
    EXPECT_EQ(h.watch(a, "cat"), std::string("ok"));
    EXPECT_EQ(h.watch(a, "dan"), std::string("ok"));
    EXPECT_EQ(set_buy_now(h, a, 3000), std::string("ok"));
    EXPECT_EQ(buy_now(h, a, "ann", 5), std::string("ok"));
    EXPECT_EQ(h.inbox("cat"), Lines{});
    EXPECT_EQ(h.inbox("dan"), (Lines{"A1 ended 3000"}));
}
