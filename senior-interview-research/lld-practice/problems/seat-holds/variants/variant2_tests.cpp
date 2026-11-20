// Variant 2 tests: a waitlist per seat.
#include <atomic>
#include <chrono>
#include <latch>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "check.hpp"
#include "seat_holds.hpp"

namespace {

using Ids = std::vector<std::string>;

std::vector<std::string> seat_ids(const std::string& prefix, int n) {
    std::vector<std::string> v;
    for (int i = 1; i <= n; ++i) v.push_back(prefix + std::to_string(i));
    return v;
}

template <class S>
std::string wait_for(S& s, const std::string& customer, const std::string& seat, long long now) {
    if constexpr (requires { s.waitFor(customer, seat, now); }) return s.waitFor(customer, seat, now);
    else return check::missing<std::string>("std::string SeatHolds::waitFor(const std::string& customer, const std::string& seat, long long now)");
}

template <class S>
Ids holds_of(S& s, const std::string& customer, long long now) {
    if constexpr (requires { s.holdsOf(customer, now); }) return s.holdsOf(customer, now);
    else return check::missing<Ids>("std::vector<std::string> SeatHolds::holdsOf(const std::string& customer, long long now)");
}

template <class S>
std::string set_limit(S& s, int n) {
    if constexpr (requires { s.setLimit(n); }) return s.setLimit(n);
    else return check::missing<std::string>("std::string SeatHolds::setLimit(int maxSeats)");
}

}  // namespace

TEST("a released seat goes to the first customer waiting", 3) {
    SeatHolds sh(seat_ids("A", 2), 60);
    EXPECT_EQ(sh.hold("cora", {"A1"}, 0), std::string("H1"));
    EXPECT_EQ(wait_for(sh, "dev", "A1", 1), std::string("ok"));
    EXPECT_EQ(wait_for(sh, "eve", "A1", 2), std::string("ok"));
    EXPECT_EQ(wait_for(sh, "dev", "A1", 3), std::string("ok"));  // keeps dev's place
    EXPECT_EQ(sh.release("H1", "cora", 10), std::string("ok"));
    EXPECT_EQ(holds_of(sh, "dev", 10), (Ids{"H2"}));
    EXPECT_EQ(sh.seatState("A1", 10), std::string("held"));
    EXPECT_EQ(sh.release("H2", "dev", 11), std::string("ok"));
    EXPECT_EQ(holds_of(sh, "eve", 11), (Ids{"H3"}));
    EXPECT_EQ(sh.confirm("H3", "eve", 12), std::string("ok"));
    EXPECT_EQ(sh.seatState("A1", 12), std::string("sold"));
    EXPECT_EQ(holds_of(sh, "eve", 12), Ids{});
}

TEST("an expired seat goes to the first customer waiting, starting now", 3) {
    SeatHolds sh(seat_ids("A", 2), 60);
    EXPECT_EQ(sh.hold("cora", {"A1"}, 0), std::string("H1"));
    EXPECT_EQ(wait_for(sh, "dev", "A1", 5), std::string("ok"));
    EXPECT_EQ(sh.seatState("A1", 70), std::string("held"));  // H1 expired at 60; dev's hold starts at 70
    EXPECT_EQ(holds_of(sh, "dev", 70), (Ids{"H2"}));
    EXPECT_EQ(holds_of(sh, "cora", 70), Ids{});
    EXPECT_EQ(sh.seatState("A1", 129), std::string("held"));
    EXPECT_EQ(sh.seatState("A1", 130), std::string("free"));
    EXPECT_EQ(holds_of(sh, "dev", 130), Ids{});
}

TEST("waitFor errors", 3) {
    SeatHolds sh(seat_ids("A", 3), 60);
    EXPECT_EQ(wait_for(sh, "dev", "A1", 0), std::string("free"));
    EXPECT_EQ(sh.hold("cora", {"A2"}, 0), std::string("H1"));
    EXPECT_EQ(sh.confirm("H1", "cora", 0), std::string("ok"));
    EXPECT_EQ(wait_for(sh, "dev", "A2", 0), std::string("sold"));
    EXPECT_EQ(wait_for(sh, "dev", "Z9", 0), std::string("unknown-seat"));
}

