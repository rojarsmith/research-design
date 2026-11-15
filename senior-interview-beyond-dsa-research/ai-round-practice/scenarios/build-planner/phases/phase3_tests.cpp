// Phase 3 tests: large manifests. Each test writes its manifest to data/generated/
// (open them to see how their shapes differ), parses it, and then times
// build_graph + simulate. Parsing is not timed.
#include <string>

#include "check.hpp"
#include "graph.hpp"
#include "manifest.hpp"
#include "scheduler.hpp"

namespace {

std::uint64_t schedule_hash(const Schedule& s) {
    std::string text = std::to_string(s.makespan_ms) + ":";
    for (auto t : s.start_ms) text += std::to_string(t) + ",";
    for (int i : s.order) text += std::to_string(i) + ",";
    return check::fnv1a(text);
}

// 24 layers of 5,000 targets. Each target depends on 1 to 6 targets from the three layers below it.
std::string wide_manifest() {
    check::Rng rng(301);
    const int layers = 24, per = 5000;
    std::string text;
    text.reserve(12 << 20);
    for (int L = 0; L < layers; ++L) {
        for (int i = 0; i < per; ++i) {
            text += "target w" + std::to_string(L) + "_" + std::to_string(i) + " " + std::to_string(rng.range(50, 20000)) + "ms";
            if (L > 0) {
                int k = (int)rng.range(1, 6);
                for (int j = 0; j < k; ++j) {
                    int dl = (int)rng.range(std::max(0, L - 3), L - 1);
                    text += j ? "," : " deps=";
                    text += "w" + std::to_string(dl) + "_" + std::to_string(rng.range(0, per - 1));
                }
            }
            text += "\n";
        }
    }
    return text;
}

// Two chains of 100,000 targets each, with occasional links to earlier targets of the other chain.
std::string chain_manifest() {
    check::Rng rng(302);
    const int len = 100000;
    std::string text;
    text.reserve(8 << 20);
    for (int i = 0; i < len; ++i) {
        for (int c = 0; c < 2; ++c) {
            text += "target c" + std::to_string(c) + "_" + std::to_string(i) + " " + std::to_string(rng.range(1, 2000)) + "ms";
            if (i > 0) {
                text += " deps=c" + std::to_string(c) + "_" + std::to_string(i - 1);
                if (rng.range(0, 9) == 0)
                    text += ",c" + std::to_string(1 - c) + "_" + std::to_string(rng.range(0, i - 1));
            }
            text += "\n";
        }
    }
    return text;
}

// 220,000 independent leaves, 2,000 group targets over random leaves, and one
// release target that depends on every leaf and every group.
std::string fanin_manifest() {
    check::Rng rng(303);
    const int leaves = 220000, groups = 2000;
    std::string text;
    text.reserve(8 << 20);
    for (int i = 0; i < leaves; ++i)
        text += "target leaf" + std::to_string(i) + " " + std::to_string(rng.range(10, 5000)) + "ms\n";
    for (int g = 0; g < groups; ++g) {
        text += "target group" + std::to_string(g) + " " + std::to_string(rng.range(100, 3000)) + "ms";
        for (int j = 0; j < 75; ++j) {
            text += j ? "," : " deps=";
            text += "leaf" + std::to_string(rng.range(0, leaves - 1));
        }
        text += "\n";
    }
    text += "target release 1000ms deps=";
    for (int i = 0; i < leaves; ++i) text += (i ? ",leaf" : "leaf") + std::to_string(i);
    for (int g = 0; g < groups; ++g) text += ",group" + std::to_string(g);
    text += "\n";
    return text;
}

void run_large(const std::string& file, const std::string& text, int workers, double limit_ms,
               std::int64_t makespan, std::uint64_t hash) {
    std::string path = check::write_generated(file, text);
    Manifest m = parse_manifest(check::read_file(path));
    Schedule s;
    check::measure(limit_ms, [&] {
        BuildGraph g = build_graph(m);
        s = simulate(g, workers);
    });
    EXPECT_EQ(s.makespan_ms, makespan);
    EXPECT_EQ(schedule_hash(s), hash);
}

}  // namespace

TEST("large-1 wide layered manifest, 64 workers", 3) {
    run_large("large-1-wide.manifest", wide_manifest(), 64, 1500, 18798362, 4500905224359295302ULL);
}

TEST("large-2 two long chains, 4 workers", 3) {
    run_large("large-2-chains.manifest", chain_manifest(), 4, 1500, 100263869, 5892939175592808025ULL);
}

TEST("large-3 one release target over everything, 32 workers", 3) {
    run_large("large-3-fanin.manifest", fanin_manifest(), 32, 1500, 17309132, 15814170778567129945ULL);
}
