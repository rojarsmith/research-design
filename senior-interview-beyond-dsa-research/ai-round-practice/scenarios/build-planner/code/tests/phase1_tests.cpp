// Existing tests for buildplan. Do not edit them to make them pass.
#include <string>

#include "check.hpp"
#include "duration.hpp"
#include "graph.hpp"
#include "manifest.hpp"
#include "report.hpp"

namespace {

BuildGraph load(const std::string& text) { return build_graph(parse_manifest(text)); }

std::vector<std::string> names(const BuildGraph& g, const std::vector<int>& idx) {
    std::vector<std::string> out;
    for (int i : idx) out.push_back(g.names[i]);
    return out;
}

template <class E, class F>
std::string error_of(F&& f) {
    try {
        f();
    } catch (const E& e) {
        return e.what();
    }
    return "(no error)";
}

}  // namespace

TEST("format_duration picks a readable unit", 1) {
    EXPECT_EQ(format_duration(0), std::string("0ms"));
    EXPECT_EQ(format_duration(999), std::string("999ms"));
    EXPECT_EQ(format_duration(1000), std::string("1.0s"));
    EXPECT_EQ(format_duration(59999), std::string("59.9s"));
    EXPECT_EQ(format_duration(60000), std::string("1m 00.0s"));
    EXPECT_EQ(format_duration(123456), std::string("2m 03.4s"));
}

TEST("parse_duration_ms handles units and combined parts", 1) {
    EXPECT_EQ(parse_duration_ms("300ms"), std::int64_t(300));
    EXPECT_EQ(parse_duration_ms("2s"), std::int64_t(2000));
    EXPECT_EQ(parse_duration_ms("2m"), std::int64_t(120000));
    EXPECT_EQ(parse_duration_ms("1m30s"), std::int64_t(90000));
    EXPECT_EQ(parse_duration_ms("1h"), std::int64_t(3600000));
}

TEST("parse_duration_ms rejects bad input", 1) {
    EXPECT_EQ(error_of<DurationError>([] { parse_duration_ms(""); }), std::string("empty duration"));
    EXPECT_EQ(error_of<DurationError>([] { parse_duration_ms("5"); }), std::string("missing unit in '5'"));
    EXPECT_EQ(error_of<DurationError>([] { parse_duration_ms("5d"); }), std::string("unknown duration unit 'd'"));
    EXPECT_EQ(error_of<DurationError>([] { parse_duration_ms("ms"); }), std::string("expected a number in 'ms'"));
}

TEST("parse_manifest reads targets, durations and deps", 1) {
    Manifest m = parse_manifest(
        "# comment\n"
        "target a 10ms\n"
        "\n"
        "target b 2s deps=a   # trailing comment\n"
        "target c 1m deps=a,b,a\n");
    EXPECT_EQ(m.targets.size(), std::size_t(3));
    EXPECT_EQ(m.targets[1].name, std::string("b"));
    EXPECT_EQ(m.targets[1].line, 4);
    EXPECT_EQ(m.targets[2].duration_ms, std::int64_t(60000));
    EXPECT_EQ(m.targets[2].deps, (std::vector<std::string>{"a", "b", "a"}));
}

TEST("parse_manifest reports the failing line", 1) {
    EXPECT_EQ(error_of<ManifestError>([] { parse_manifest("target a 1s\ntarget a 2s\n"); }),
              std::string("line 2: duplicate target 'a'"));
    EXPECT_EQ(error_of<ManifestError>([] { parse_manifest("target a\n"); }),
              std::string("line 1: expected: target <name> <duration> [deps=...]"));
    EXPECT_EQ(error_of<ManifestError>([] { parse_manifest("\ntarget a 3x\n"); }),
              std::string("line 2: unknown duration unit 'x'"));
}

TEST("build_graph rejects unknown and self dependencies, keeps one edge per pair", 1) {
    EXPECT_EQ(error_of<GraphError>([] { load("target a 1s deps=zzz\n"); }),
              std::string("target 'a' (line 1) depends on unknown target 'zzz'"));
    EXPECT_EQ(error_of<GraphError>([] { load("target a 1s deps=a\n"); }), std::string("target 'a' depends on itself"));
    BuildGraph g = load("target a 1s\ntarget b 1s deps=a,a,a\n");
    EXPECT_EQ(g.deps[1].size(), std::size_t(1));
    EXPECT_EQ(g.dependents[0].size(), std::size_t(1));
}

TEST("find_cycle finds a cycle and nothing in a DAG", 1) {
    BuildGraph dag = load("target a 1s\ntarget b 1s deps=a\ntarget c 1s deps=a,b\n");
    EXPECT_TRUE(find_cycle(dag).empty());
    BuildGraph cyc = load("target a 1s deps=c\ntarget b 1s deps=a\ntarget c 1s deps=b\ntarget d 1s\n");
    auto cycle = names(cyc, find_cycle(cyc));
    EXPECT_EQ(cycle.size(), std::size_t(4));
    if (!cycle.empty()) EXPECT_EQ(cycle.front(), cycle.back());
}

TEST("topo_order puts deps first and breaks ties by name", 1) {
    BuildGraph g = load("target zeta 1s\ntarget beta 1s\ntarget alpha 1s deps=zeta\ntarget gamma 1s deps=beta\n");
    EXPECT_EQ(names(g, topo_order(g)), (std::vector<std::string>{"beta", "gamma", "zeta", "alpha"}));
}

TEST("critical path of the sample manifest", 1) {
    BuildGraph g = build_graph(parse_manifest(check::read_file("data/sample.manifest")));
    // proto -> core -> ui -> app -> tests = 400ms + 1.2s + 2.5s + 3s + 1.5s
    EXPECT_EQ(critical_path_ms(g), std::int64_t(8600));
}

TEST("plan report for the sample manifest", 1) {
    BuildGraph g = build_graph(parse_manifest(check::read_file("data/sample.manifest")));
    EXPECT_EQ(plan_report(g), std::string(
        "Build plan: 8 targets\n"
        "  1. assets  1.8s\n"
        "  2. docs    700ms\n"
        "  3. proto   400ms\n"
        "  4. core    1.2s\n"
        "  5. net     900ms\n"
        "  6. ui      2.5s\n"
        "  7. app     3.0s\n"
        "  8. tests   1.5s\n"
        "Critical path: 8.6s\n"));
}
