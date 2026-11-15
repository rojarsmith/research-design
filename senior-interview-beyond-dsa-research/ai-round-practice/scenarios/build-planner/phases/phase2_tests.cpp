// Phase 2 tests: simulate(g, workers).
#include <stdexcept>
#include <string>

#include "check.hpp"
#include "graph.hpp"
#include "manifest.hpp"
#include "scheduler.hpp"

namespace {

BuildGraph load(const std::string& text) { return build_graph(parse_manifest(text)); }

std::vector<std::string> started(const BuildGraph& g, const Schedule& s) {
    std::vector<std::string> out;
    for (int i : s.order) out.push_back(g.names[i]);
    return out;
}

std::int64_t start_of(const BuildGraph& g, const Schedule& s, const std::string& name) {
    int i = g.index.at(name);
    return i < (int)s.start_ms.size() ? s.start_ms[i] : -1;
}

std::uint64_t schedule_hash(const Schedule& s) {
    std::string text = std::to_string(s.makespan_ms) + ":";
    for (auto t : s.start_ms) text += std::to_string(t) + ",";
    for (int i : s.order) text += std::to_string(i) + ",";
    return check::fnv1a(text);
}

// Random DAG: each target depends on up to 4 of the 50 targets before it.
// Names are random so that name order and file order disagree.
std::string random_dag(int n, std::uint64_t seed) {
    check::Rng rng(seed);
    std::vector<std::string> names;
    for (int i = 0; i < n; ++i) names.push_back("t" + std::to_string(rng.range(0, 999999)) + "_" + std::to_string(i));
    std::string text;
    for (int i = 0; i < n; ++i) {
        text += "target " + names[i] + " " + std::to_string(rng.range(0, 5000)) + "ms";
        int k = i == 0 ? 0 : (int)rng.range(0, std::min(4, i));
        for (int j = 0; j < k; ++j) {
            text += j ? "," : " deps=";
            text += names[rng.range(std::max(0, i - 50), i - 1)];
        }
        text += "\n";
    }
    return text;
}

}  // namespace

TEST("chain on one worker", 2) {
    BuildGraph g = load("target a 100ms\ntarget b 200ms deps=a\ntarget c 300ms deps=b\n");
    Schedule s = simulate(g, 1);
    EXPECT_EQ(s.makespan_ms, std::int64_t(600));
    EXPECT_EQ(s.start_ms, (std::vector<std::int64_t>{0, 100, 300}));
    EXPECT_EQ(started(g, s), (std::vector<std::string>{"a", "b", "c"}));
}

TEST("longer tail goes first, not the smaller name", 2) {
    BuildGraph g = load("target a 100ms\ntarget b 100ms\ntarget c 500ms deps=b\n");
    Schedule s = simulate(g, 1);
    EXPECT_EQ(started(g, s), (std::vector<std::string>{"b", "c", "a"}));
    EXPECT_EQ(s.makespan_ms, std::int64_t(700));
}

TEST("equal tails break ties by name", 2) {
    BuildGraph g = load(
        "target root 100ms\n"
        "target right 300ms deps=root\n"
        "target mid 200ms deps=root\n"
        "target left 300ms deps=root\n"
        "target join 100ms deps=left,right,mid\n");
    Schedule s = simulate(g, 2);
    EXPECT_EQ(started(g, s), (std::vector<std::string>{"root", "left", "right", "mid", "join"}));
    EXPECT_EQ(start_of(g, s, "mid"), std::int64_t(400));
    EXPECT_EQ(s.makespan_ms, std::int64_t(700));
}

TEST("zero-duration targets release dependents at the same moment", 2) {
    BuildGraph g = load(
        "target gen 0ms\n"
        "target lib 300ms deps=gen\n"
        "target cfg 0ms deps=gen\n"
        "target app 100ms deps=lib,cfg\n"
        "target lint 200ms\n");
    Schedule s = simulate(g, 2);
    EXPECT_EQ(started(g, s), (std::vector<std::string>{"gen", "lint", "lib", "cfg", "app"}));
    EXPECT_EQ(s.start_ms, (std::vector<std::int64_t>{0, 0, 200, 300, 0}));
    EXPECT_EQ(s.makespan_ms, std::int64_t(400));
}

TEST("more workers than targets", 2) {
    BuildGraph g = load("target x 50ms\ntarget y 70ms\ntarget z 70ms\n");
    Schedule s = simulate(g, 10);
    EXPECT_EQ(started(g, s), (std::vector<std::string>{"y", "z", "x"}));
    EXPECT_EQ(s.start_ms, (std::vector<std::int64_t>{0, 0, 0}));
    EXPECT_EQ(s.makespan_ms, std::int64_t(70));
}

TEST("empty manifest, bad worker count, cycles", 2) {
    Schedule empty = simulate(load(""), 3);
    EXPECT_EQ(empty.makespan_ms, std::int64_t(0));
    EXPECT_TRUE(empty.order.empty());

    bool invalid = false;
    try { simulate(load("target a 1ms\n"), 0); } catch (const std::invalid_argument&) { invalid = true; }
    EXPECT_TRUE(invalid);

    bool cycle = false;
    try { simulate(load("target a 1ms deps=b\ntarget b 1ms deps=a\n"), 2); } catch (const GraphError&) { cycle = true; }
    EXPECT_TRUE(cycle);
}

TEST("medium random manifests", 2) {
    struct Case { int n; int workers; std::uint64_t seed; std::int64_t makespan; std::uint64_t hash; };
    const Case cases[] = {
        {300, 3, 11, 242040, 18027366973916788311ULL},
        {2000, 7, 12, 695829, 11482830809501189824ULL},
        {2000, 1, 13, 4922176, 7277930586058922362ULL},
    };
    for (const Case& c : cases) {
        BuildGraph g = load(random_dag(c.n, c.seed));
        Schedule s = simulate(g, c.workers);
        EXPECT_EQ(s.makespan_ms, c.makespan);
        EXPECT_EQ(schedule_hash(s), c.hash);
    }
}