TEST("waiting customers over their limit are skipped", 3) {
    SeatHolds sh(seat_ids("A", 3), 60);
    EXPECT_EQ(set_limit(sh, 1), std::string("ok"));
    EXPECT_EQ(sh.hold("cora", {"A1"}, 0), std::string("H1"));
    EXPECT_EQ(sh.hold("dev", {"A2"}, 0), std::string("H2"));
    EXPECT_EQ(wait_for(sh, "dev", "A1", 1), std::string("ok"));
    EXPECT_EQ(wait_for(sh, "eve", "A1", 2), std::string("ok"));
    EXPECT_EQ(sh.release("H1", "cora", 3), std::string("ok"));
    EXPECT_EQ(holds_of(sh, "dev", 3), (Ids{"H2"}));
    EXPECT_EQ(holds_of(sh, "eve", 3), (Ids{"H3"}));
    EXPECT_EQ(sh.release("H3", "eve", 4), std::string("ok"));
    EXPECT_EQ(sh.seatState("A1", 4), std::string("free"));  // dev was dropped from the waitlist
}

TEST("a freed multi-seat hold hands each seat to its own waiter, in hold order", 3) {
    SeatHolds sh(seat_ids("A", 3), 60);
    EXPECT_EQ(sh.hold("cora", {"A2", "A1"}, 0), std::string("H1"));
    EXPECT_EQ(wait_for(sh, "dev", "A1", 1), std::string("ok"));
    EXPECT_EQ(wait_for(sh, "eve", "A2", 2), std::string("ok"));
    EXPECT_EQ(sh.release("H1", "cora", 3), std::string("ok"));
    EXPECT_EQ(holds_of(sh, "eve", 3), (Ids{"H2"}));
    EXPECT_EQ(holds_of(sh, "dev", 3), (Ids{"H3"}));
}

THREAD_TEST("threads: a contested seat reaches every waiter exactly once", 3) {
    const int kThreads = 8, kRounds = 40;
    for (int round = 0; round < kRounds && check::failures() == 0; ++round) {
        SeatHolds sh({"X", "Y"}, 1000000);
        std::string first = sh.hold("owner", {"X"}, 0);
        std::latch queued(kThreads + 1);
        std::vector<std::string> got(kThreads);
        std::vector<std::thread> threads;
        for (int t = 0; t < kThreads; ++t)
            threads.emplace_back([&, t] {
                check::guarded([&] {
                    std::string me = "w" + std::to_string(t);
                    if (wait_for(sh, me, "X", 0) != "ok") check::fail(__FILE__, __LINE__, me + " could not wait for X");
                    queued.arrive_and_wait();
                    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                    while (std::chrono::steady_clock::now() < deadline) {
                        Ids mine = holds_of(sh, me, 0);
                        if (!mine.empty()) {
                            got[(size_t)t] = mine.front();
                            if (sh.seatState("X", 0) != "held") check::fail(__FILE__, __LINE__, "X is not held while " + me + " holds it");
                            std::string r = sh.release(mine.front(), me, 0);
                            if (r != "ok") check::fail(__FILE__, __LINE__, "release returned " + r);
                            return;
                        }
                        std::this_thread::yield();
                    }
                    check::fail(__FILE__, __LINE__, "round " + std::to_string(round) + ": " + me + " never got seat X");
                });
            });
        queued.arrive_and_wait();
        if (sh.release(first, "owner", 0) != "ok") check::fail(__FILE__, __LINE__, "owner could not release X");
        for (auto& th : threads) th.join();
        std::set<std::string> distinct(got.begin(), got.end());
        EXPECT_EQ(distinct.size(), (size_t)kThreads);
        EXPECT_EQ(sh.seatState("X", 0), std::string("free"));
        EXPECT_EQ(sh.freeCount(0), 2);
    }
}
