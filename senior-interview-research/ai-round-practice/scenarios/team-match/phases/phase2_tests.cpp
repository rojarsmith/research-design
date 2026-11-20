// Phase 2 tests: match_interns.
#include <map>
#include <string>
#include <vector>

#include "check.hpp"
#include "matcher.hpp"
#include "roster.hpp"

namespace {

// "intern->team" in roster order, "-" for interns who were not placed.
std::vector<std::string> placed(const Roster& r, const Placement& p) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < r.interns.size() && i < p.team_of.size(); ++i)
        out.push_back(r.interns[i].id + "->" + (p.team_of[i] < 0 ? std::string("-") : r.teams[p.team_of[i]].id));
    return out;
}

std::vector<std::string> match_text(const std::string& text) {
    Roster r = parse_roster(text);
    return placed(r, match_interns(r));
}

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

std::string random_mark(check::Rng& rng) {
    long long tenths = rng.range(30, 50);
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10);
}

// Small capacities, overlapping lists and marks between 3.0 and 5.0, so ties are common.
std::string random_roster(int teams, int interns, std::uint64_t seed) {
    check::Rng rng(seed);
    std::string text, marks;
    for (int t = 0; t < teams; ++t) text += "team t" + std::to_string(t) + " cap=" + std::to_string(rng.range(0, 4)) + "\n";
    std::vector<int> order(teams);
    for (int i = 0; i < interns; ++i) {
        for (int t = 0; t < teams; ++t) order[t] = t;
        int k = (int)rng.range(0, std::min(6, teams));
        std::string prefs;
        for (int j = 0; j < k; ++j) {
            std::swap(order[j], order[rng.range(j, teams - 1)]);
            prefs += (j ? "," : "") + std::string("t") + std::to_string(order[j]);
        }
        std::string id = "i" + std::to_string(rng.range(0, 999)) + "x" + std::to_string(i);
        text += "intern " + id + " applied=" + random_time(rng) + " prefs=" + (k ? prefs : "-") + "\n";
        for (int j = 0; j < k; ++j) {
            if (rng.range(0, 3) == 0) continue;  // not interviewed
            int n = (int)rng.range(1, 3);
            std::string list;
            for (int m = 0; m < n; ++m) list += (m ? "," : "") + random_mark(rng);
            marks += "marks t" + std::to_string(order[j]) + " " + id + " " + list + "\n";
        }
    }
    return text + marks;
}

}  // namespace

TEST("the spring cohort", 2) {
    EXPECT_EQ(match_text(check::read_file("data/spring.roster")),
              (std::vector<std::string>{"ava->payments", "ben->payments", "cal->search", "dan->infra", "eve->infra", "fay->-",
                                        "gus->-"}));
}

TEST("a stronger applicant bumps the weakest one", 2) {
    EXPECT_EQ(match_text("team a cap=1\nteam b cap=1\n"
                         "intern x applied=2026-09-01T09:00 prefs=a,b\n"
                         "intern y applied=2026-09-02T09:00 prefs=a\n"
                         "marks a x 3.0\nmarks a y 4.0\nmarks b x 3.0\n"),
              (std::vector<std::string>{"x->b", "y->a"}));
}

TEST("no marks means no place, and cap=0 takes nobody", 2) {
    EXPECT_EQ(match_text("team a cap=5\nteam frozen cap=0\n"
                         "intern x applied=2026-09-01T09:00 prefs=frozen,a\n"
                         "intern y applied=2026-09-01T09:00 prefs=a,frozen\n"
                         "marks frozen x 5.0\nmarks a y 2.0\nmarks frozen y 5.0\n"),
              (std::vector<std::string>{"x->-", "y->a"}));
}

TEST("equal averages go to the earlier application, then the smaller id", 2) {
    EXPECT_EQ(match_text("team a cap=1\nteam b cap=1\n"
                         "intern p applied=2026-09-02T09:00 prefs=a\n"
                         "intern q applied=2026-09-01T09:00 prefs=a\n"
                         "intern zed applied=2026-09-01T08:00 prefs=b\n"
                         "intern amy applied=2026-09-01T08:00 prefs=b\n"
                         "marks a p 3.6,4.8\nmarks a q 4.1,4.3\nmarks b zed 4.0\nmarks b amy 3.9,4.1\n"),
              (std::vector<std::string>{"p->-", "q->a", "zed->-", "amy->b"}));
}

TEST("the result does not depend on the order of the roster", 2) {
    std::string teams = "team a cap=1\nteam b cap=1\nteam c cap=1\n";
    std::vector<std::string> interns = {
        "intern u applied=2026-09-01T09:00 prefs=a,b,c\n", "intern v applied=2026-09-01T10:00 prefs=b,a,c\n",
        "intern w applied=2026-09-01T11:00 prefs=a,c,b\n", "intern z applied=2026-09-01T12:00 prefs=c,a\n"};
    std::string marks =
        "marks a u 3.0\nmarks b u 4.0\nmarks c u 5.0\nmarks b v 3.5\nmarks a v 4.5\nmarks c v 3.0\n"
        "marks a w 4.0\nmarks c w 4.5\nmarks b w 2.0\nmarks c z 3.5\nmarks a z 5.0\n";
    std::map<std::string, std::string> first;
    for (int rot = 0; rot < 4; ++rot) {
        std::string text = teams;
        for (int k = 0; k < 4; ++k) text += interns[(rot + k) % 4];
        std::map<std::string, std::string> got;
        for (const std::string& s : match_text(text + marks)) got[s.substr(0, s.find('-'))] = s;
        if (rot == 0) first = got;
        EXPECT_EQ(got.size(), std::size_t(4));
        if (got != first) check::fail(__FILE__, __LINE__, "the placement changed when the interns were listed in another order");
    }
    EXPECT_EQ(first["u"], std::string("u->b"));
}

TEST("empty rosters and empty lists", 2) {
    EXPECT_TRUE(match_text("").empty());
    EXPECT_EQ(match_text("team a cap=3\nintern x applied=2026-09-01T09:00 prefs=-\n"), std::vector<std::string>{"x->-"});
}

TEST("medium random rosters", 2) {
    struct Case { int teams; int interns; std::uint64_t seed; int placed; std::uint64_t hash; };
    const Case cases[] = {
        {12, 40, 81, 23, 18322264409143898731ULL},
        {400, 2000, 82, 750, 17853151708000318626ULL},
    };
    for (const Case& c : cases) {
        Roster r = parse_roster(random_roster(c.teams, c.interns, c.seed));
        Placement p = match_interns(r);
        int n = 0;
        for (int t : p.team_of) n += t >= 0;
        EXPECT_EQ(n, c.placed);
        EXPECT_EQ(placement_hash(p), c.hash);
    }
}
