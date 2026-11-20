// Variant 1 tests: a seat limit per customer.
#include <atomic>
#include <latch>
#include <string>
#include <thread>
#include <vector>

#include "check.hpp"
#include "seat_holds.hpp"

namespace {

std::vector<std::string> seat_ids(const std::string& prefix, int n) {
    std::vector<std::string> v;
    for (int i = 1; i <= n; ++i) v.push_back(prefix + std::to_string(i));
    return v;
}

template <class S>
std::string set_limit(S& s, int n) {
    if constexpr (requires { s.setLimit(n); }) return s.setLimit(n);
    else return check::missing<std::string>("std::string SeatHolds::setLimit(int maxSeats)");
}

}  // namespace

TEST("the limit counts live holds and sold seats", 2) {
    SeatHolds sh(seat_ids("A", 8), 60);
    EXPECT_EQ(set_limit(sh, 4), std::string("ok"));
    EXPECT_EQ(sh.hold("cora", {"A1", "A2", "A3"}, 0), std::string("H1"));
    EXPECT_EQ(sh.hold("cora", {"A4", "A5"}, 0), std::string("limit"));
    EXPECT_EQ(sh.hold("cora", {"A4"}, 0), std::string("H2"));
    EXPECT_EQ(sh.confirm("H1", "cora", 1), std::string("ok"));
    EXPECT_EQ(sh.hold("cora", {"A5"}, 1), std::string("limit"));
    EXPECT_EQ(sh.release("H2", "cora", 2), std::string("ok"));
    EXPECT_EQ(sh.hold("cora", {"A5"}, 2), std::string("H3"));
    EXPECT_EQ(sh.hold("cora", {"A6"}, 3), std::string("limit"));
    EXPECT_EQ(sh.hold("dev", {"A6", "A7", "A8"}, 3), std::string("H4"));  // other customers are not affected
}

TEST("expired holds do not count", 2) {
    SeatHolds sh(seat_ids("A", 4), 60);
    EXPECT_EQ(set_limit(sh, 2), std::string("ok"));
    EXPECT_EQ(sh.hold("cora", {"A1", "A2"}, 0), std::string("H1"));
    EXPECT_EQ(sh.hold("cora", {"A3"}, 59), std::string("limit"));
    EXPECT_EQ(sh.hold("cora", {"A3", "A4"}, 60), std::string("H2"));
}

TEST("limit errors and their order", 2) {
    SeatHolds sh(seat_ids("A", 4), 60);
    EXPECT_EQ(set_limit(sh, 0), std::string("bad-limit"));
    EXPECT_EQ(sh.hold("cora", {"A1", "A2", "A3"}, 0), std::string("H1"));  // still no limit
    EXPECT_EQ(set_limit(sh, 3), std::string("ok"));
    EXPECT_EQ(sh.hold("dev", {"A4"}, 0), std::string("H2"));
    EXPECT_EQ(sh.hold("cora", {"A4"}, 0), std::string("limit"));         // limit before unavailable
    EXPECT_EQ(sh.hold("cora", {"Z1"}, 0), std::string("unknown-seat"));  // unknown-seat before limit
    EXPECT_EQ(sh.hold("cora", {"A4", "A4"}, 0), std::string("bad-request"));
}

THREAD_TEST("threads: one customer in 16 tabs still gets exactly the limit", 2) {
    const int kThreads = 16, kEach = 4, kRounds = 100, kLimit = 4;
    std::vector<std::string> seats = seat_ids("S", kThreads * kEach);
    for (int round = 0; round < kRounds && check::failures() == 0; ++round) {
        SeatHolds sh(seats, 1000000);
        if (set_limit(sh, kLimit) != "ok") return;
        std::atomic<int> held{0};
        std::latch start(kThreads);
        std::vector<std::thread> threads;
        for (int t = 0; t < kThreads; ++t)
            threads.emplace_back([&, t] {
                check::guarded([&] {
                    start.arrive_and_wait();
                    for (int i = 0; i < kEach; ++i) {
                        std::string r = sh.hold("cora", {seats[(size_t)(t * kEach + i)]}, 0);
                        if (r.size() > 1 && r[0] == 'H') ++held;
                        else if (r != "limit") check::fail(__FILE__, __LINE__, "hold returned " + r);
                    }
                });
            });
        for (auto& th : threads) th.join();
        if (held != kLimit)
            check::fail(__FILE__, __LINE__, "round " + std::to_string(round) + ": cora holds " + std::to_string(held.load()) +
                                                " seats with a limit of " + std::to_string(kLimit));
        EXPECT_EQ(sh.freeCount(0), (int)seats.size() - kLimit);
    }
}
