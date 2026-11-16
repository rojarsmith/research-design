#include "ranking.hpp"

#include <algorithm>

bool ranks_before(const Roster& r, const Interview& a, const Interview& b) {
    double x = average(a.marks), y = average(b.marks);
    if (x != y) return x > y;
    const Intern& ia = r.interns[a.intern];
    const Intern& ib = r.interns[b.intern];
    if (ia.applied != ib.applied) return ia.applied < ib.applied;
    return ia.id < ib.id;
}

std::vector<int> rank_applicants(const Roster& r, int team) {
    std::vector<const Interview*> mine;
    for (const Interview& iv : r.interviews)
        if (iv.team == team) mine.push_back(&iv);
    std::sort(mine.begin(), mine.end(), [&](const Interview* a, const Interview* b) { return ranks_before(r, *a, *b); });
    std::vector<int> out;
    for (const Interview* iv : mine) out.push_back(iv->intern);
    return out;
}
