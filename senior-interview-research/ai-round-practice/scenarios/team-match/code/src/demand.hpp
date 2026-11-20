#pragma once

#include <string>

#include "roster.hpp"

// Demand per team for the program managers, in roster order:
//
//   Teams: 2, seats: 3, interns: 4, interviews: 5
//     payments  2 seats  3 applied  2 first choices  top: cal 4.60, ava 4.20, ben 4.20
//     search    1 seat   2 applied  1 first choice   top: ava 4.80
//
// "top" is the team's three best-ranked interns with their averages, or "-" if it interviewed nobody.
std::string demand_report(const Roster& r);
