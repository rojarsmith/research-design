#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "netlist.hpp"

// Pins of the part that are on no net, ascending. Throws NetlistError for an unknown part.
std::vector<int> unconnected_pins(const Netlist& n, std::string_view designator);

// Pin usage for the schematic review:
//
//   Parts: 3, nets: 2, pins on nets: 5 of 12
//     R1     2 pins, all on nets
//     TP1    1 pin, all on nets
//     U1     9 pins, 6 not on any net: 2-5, 7, 9
//   Single-pin nets: TP
//
// Parts in natural order. Pins that are on no net are listed as ranges.
std::string pin_report(const Netlist& n);
