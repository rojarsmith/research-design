// Phase 3 tests: company-wide programs. Each test writes its roster to data/generated/ (open
// them: their shapes differ), parses it, and then times match_interns. Parsing is not timed.
#include <string>
#include <vector>

#include "check.hpp"
#include "matcher.hpp"
#include "roster.hpp"

namespace {

std::uint64_t placement_hash(const Placement& p) {
    std::string text;
    for (int t : p.team_of) text += std::to_string(t) + ",";
    return check::fnv1a(text);
}

std::string two(long long v) { return (v < 10 ? "0" : "") + std::to_string(v); }

std::string random_time(check::Rng& rng) {
    long long day = rng.range(1, 28);
    long long hour = rng.range(8, 17);
    long long minute = rng.range(0, 59);
    return "2026-09-" + two(day) + "T" + two(hour) + ":" + two(minute);
}

std::string random_marks(check::Rng& rng, int count) {
    std::string out;
    for (int m = 0; m < count; ++m) {
        long long tenths = rng.range(20, 50);
        out += (m ? "," : "") + std::to_string(tenths / 10) + "." + std::to_string(tenths % 10);
    }
    return out;
}

struct Spec {
    int teams;
    int cap_lo, cap_hi;
    int interns;
    int prefs_lo, prefs_hi;
    int interviewed_percent;
    int spray;         // extra interns who list `spray_prefs` teams but are interviewed by few
    int spray_prefs;
    std::uint64_t seed;
};

std::string roster(const Spec& s) {
    check::Rng rng(s.seed);
    std::string text, marks;
    for (int t = 0; t < s.teams; ++t)
        text += "team t" + std::to_string(t) + " cap=" + std::to_string(rng.range(s.cap_lo, s.cap_hi)) + "\n";
    std::vector<int> order(s.teams);
    for (int t = 0; t < s.teams; ++t) order[t] = t;
    for (int i = 0; i < s.interns + s.spray; ++i) {
        bool spray = i >= s.interns;
        int k = spray ? s.spray_prefs : (int)rng.range(s.prefs_lo, s.prefs_hi);
        std::string id = (spray ? "s" : "i") + std::to_string(i);
        std::string prefs;
        for (int j = 0; j < k; ++j) {
            std::swap(order[j], order[rng.range(j, s.teams - 1)]);
            prefs += (j ? ",t" : "t") + std::to_string(order[j]);
            bool interviewed = spray ? rng.range(0, 399) == 0 : rng.range(0, 99) < s.interviewed_percent;
            if (interviewed) {
                int n = (int)rng.range(1, 3);
                marks += "marks t" + std::to_string(order[j]) + " " + id + " " + random_marks(rng, n) + "\n";
            }
        }
        text += "intern " + id + " applied=" + random_time(rng) + " prefs=" + prefs + "\n";
    }
    return text + marks;
}

void run_large(const std::string& name, const Spec& spec, double limit_ms, int placed, std::uint64_t hash) {
    Roster r = parse_roster(check::read_file(check::write_generated(name + ".roster", roster(spec))));
    Placement p;
    check::measure(limit_ms, [&] { p = match_interns(r); });
    int n = 0;
    for (int t : p.team_of) n += t >= 0;
    EXPECT_EQ(n, placed);
    EXPECT_EQ(placement_hash(p), hash);
}

}  // namespace

TEST("large-1 a dozen very large teams", 3) {
    run_large("large-1-big-teams", {12, 6000, 8000, 110000, 3, 6, 85, 0, 0, 901}, 1500, 84932, 11117167126176767643ULL);
}

TEST("large-2 thousands of small teams", 3) {
    run_large("large-2-many-teams", {6000, 1, 5, 60000, 10, 20, 80, 0, 0, 902}, 1500, 17954, 8401658498129861063ULL);
}

TEST("large-3 applicants who list almost every team", 3) {
    run_large("large-3-spray", {3000, 1, 8, 80000, 3, 8, 85, 1000, 2900, 903}, 1500, 13537, 106756830133116961ULL);
}
