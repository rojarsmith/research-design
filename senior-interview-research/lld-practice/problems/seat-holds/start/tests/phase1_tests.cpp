// Phase 1 tests: hold, confirm, release, expire, from one thread and from many.
//
// The thread tests are deterministic in what they check: each checks an invariant that holds for every
// possible interleaving of a correct implementation (no seat in two live holds, all or nothing, unique
// ids). Which thread wins a seat varies from run to run; the invariants do not. To make a race likely to
// show up, every round starts all threads at once from a latch, and the test repeats many rounds.
#include <algorithm>
#include <atomic>
#include <latch>
#include <map>
#include <set>
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

bool is_hold_id(const std::string& r) { return r.size() > 1 && r[0] == 'H'; }

}  // namespace

TEST("hold, then confirm", 1) {
    SeatHolds sh(seat_ids("A", 4), 60);
    EXPECT_EQ(sh.hold("cora", {"A1", "A2"}, 0), std::string("H1"));
    EXPECT_EQ(sh.seatState("A1", 1), std::string("held"));
    EXPECT_EQ(sh.seatState("A3", 1), std::string("free"));
    EXPECT_EQ(sh.freeCount(1), 2);
    EXPECT_EQ(sh.confirm("H1", "cora", 30), std::string("ok"));
    EXPECT_EQ(sh.seatState("A2", 31), std::string("sold"));
    EXPECT_EQ(sh.freeCount(500), 2);  // sold seats never come back
    EXPECT_EQ(sh.hold("dev", {"A2"}, 600), std::string("unavailable"));
}

TEST("all or nothing, and failed holds use no id", 1) {
    SeatHolds sh(seat_ids("A", 4), 60);
    EXPECT_EQ(sh.hold("cora", {"A1"}, 0), std::string("H1"));
    EXPECT_EQ(sh.hold("dev", {"A2", "A1"}, 0), std::string("unavailable"));
    EXPECT_EQ(sh.seatState("A2", 0), std::string("free"));
    EXPECT_EQ(sh.hold("dev", {"A2", "A3"}, 0), std::string("H2"));
    EXPECT_EQ(sh.freeCount(0), 1);
}

TEST("holds expire after holdSeconds", 1) {
    SeatHolds sh(seat_ids("A", 2), 60);
    EXPECT_EQ(sh.hold("cora", {"A1"}, 100), std::string("H1"));
    EXPECT_EQ(sh.seatState("A1", 159), std::string("held"));
    EXPECT_EQ(sh.seatState("A1", 160), std::string("free"));
    EXPECT_EQ(sh.confirm("H1", "cora", 160), std::string("expired"));
    EXPECT_EQ(sh.release("H1", "cora", 160), std::string("expired"));
    EXPECT_EQ(sh.hold("dev", {"A1"}, 160), std::string("H2"));
    EXPECT_EQ(sh.freeCount(161), 1);
}

TEST("release frees the seats, once", 1) {
    SeatHolds sh(seat_ids("A", 2), 60);
    EXPECT_EQ(sh.hold("cora", {"A1", "A2"}, 0), std::string("H1"));
    EXPECT_EQ(sh.release("H1", "cora", 5), std::string("ok"));
    EXPECT_EQ(sh.freeCount(5), 2);
    EXPECT_EQ(sh.release("H1", "cora", 6), std::string("done"));
    EXPECT_EQ(sh.confirm("H1", "cora", 6), std::string("done"));
    EXPECT_EQ(sh.hold("dev", {"A2"}, 7), std::string("H2"));
    EXPECT_EQ(sh.confirm("H2", "dev", 8), std::string("ok"));
    EXPECT_EQ(sh.release("H2", "dev", 9), std::string("done"));
    EXPECT_EQ(sh.confirm("H2", "dev", 9000), std::string("done"));  // done comes before expired
}

TEST("errors, in order", 1) {
    SeatHolds sh(seat_ids("A", 3), 60);
    EXPECT_EQ(sh.hold("cora", {}, 0), std::string("bad-request"));
    EXPECT_EQ(sh.hold("cora", {"A1", "A1"}, 0), std::string("bad-request"));
    EXPECT_EQ(sh.hold("cora", {"A1", "Z9", "Z9"}, 0), std::string("bad-request"));
    EXPECT_EQ(sh.hold("cora", {"A1", "Z9"}, 0), std::string("unknown-seat"));
    EXPECT_EQ(sh.hold("cora", {"A1"}, 0), std::string("H1"));
    EXPECT_EQ(sh.hold("dev", {"Z9", "A1"}, 0), std::string("unknown-seat"));
    EXPECT_EQ(sh.confirm("H7", "cora", 0), std::string("unknown-hold"));
    EXPECT_EQ(sh.confirm("H1", "dev", 0), std::string("not-yours"));
    EXPECT_EQ(sh.release("H1", "dev", 999), std::string("not-yours"));
    EXPECT_EQ(sh.seatState("Z9", 0), std::string("unknown-seat"));
}

TEST("time never goes backwards", 1) {
    SeatHolds sh(seat_ids("A", 2), 60);
    EXPECT_EQ(sh.hold("cora", {"A1"}, 0), std::string("H1"));
    EXPECT_EQ(sh.freeCount(100), 2);                       // a worker at 100: H1 has expired
    EXPECT_EQ(sh.confirm("H1", "cora", 50), std::string("expired"));  // a worker that is behind: still 100
    EXPECT_EQ(sh.hold("dev", {"A2"}, 10), std::string("H2"));        // made at 100, expires at 160
    EXPECT_EQ(sh.seatState("A2", 159), std::string("held"));
    EXPECT_EQ(sh.seatState("A2", 160), std::string("free"));
}

