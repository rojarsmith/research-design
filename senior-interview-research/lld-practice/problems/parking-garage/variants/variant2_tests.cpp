// Variant 2 tests: allocation policies.
#include <string>
#include <vector>

#include "check.hpp"
#include "garage.hpp"

namespace {

template <class G>
std::string set_allocation(G& g, const std::string& policy) {
    if constexpr (requires { g.setAllocation(policy); }) return g.setAllocation(policy);
    else return check::missing<std::string>("std::string Garage::setAllocation(const std::string& policy)");
}

std::vector<std::string> park(Garage& g, const std::string& vehicle, int n, int& next_plate) {
    std::vector<std::string> spots;
    for (int i = 0; i < n; ++i) {
        std::string t = g.enter("P-" + std::to_string(next_plate++), vehicle, 0);
        spots.push_back(t.rfind("T", 0) == 0 ? g.spotOf(t) : t);
    }
    return spots;
}

}  // namespace

TEST("spread: the level with the most free spots of the chosen size", 3) {
    Garage g({{0, 2, 0}, {0, 3, 0}, {0, 3, 0}});
    EXPECT_EQ(set_allocation(g, "spread"), std::string("ok"));
    int plate = 1;
    // free regular spots per level: 2 3 3 -> level 2; 2 2 3 -> level 3; 2 2 2 -> level 1; 1 2 2 -> level 2 ...
    EXPECT_EQ(park(g, "car", 8, plate),
              (std::vector<std::string>{"2-R1", "3-R1", "1-R1", "2-R2", "3-R2", "1-R2", "2-R3", "3-R3"}));
    EXPECT_EQ(park(g, "car", 1, plate), (std::vector<std::string>{"full"}));
}

TEST("spread keeps choosing the smallest size first", 3) {
    Garage g({{1, 0, 0}, {0, 5, 0}});
    EXPECT_EQ(set_allocation(g, "spread"), std::string("ok"));
    int plate = 1;
    EXPECT_EQ(park(g, "motorcycle", 2, plate), (std::vector<std::string>{"1-S1", "2-R1"}));
}

TEST("freed spots count again when spreading", 3) {
    Garage g({{0, 2, 0}, {0, 2, 0}});
    EXPECT_EQ(set_allocation(g, "spread"), std::string("ok"));
    std::string a = g.enter("A", "car", 0);  // 1-R1 (tie: lower level)
    std::string b = g.enter("B", "car", 0);  // 2-R1
    std::string c = g.enter("C", "car", 0);  // 1-R2
    EXPECT_EQ(g.spotOf(c), std::string("1-R2"));
    EXPECT_EQ(g.exit(a, 30), 300LL);
    EXPECT_EQ(g.exit(c, 30), 300LL);
    std::string d = g.enter("D", "car", 40);  // level 1 has 2 free, level 2 has 1
    EXPECT_EQ(g.spotOf(d), std::string("1-R1"));
    EXPECT_EQ(g.spotOf(b), std::string("2-R1"));
}

TEST("switching back to lowest-level, and unknown policies", 3) {
    Garage g({{0, 1, 0}, {0, 3, 0}});
    EXPECT_EQ(set_allocation(g, "spread"), std::string("ok"));
    EXPECT_EQ(set_allocation(g, "random"), std::string("unknown-policy"));
    int plate = 1;
    EXPECT_EQ(park(g, "car", 1, plate), (std::vector<std::string>{"2-R1"}));  // still spread
    EXPECT_EQ(set_allocation(g, "lowest-level"), std::string("ok"));
    EXPECT_EQ(park(g, "car", 2, plate), (std::vector<std::string>{"1-R1", "2-R2"}));
}
