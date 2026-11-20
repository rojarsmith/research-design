#include "demand.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "ranking.hpp"

namespace {

std::string count(int n, const char* one, const char* many) { return std::to_string(n) + " " + (n == 1 ? one : many); }

}  // namespace

std::string demand_report(const Roster& r) {
    std::vector<int> applied(r.teams.size(), 0), first(r.teams.size(), 0);
    int seats = 0;
    for (const Team& t : r.teams) seats += t.cap;
    for (const Intern& in : r.interns) {
        for (int t : in.prefs) ++applied[t];
        if (!in.prefs.empty()) ++first[in.prefs[0]];
    }
    // Marks of each interview, to print the averages.
    std::vector<std::vector<const Interview*>> by_team(r.teams.size());
    for (const Interview& iv : r.interviews) by_team[iv.team].push_back(&iv);

    std::size_t width = 0;
    for (const Team& t : r.teams) width = std::max(width, t.id.size());
    std::ostringstream out;
    out << "Teams: " << r.teams.size() << ", seats: " << seats << ", interns: " << r.interns.size()
        << ", interviews: " << r.interviews.size() << "\n";
    for (int t = 0; t < (int)r.teams.size(); ++t) {
        out << "  " << std::left << std::setw((int)width) << r.teams[t].id << "  " << std::setw(9)
            << count(r.teams[t].cap, "seat", "seats") << std::setw(11) << count(applied[t], "applied", "applied")
            << std::setw(17) << count(first[t], "first choice", "first choices") << "top:";
        std::vector<int> ranked = rank_applicants(r, t);
        if (ranked.empty()) out << " -";
        for (std::size_t k = 0; k < ranked.size() && k < 3; ++k) {
            const Interview* iv = nullptr;
            for (const Interview* x : by_team[t])
                if (x->intern == ranked[k]) iv = x;
            out << (k ? ", " : " ") << r.interns[ranked[k]].id << " " << format_average(iv->marks);
        }
        out << "\n";
    }
    return out.str();
}
