#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "pinref.hpp"

struct NetlistError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Part {
    std::string designator;
    int pins = 0;       // pins are numbered 1..pins
    std::string value;  // free text, may be empty
};

struct Net {
    std::string name;
    std::vector<PinRef> pins;  // as listed, at least one
};

struct Netlist {
    std::vector<Part> parts;  // in file order
    std::vector<Net> nets;    // in file order
    std::unordered_map<std::string, int> part_index;  // designator -> index into parts

    // Index of the part, or -1.
    int find_part(std::string_view designator) const;
};

// The schematic export:
//   part <designator> pins=<n> [value]
//   net <name> <pin> <pin> ...
// '#' starts a comment. Throws NetlistError("line N: ...") for malformed lines, repeated
// parts or nets, unknown parts, pins out of range, and a pin that is on more than one net.
Netlist parse_netlist(std::string_view text);