THREAD_TEST("threads: no seat is ever in two live holds", 1) {
    const int kSeats = 40, kThreads = 8, kRounds = 30, kTries = 300;
    std::vector<std::string> seats = seat_ids("S", kSeats);
    for (int round = 0; round < kRounds && check::failures() == 0; ++round) {
        SeatHolds sh(seats, 1000000);
        // Each thread keeps its own live holds: id and seats.
        std::vector<std::vector<std::pair<std::string, std::vector<std::string>>>> mine(kThreads);
        std::latch start(kThreads);
        std::vector<std::thread> threads;
        for (int t = 0; t < kThreads; ++t)
            threads.emplace_back([&, t] {
                check::guarded([&] {
                    check::Rng rng((std::uint64_t)(round * 1000 + t + 1));
                    std::string me = "c" + std::to_string(t);
                    start.arrive_and_wait();
                    for (int i = 0; i < kTries; ++i) {
                        if (!mine[t].empty() && rng.range(0, 3) == 0) {
                            auto h = mine[t].back();
                            mine[t].pop_back();
                            std::string r = sh.release(h.first, me, 0);
                            if (r != "ok") check::fail(__FILE__, __LINE__, "release of a live hold returned " + r);
                            continue;
                        }
                        std::set<std::string> want;
                        int n = (int)rng.range(1, 3);
                        while ((int)want.size() < n) want.insert(seats[(size_t)rng.range(0, kSeats - 1)]);
                        std::vector<std::string> list(want.begin(), want.end());
                        if (rng.range(0, 1)) std::reverse(list.begin(), list.end());
                        std::string r = sh.hold(me, list, 0);
                        if (is_hold_id(r)) mine[t].push_back({r, list});
                        else if (r != "unavailable") check::fail(__FILE__, __LINE__, "hold returned " + r);
                    }
                });
            });
        for (auto& th : threads) th.join();
        std::map<std::string, int> holders;
        int held = 0;
        for (auto& list : mine)
            for (auto& h : list)
                for (auto& s : h.second) ++holders[s], ++held;
        for (auto& [seat, n] : holders)
            if (n != 1) check::fail(__FILE__, __LINE__, "round " + std::to_string(round) + ": seat " + seat + " is in " + std::to_string(n) + " live holds");
        EXPECT_EQ(sh.freeCount(0), kSeats - held);
        for (auto& [seat, n] : holders) EXPECT_EQ(sh.seatState(seat, 0), std::string("held"));
        // Confirm everything from all threads at once: every live hold sells.
        std::latch go(kThreads);
        std::vector<std::thread> confirmers;
        for (int t = 0; t < kThreads; ++t)
            confirmers.emplace_back([&, t] {
                check::guarded([&] {
                    go.arrive_and_wait();
                    for (auto& h : mine[t]) {
                        std::string r = sh.confirm(h.first, "c" + std::to_string(t), 0);
                        if (r != "ok") check::fail(__FILE__, __LINE__, "confirm of a live hold returned " + r);
                    }
                });
            });
        for (auto& th : confirmers) th.join();
        EXPECT_EQ(sh.freeCount(0), kSeats - held);
    }
}

THREAD_TEST("threads: opposite seat orders neither deadlock nor leave half a hold", 1) {
    SeatHolds sh({"X", "Y", "Z"}, 1000000);
    std::latch start(2);
    auto worker = [&](const std::string& me, std::vector<std::string> order) {
        check::guarded([&] {
            start.arrive_and_wait();
            for (int i = 0; i < 5000; ++i) {
                std::string r = sh.hold(me, order, 0);
                if (is_hold_id(r)) {
                    std::string rel = sh.release(r, me, 0);
                    if (rel != "ok") check::fail(__FILE__, __LINE__, "release returned " + rel);
                } else if (r != "unavailable") {
                    check::fail(__FILE__, __LINE__, "hold returned " + r);
                }
            }
        });
    };
    std::thread a(worker, "ann", std::vector<std::string>{"X", "Y"});
    std::thread b(worker, "ben", std::vector<std::string>{"Y", "X"});
    a.join();
    b.join();
    EXPECT_EQ(sh.freeCount(0), 3);
    EXPECT_EQ(sh.seatState("X", 0), std::string("free"));
    EXPECT_EQ(sh.seatState("Y", 0), std::string("free"));
}

THREAD_TEST("threads: hold ids are unique and have no gaps", 1) {
    const int kThreads = 16, kEach = 200;
    std::vector<std::string> seats = seat_ids("S", kThreads);
    SeatHolds sh(seats, 1000000);
    std::vector<std::vector<std::string>> ids(kThreads);
    std::latch start(kThreads);
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t)
        threads.emplace_back([&, t] {
            check::guarded([&] {
                std::string me = "c" + std::to_string(t);
                start.arrive_and_wait();
                for (int i = 0; i < kEach; ++i) {
                    std::string r = sh.hold(me, {seats[(size_t)t]}, 0);
                    if (!is_hold_id(r)) {
                        check::fail(__FILE__, __LINE__, "hold of a free seat returned " + r);
                        return;
                    }
                    ids[t].push_back(r);
                    sh.release(r, me, 0);
                }
            });
        });
    for (auto& th : threads) th.join();
    std::set<std::string> all, expected;
    size_t total = 0;
    for (auto& v : ids) {
        all.insert(v.begin(), v.end());
        total += v.size();
    }
    for (int i = 1; i <= kThreads * kEach; ++i) expected.insert("H" + std::to_string(i));
    EXPECT_EQ(total, (size_t)(kThreads * kEach));
    EXPECT_TRUE(all == expected);
}
