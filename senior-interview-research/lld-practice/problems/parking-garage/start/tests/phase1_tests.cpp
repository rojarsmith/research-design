// Phase 1 tests: entering, leaving, paying. They only use the public interface in garage.hpp.
#include <string>
#include <vector>

#include "check.hpp"
#include "garage.hpp"

namespace {

// The fee for one stay of `minutes` in a roomy garage.
long long fee(const std::string& vehicle, long long minutes) {
    Garage g({{5, 5, 5}});
    std::string t = g.enter("P-1", vehicle, 100);
    return g.exit(t, 100 + minutes);
}

}  // namespace

TEST("the smallest size first, then the lowest level, then the lowest number", 1) {
    Garage g({{2, 2, 1}, {1, 1, 1}});
    std::vector<std::string> tickets;
    for (const char* p : {"M-1", "M-2", "M-3", "M-4"}) tickets.push_back(g.enter(p, "motorcycle", 0));
    for (const char* p : {"C-1", "C-2", "C-3"}) tickets.push_back(g.enter(p, "car", 0));
    tickets.push_back(g.enter("V-1", "van", 0));
    EXPECT_EQ(tickets, (std::vector<std::string>{"T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8"}));
    std::vector<std::string> spots;
    for (auto& t : tickets) spots.push_back(g.spotOf(t));
    EXPECT_EQ(spots, (std::vector<std::string>{"1-S1", "1-S2", "2-S1", "1-R1", "1-R2", "2-R1", "1-L1", "2-L1"}));
    EXPECT_EQ(g.enter("V-2", "van", 0), std::string("full"));
    EXPECT_EQ(g.enter("M-5", "motorcycle", 0), std::string("full"));
}

TEST("cars and vans never take a smaller spot", 1) {
    Garage g({{3, 0, 1}});
    EXPECT_EQ(g.enter("C-1", "car", 0), std::string("T1"));
    EXPECT_EQ(g.spotOf("T1"), std::string("1-L1"));
    EXPECT_EQ(g.enter("C-2", "car", 0), std::string("full"));
    EXPECT_EQ(g.enter("V-1", "van", 0), std::string("full"));
    EXPECT_EQ(g.enter("M-1", "motorcycle", 0), std::string("T2"));
    EXPECT_EQ(g.spotOf("T2"), std::string("1-S1"));
}

TEST("fees: 15 free minutes, then every started hour", 1) {
    EXPECT_EQ(fee("motorcycle", 15), 0LL);
    EXPECT_EQ(fee("motorcycle", 16), 100LL);
    EXPECT_EQ(fee("car", 60), 300LL);
    EXPECT_EQ(fee("car", 61), 600LL);
    EXPECT_EQ(fee("van", 1), 0LL);
    EXPECT_EQ(fee("van", 125), 1500LL);
    EXPECT_EQ(fee("car", 24 * 60), 7200LL);
}

TEST("leaving frees the spot, and the lowest spot is used again", 1) {
    Garage g({{0, 2, 0}});
    EXPECT_EQ(g.enter("A", "car", 0), std::string("T1"));
    EXPECT_EQ(g.enter("B", "car", 5), std::string("T2"));
    EXPECT_EQ(g.freeSpots("regular"), 0);
    EXPECT_EQ(g.exit("T1", 30), 300LL);
    EXPECT_EQ(g.freeSpots("regular"), 1);
    EXPECT_EQ(g.spotOf("T1"), std::string(""));
    EXPECT_EQ(g.enter("C", "car", 40), std::string("T3"));
    EXPECT_EQ(g.spotOf("T3"), std::string("1-R1"));
    EXPECT_EQ(g.spotOf("T2"), std::string("1-R2"));
}

TEST("errors when entering and leaving", 1) {
    Garage g({{1, 1, 0}});
    EXPECT_EQ(g.enter("X", "truck", 0), std::string("bad-vehicle"));
    EXPECT_EQ(g.enter("A", "car", 0), std::string("T1"));
    EXPECT_EQ(g.enter("A", "car", 5), std::string("already-inside"));
    EXPECT_EQ(g.enter("A", "motorcycle", 5), std::string("already-inside"));
    EXPECT_EQ(g.exit("T9", 10), -1LL);
    EXPECT_EQ(g.exit("T1", 10), 0LL);
    EXPECT_EQ(g.exit("T1", 20), -1LL);
    EXPECT_EQ(g.enter("A", "car", 30), std::string("T2"));
    EXPECT_EQ(g.spotOf("T9"), std::string(""));
}

TEST("errors are checked in order, and failed entries use no ticket id", 1) {
    Garage g({{0, 1, 0}});
    EXPECT_EQ(g.enter("A", "car", 0), std::string("T1"));
    EXPECT_EQ(g.enter("B", "truck", 0), std::string("bad-vehicle"));  // also full
    EXPECT_EQ(g.enter("A", "car", 0), std::string("already-inside"));  // also full
    EXPECT_EQ(g.enter("B", "car", 0), std::string("full"));
    EXPECT_EQ(g.exit("T1", 100), 600LL);
    EXPECT_EQ(g.enter("B", "car", 100), std::string("T2"));
}

TEST("free spots per size on all levels", 1) {
    Garage g({{2, 1, 1}, {1, 3, 0}});
    EXPECT_EQ(g.freeSpots("small"), 3);
    EXPECT_EQ(g.freeSpots("regular"), 4);
    EXPECT_EQ(g.freeSpots("large"), 1);
    EXPECT_EQ(g.freeSpots("huge"), -1);
    EXPECT_EQ(g.enter("V", "van", 0), std::string("T1"));
    EXPECT_EQ(g.enter("C", "car", 0), std::string("T2"));
    EXPECT_EQ(g.freeSpots("large"), 0);
    EXPECT_EQ(g.freeSpots("regular"), 3);
}
