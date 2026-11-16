#pragma once

#include <vector>

#include "roster.hpp"

struct Placement {
    std::vector<int> team_of;  // per intern, in roster order: team index, or -1 if not placed
};

// Places interns on teams. Not implemented yet.
Placement match_interns(const Roster& r);
