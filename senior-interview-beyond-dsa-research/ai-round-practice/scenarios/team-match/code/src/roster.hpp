#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "marks.hpp"

struct RosterError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Team {
    std::string id;
    int cap = 0;  // how many interns it can take
};

struct Intern {
    std::string id;
    std::string applied;     // "2026-09-01T09:30"; the format sorts as text
    std::vector<int> prefs;  // team indices, most wanted first
};

struct Interview {
    int team = 0;
    int intern = 0;
    Marks marks;
};

struct Roster {
    std::vector<Team> teams;            // in file order
    std::vector<Intern> interns;        // in file order
    std::vector<Interview> interviews;  // in file order
    std::unordered_map<std::string, int> team_index, intern_index;

    int find_team(std::string_view id) const;    // -1 if unknown
    int find_intern(std::string_view id) const;  // -1 if unknown
};

// One file, '#' starts a comment:
//   team <id> cap=<n>
//   intern <id> applied=<YYYY-MM-DDTHH:MM> prefs=<team>,<team>,...   (prefs=- for none)
//   marks <team> <intern> <mark>,<mark>,...
// Teams must be declared before interns list them, and interns before their marks.
// Throws RosterError("line N: ...").
Roster parse_roster(std::string_view text);
