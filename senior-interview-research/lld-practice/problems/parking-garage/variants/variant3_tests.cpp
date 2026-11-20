// Variant 3 tests: EV charging.
#include <string>
#include <vector>

#include "check.hpp"
#include "garage.hpp"

namespace {

template <class G>
std::string add_chargers(G& g, int level, const std::string& size, int count) {
    if constexpr (requires { g.addChargers(level, size, count); }) return g.addChargers(level, size, count);
    else return check::missing<std::string>("std::string Garage::addChargers(int level, const std::string& size, int count)");
}

template <class G>
std::string set_allocation(G& g, const std::string& policy) {
    if constexpr (requires { g.setAllocation(policy); }) return g.setAllocation(policy);
    else return check::missing<std::string>("std::string Garage::setAllocation(const std::string& policy)");
}

template <class G>
std::string set_pricing(G& g, const std::string& plan) {
    if constexpr (requires { g.setPricing(plan); }) return g.setPricing(plan);
    else return check::missing<std::string>("std::string Garage::setPricing(const std::string& plan)");
}

std::string spot(Garage& g, const std::string& plate, const std::string& vehicle) {
    std::string t = g.enter(plate, vehicle, 0);
    return t.rfind("T", 0) == 0 ? g.spotOf(t) : t;
}

}  // namespace

TEST("EVs prefer chargers; other vehicles avoid them", 4) {
    Garage g({{0, 4, 1}, {0, 2, 1}});
    EXPECT_EQ(add_chargers(g, 1, "regular", 2), std::string("ok"));  // 1-R3 and 1-R4
    EXPECT_EQ(spot(g, "E-1", "ev-car"), std::string("1-R3"));
    EXPECT_EQ(spot(g, "C-1", "car"), std::string("1-R1"));
    EXPECT_EQ(spot(g, "C-2", "car"), std::string("1-R2"));
    EXPECT_EQ(spot(g, "C-3", "car"), std::string("2-R1"));
    EXPECT_EQ(spot(g, "C-4", "car"), std::string("2-R2"));
    EXPECT_EQ(spot(g, "C-5", "car"), std::string("1-L1"));  // a large spot before a charger
    EXPECT_EQ(spot(g, "C-6", "car"), std::string("2-L1"));
    EXPECT_EQ(spot(g, "C-7", "car"), std::string("1-R4"));  // only a charger is left
    EXPECT_EQ(spot(g, "E-2", "ev-car"), std::string("full"));
    EXPECT_EQ(g.freeSpots("regular"), 0);
}

TEST("an EV takes a normal spot when the chargers are taken", 4) {
    Garage g({{0, 3, 0}});
    EXPECT_EQ(add_chargers(g, 1, "regular", 1), std::string("ok"));  // 1-R3
    EXPECT_EQ(spot(g, "E-1", "ev-car"), std::string("1-R3"));
    EXPECT_EQ(spot(g, "E-2", "ev-car"), std::string("1-R1"));
    EXPECT_EQ(spot(g, "V-1", "ev-van"), std::string("full"));
}

TEST("charging fees", 4) {
    Garage g({{0, 2, 2}});
    EXPECT_EQ(add_chargers(g, 1, "regular", 1), std::string("ok"));  // 1-R2
    EXPECT_EQ(add_chargers(g, 1, "large", 1), std::string("ok"));    // 1-L2
    std::string e1 = g.enter("E-1", "ev-car", 0);   // 1-R2, charging
    std::string e2 = g.enter("E-2", "ev-car", 0);   // 1-L2, charging
    std::string e3 = g.enter("E-3", "ev-car", 0);   // 1-R1, no charger
    std::string c1 = g.enter("C-1", "car", 0);      // 1-L1
    EXPECT_EQ(g.spotOf(e2), std::string("1-L2"));
    EXPECT_EQ(g.spotOf(c1), std::string("1-L1"));
    EXPECT_EQ(g.exit(e1, 61), 1000LL);  // 600 + 2 hours of charging
    EXPECT_EQ(g.exit(e2, 10), 0LL);     // 15 free minutes cover charging too
    EXPECT_EQ(g.exit(e3, 61), 600LL);   // no charger, no charging fee
    std::string c2 = g.enter("C-2", "car", 100);  // free: 1-R1, and the chargers 1-R2 and 1-L2
    EXPECT_EQ(g.spotOf(c2), std::string("1-R1"));
    EXPECT_EQ(g.exit(c1, 61), 600LL);
}

TEST("a non-EV on a charging spot pays no charging fee", 4) {
    Garage g({{0, 1, 0}});
    EXPECT_EQ(add_chargers(g, 1, "regular", 1), std::string("ok"));
    std::string c = g.enter("C-1", "car", 0);
    EXPECT_EQ(g.spotOf(c), std::string("1-R1"));
    EXPECT_EQ(g.exit(c, 61), 600LL);
    std::string v = g.enter("V-1", "ev-van", 0);
    EXPECT_EQ(v, std::string("full"));
}

TEST("chargers work with the spread policy and the pricing plans", 4) {
    Garage g({{0, 4, 0}, {0, 4, 0}});
    EXPECT_EQ(add_chargers(g, 1, "regular", 2), std::string("ok"));  // 1-R3, 1-R4
    EXPECT_EQ(add_chargers(g, 2, "regular", 2), std::string("ok"));  // 2-R3, 2-R4
    EXPECT_EQ(set_allocation(g, "spread"), std::string("ok"));
    EXPECT_EQ(spot(g, "E-1", "ev-car"), std::string("1-R3"));  // chargers free: 2 and 2, tie
    EXPECT_EQ(spot(g, "E-2", "ev-car"), std::string("2-R3"));  // 1 and 2
    EXPECT_EQ(spot(g, "C-1", "car"), std::string("1-R1"));     // normal spots free: 2 and 2, tie
    EXPECT_EQ(set_pricing(g, "capped"), std::string("ok"));
    std::string t = g.enter("E-3", "ev-car", 0);  // chargers free: 1 and 1 -> 1-R4
    EXPECT_EQ(g.spotOf(t), std::string("1-R4"));
    EXPECT_EQ(g.exit(t, 26 * 60), 7800LL);  // capped 2600 + 26 hours of charging
}

TEST("addChargers errors", 4) {
    Garage g({{0, 4, 0}});
    EXPECT_EQ(add_chargers(g, 2, "regular", 1), std::string("bad-spot"));
    EXPECT_EQ(add_chargers(g, 0, "regular", 1), std::string("bad-spot"));
    EXPECT_EQ(add_chargers(g, 1, "huge", 1), std::string("bad-spot"));
    EXPECT_EQ(add_chargers(g, 1, "regular", 5), std::string("bad-spot"));
    EXPECT_EQ(add_chargers(g, 1, "regular", 0), std::string("bad-spot"));
    EXPECT_EQ(add_chargers(g, 1, "large", 1), std::string("bad-spot"));
    EXPECT_EQ(spot(g, "E-1", "ev-car"), std::string("1-R1"));  // no chargers were added
}
