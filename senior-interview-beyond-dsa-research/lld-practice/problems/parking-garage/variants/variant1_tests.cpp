// Variant 1 tests: pricing plans.
#include <string>

#include "check.hpp"
#include "garage.hpp"

namespace {

template <class G>
std::string set_pricing(G& g, const std::string& plan) {
    if constexpr (requires { g.setPricing(plan); }) return g.setPricing(plan);
    else return check::missing<std::string>("std::string Garage::setPricing(const std::string& plan)");
}

// The fee for one stay from minute `in` to minute `out` under `plan`.
long long fee(const std::string& plan, const std::string& vehicle, long long in, long long out) {
    Garage g({{5, 5, 5}});
    if (set_pricing(g, plan) != "ok") return -2;
    std::string t = g.enter("P-1", vehicle, in);
    return g.exit(t, out);
}

constexpr long long kDay = 1440;

}  // namespace

TEST("hourly is the default and can be chosen again", 2) {
    EXPECT_EQ(fee("hourly", "car", 0, 61), 600LL);
    Garage g({{0, 2, 0}});
    EXPECT_EQ(set_pricing(g, "capped"), std::string("ok"));
    EXPECT_EQ(set_pricing(g, "hourly"), std::string("ok"));
    std::string t = g.enter("A", "car", 0);
    EXPECT_EQ(g.exit(t, 26 * 60), 7800LL);
}

TEST("unknown plans are refused and change nothing", 2) {
    Garage g({{0, 2, 0}});
    EXPECT_EQ(set_pricing(g, "capped"), std::string("ok"));
    EXPECT_EQ(set_pricing(g, "weekend"), std::string("unknown-plan"));
    std::string t = g.enter("A", "car", 0);
    EXPECT_EQ(g.exit(t, 26 * 60), 2600LL);  // still capped
}

TEST("capped: each 24-hour block costs at most 2000", 2) {
    EXPECT_EQ(fee("capped", "car", 0, 26 * 60), 2600LL);
    EXPECT_EQ(fee("capped", "motorcycle", 0, 30 * 60), 2600LL);
    EXPECT_EQ(fee("capped", "car", 0, 3 * 60), 900LL);
    EXPECT_EQ(fee("capped", "car", 0, 10), 0LL);
    EXPECT_EQ(fee("capped", "van", 0, 2 * kDay), 4000LL);
    EXPECT_EQ(fee("capped", "van", 500, 500 + 2 * kDay + 1), 4500LL);
}

TEST("evening: a flat 800 for overnight stays that start after 18:00", 2) {
    EXPECT_EQ(fee("evening", "car", 18 * 60, kDay + 7 * 60 + 30), 800LL);
    EXPECT_EQ(fee("evening", "car", 18 * 60, kDay + 8 * 60), 800LL);
    EXPECT_EQ(fee("evening", "car", 17 * 60 + 59, kDay + 7 * 60), 4200LL);  // entered too early: hourly
    EXPECT_EQ(fee("evening", "car", 20 * 60, kDay + 8 * 60 + 1), 3900LL);   // left too late: hourly
    EXPECT_EQ(fee("evening", "motorcycle", 18 * 60, 20 * 60), 800LL);         // flat, even when hourly is cheaper
    EXPECT_EQ(fee("evening", "van", 2 * kDay + 19 * 60, 3 * kDay + 6 * 60), 800LL);
    EXPECT_EQ(fee("evening", "car", 23 * 60, 23 * 60 + 10), 0LL);  // 15 free minutes first
    EXPECT_EQ(fee("evening", "car", 9 * 60, 11 * 60), 600LL);
}

TEST("the plan in force at exit decides", 2) {
    Garage g({{0, 2, 0}});
    std::string a = g.enter("A", "car", 0);
    std::string b = g.enter("B", "car", 0);
    EXPECT_EQ(g.exit(a, 26 * 60), 7800LL);
    EXPECT_EQ(set_pricing(g, "capped"), std::string("ok"));
    EXPECT_EQ(g.exit(b, 26 * 60), 2600LL);
}
