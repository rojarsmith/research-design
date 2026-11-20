#pragma once

#include <string>
#include <vector>

#include "layout.hpp"
#include "netlist.hpp"

struct Open {
    std::string net;
    std::vector<std::vector<std::string>> islands;  // pins as "U1.12"

    bool operator==(const Open&) const = default;
};

struct Short {
    std::vector<std::string> nets;

    bool operator==(const Short&) const = default;
};

struct CheckResult {
    std::vector<Open> opens;
    std::vector<Short> shorts;
};

// Compares the schematic with the copper on the board. Not implemented yet.
CheckResult check_board(const Netlist& n, const Board& b);
