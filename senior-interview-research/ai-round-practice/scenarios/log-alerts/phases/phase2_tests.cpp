// Phase 2 tests: evaluate(timeline, rules). Events are built directly, not parsed.
#include <string>
#include <vector>

#include "alerts.hpp"
#include "check.hpp"
#include "rules.hpp"

namespace {

Event ev(std::int64_t ms, const char* service, Level level, const char* code) {
    Event e;
    e.ts_ms = ms;
    e.service = service;
    e.level = level;
    e.code = code;
    return e;
}

std::vector<std::string> brief(const std::vector<Alert>& alerts) {
    std::vector<std::string> out;
    for (const Alert& a : alerts)
        out.push_back(a.rule + " " + a.service + "/" + a.code + " @" + std::to_string(a.at_ms) + " n=" + std::to_string(a.count));
    return out;
}

std::uint64_t alerts_hash(const std::vector<Alert>& alerts) {
    std::string text;
    for (const Alert& a : alerts)
        text += a.rule + "|" + a.service + "|" + a.code + "|" + std::to_string(a.at_ms) + "|" + std::to_string(a.count) + "\n";
    return check::fnv1a(text);
}

const Level I = Level::Info, W = Level::Warn, E = Level::Error, F = Level::Fatal;

}  // namespace

TEST("fires once when the threshold is reached", 2) {
    auto rules = parse_rules("rule r service=api level=ERROR code=E1 threshold=3 window=10s cooldown=1m\n");
    auto got = evaluate({ev(0, "api", E, "E1"), ev(1000, "api", E, "E1"), ev(2000, "api", E, "E1"), ev(3000, "api", E, "E1")}, rules);
    EXPECT_EQ(brief(got), (std::vector<std::string>{"r api/E1 @2000 n=3"}));
}

TEST("window is (t - window, t]", 2) {
    auto rules = parse_rules("rule r level=ERROR threshold=3 window=10s cooldown=0ms\n");
    auto got = evaluate({ev(0, "api", E, "E1"), ev(5000, "api", E, "E1"), ev(10000, "api", E, "E1"), ev(12000, "api", E, "E1")}, rules);
    // At 10000 the event at 0 is exactly one window old and no longer counts.
    EXPECT_EQ(brief(got), (std::vector<std::string>{"r api/E1 @12000 n=3"}));
}

TEST("cooldown silences the key, but silenced events still count", 2) {
    auto rules = parse_rules("rule r level=ERROR threshold=2 window=10s cooldown=5s\n");
    auto got = evaluate({ev(0, "api", E, "E1"), ev(1000, "api", E, "E1"), ev(2000, "api", E, "E1"),
                         ev(5999, "api", E, "E1"), ev(6000, "api", E, "E1")},
                        rules);
    EXPECT_EQ(brief(got), (std::vector<std::string>{"r api/E1 @1000 n=2", "r api/E1 @6000 n=5"}));
}

TEST("each service and code is its own key, even for wildcard rules", 2) {
    auto rules = parse_rules("rule r service=* level=ERROR code=* threshold=2 window=1m cooldown=1m\n");
    auto got = evaluate({ev(0, "api", E, "E1"), ev(100, "billing", E, "E1"), ev(200, "api", E, "E2"), ev(300, "api", E, "E1")}, rules);
    EXPECT_EQ(brief(got), (std::vector<std::string>{"r api/E1 @300 n=2"}));
}

TEST("minimum level and filters", 2) {
    auto rules = parse_rules("rule warn service=api level=WARN threshold=2 window=1m cooldown=1h\n"
                             "rule err level=ERROR code=E9 threshold=1 window=1s cooldown=0ms\n");
    auto got = evaluate({ev(0, "api", I, "E9"), ev(10, "api", I, "E9"), ev(20, "api", W, "E9"), ev(30, "billing", F, "E9"),
                         ev(40, "api", F, "E9")},
                        rules);
    EXPECT_EQ(brief(got), (std::vector<std::string>{"err billing/E9 @30 n=1", "warn api/E9 @40 n=2", "err api/E9 @40 n=1"}));
}

TEST("no rules, no events", 2) {
    EXPECT_TRUE(evaluate({}, parse_rules("rule r threshold=1 window=1s\n")).empty());
    EXPECT_TRUE(evaluate({ev(0, "api", F, "X")}, {}).empty());
}

TEST("medium random timelines", 2) {
    struct Case { int n; std::uint64_t seed; std::size_t alerts; std::uint64_t hash; };
    const Case cases[] = {{2000, 21, 12, 15219302385088218544ULL}, {20000, 22, 133, 18173813279103503302ULL}};
    const char* rules_text =
        "rule hot      service=svc1 level=ERROR code=*  threshold=4  window=2s   cooldown=10s\n"
        "rule anywarn  service=*    level=WARN  code=*  threshold=6  window=5s   cooldown=30s\n"
        "rule e7       service=*    level=INFO  code=C7 threshold=3  window=500ms cooldown=1s\n"
        "rule fatal    service=*    level=FATAL code=*  threshold=1  window=1s   cooldown=1m\n"
        "rule chatty   service=svc0 level=DEBUG code=*  threshold=25 window=10s  cooldown=0ms\n";
    auto rules = parse_rules(rules_text);
    for (const Case& c : cases) {
        check::Rng rng(c.seed);
        std::vector<Event> timeline;
        std::int64_t t = 1794736800000;
        const Level levels[] = {Level::Debug, I, I, W, E, E, F};
        for (int i = 0; i < c.n; ++i) {
            t += rng.range(0, 120);
            std::string svc = "svc" + std::to_string(rng.range(0, 3));
            std::string code = "C" + std::to_string(rng.range(0, 9));
            Level lv = levels[rng.range(0, rng.range(0, 30) == 0 ? 6 : 5)];
            timeline.push_back(ev(t, svc.c_str(), lv, code.c_str()));
        }
        auto got = evaluate(timeline, rules);
        EXPECT_EQ(got.size(), c.alerts);
        EXPECT_EQ(alerts_hash(got), c.hash);
    }
}
