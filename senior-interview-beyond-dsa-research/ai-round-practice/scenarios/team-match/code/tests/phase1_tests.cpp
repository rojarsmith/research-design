// Existing tests for teammatch. Do not edit them to make them pass.
#include <string>
#include <vector>

#include "check.hpp"
#include "demand.hpp"
#include "marks.hpp"
#include "ranking.hpp"
#include "roster.hpp"

namespace {

template <class E, class F>
std::string error_of(F&& f) {
    try {
        f();
    } catch (const E& e) {
        return e.what();
    }
    return "(no error)";
}

Roster spring() { return parse_roster(check::read_file("data/spring.roster")); }

std::vector<std::string> ranked_ids(const Roster& r, const char* team) {
    std::vector<std::string> out;
    for (int i : rank_applicants(r, r.find_team(team))) out.push_back(r.interns[i].id);
    return out;
}

}  // namespace

TEST("marks have one decimal and print with two", 1) {
    EXPECT_EQ(format_average(parse_marks("4.1,4.3")), std::string("4.20"));
    EXPECT_EQ(format_average(parse_marks("4.7")), std::string("4.70"));
    EXPECT_EQ(format_average(parse_marks("3.3,3.4,3.4")), std::string("3.37"));
    EXPECT_EQ(parse_marks("5.0,1.0").size(), std::size_t(2));
    for (const char* bad : {"4.15", "5.1", "0.9", "4,", "", "4.1,,4.2", "10.0", "4.1,4.1,4.1,4.1,4.1,4.1,4.1,4.1,4.1,4.1"}) {
        bool threw = false;
        try { parse_marks(bad); } catch (const MarksError&) { threw = true; }
        if (!threw) check::fail(__FILE__, __LINE__, std::string("accepted bad marks \"") + bad + "\"");
    }
}

TEST("parse_roster reads teams, interns and marks", 1) {
    Roster r = spring();
    EXPECT_EQ(r.teams.size(), std::size_t(4));
    EXPECT_EQ(r.interns.size(), std::size_t(7));
    EXPECT_EQ(r.interviews.size(), std::size_t(16));
    EXPECT_EQ(r.teams[3].cap, 0);
    EXPECT_EQ(r.interns[2].prefs, (std::vector<int>{1, 0, 2}));
    EXPECT_EQ(r.interns[5].applied, std::string("2026-09-05T10:00"));
    EXPECT_EQ(r.find_team("infra"), 2);
    EXPECT_EQ(r.find_intern("nobody"), -1);
    EXPECT_EQ(r.interviews[2].team, 0);
    EXPECT_EQ(r.interviews[2].intern, 2);
}

TEST("parse_roster reports the failing line", 1) {
    auto err = [](const std::string& text) { return error_of<RosterError>([&] { parse_roster(text); }); };
    const std::string a = "team a cap=1\n", x = "intern x applied=2026-09-01T09:30 prefs=a\n";
    EXPECT_EQ(err(a + "team a cap=2\n"), std::string("line 2: team a is declared twice"));
    EXPECT_EQ(err(a + "intern x applied=2026-09-01T09:30 prefs=b\n"), std::string("line 2: unknown team 'b'"));
    EXPECT_EQ(err(a + "intern x applied=2026-09-01T09:30 prefs=a,a\n"), std::string("line 2: x lists a twice"));
    EXPECT_EQ(err(a + "intern x applied=2026-09-01 prefs=a\n"), std::string("line 2: bad time '2026-09-01'"));
    EXPECT_EQ(err(a + "team b cap=1\n" + x + "marks b x 4.0\n"), std::string("line 4: x did not apply to b"));
    EXPECT_EQ(err(a + x + "marks a x 4.0\nmarks a x 3.0\n"), std::string("line 4: x already has marks from a"));
    EXPECT_EQ(err(a + x + "marks a x 4.25\n"), std::string("line 3: bad mark '4.25' in '4.25'"));
    EXPECT_EQ(err("team a cap=x\n"), std::string("line 1: bad capacity 'x'"));
}

TEST("an intern may list no teams", 1) {
    Roster r = parse_roster("team a cap=1\nintern x applied=2026-09-01T09:30 prefs=-\n");
    EXPECT_TRUE(r.interns[0].prefs.empty());
    EXPECT_EQ(ranked_ids(r, "a"), std::vector<std::string>{});
}

TEST("a team ranks by average, then application time, then id", 1) {
    Roster r = spring();
    EXPECT_EQ(ranked_ids(r, "payments"), (std::vector<std::string>{"cal", "ava", "ben", "eve", "gus"}));
    EXPECT_EQ(ranked_ids(r, "search"), (std::vector<std::string>{"ava", "cal", "dan", "fay"}));
    EXPECT_EQ(ranked_ids(r, "infra"), (std::vector<std::string>{"eve", "dan", "cal", "ben", "fay"}));
    EXPECT_EQ(ranked_ids(r, "mobile"), (std::vector<std::string>{"eve", "gus"}));
}

TEST("demand report for the spring cohort", 1) {
    EXPECT_EQ(demand_report(spring()), std::string(
        "Teams: 4, seats: 5, interns: 7, interviews: 16\n"
        "  payments  2 seats  5 applied  3 first choices  top: cal 4.60, ava 4.20, ben 4.20\n"
        "  search    1 seat   4 applied  2 first choices  top: ava 4.80, cal 3.90, dan 3.90\n"
        "  infra     2 seats  5 applied  1 first choice   top: eve 4.50, dan 4.40, cal 4.20\n"
        "  mobile    0 seats  2 applied  1 first choice   top: eve 5.00, gus 4.90\n"));
}
