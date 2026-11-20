// teammatch report <roster>
// teammatch match <roster>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "demand.hpp"
#include "matcher.hpp"
#include "roster.hpp"

static std::string slurp(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error(std::string("cannot open ") + path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    std::string mode = argc == 3 ? argv[1] : "";
    if (mode != "report" && mode != "match") {
        std::cerr << "usage: teammatch report <roster>\n       teammatch match <roster>\n";
        return 2;
    }
    try {
        Roster r = parse_roster(slurp(argv[2]));
        if (mode == "report") {
            std::cout << demand_report(r);
            return 0;
        }
        Placement p = match_interns(r);
        int placed = 0;
        for (std::size_t i = 0; i < r.interns.size(); ++i) {
            int t = p.team_of[i];
            std::cout << r.interns[i].id << " -> " << (t < 0 ? "(not placed)" : r.teams[t].id) << "\n";
            placed += t >= 0;
        }
        std::cout << placed << " of " << r.interns.size() << " interns placed\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
