#pragma once

#include <vector>

#include "roster.hpp"

// True if the team that gave both interviews ranks a's intern before b's: higher average
// mark first, then the earlier application, then the smaller intern id (byte order).
bool ranks_before(const Roster& r, const Interview& a, const Interview& b);

// Interns the team interviewed, best first (intern indices).
std::vector<int> rank_applicants(const Roster& r, int team);
