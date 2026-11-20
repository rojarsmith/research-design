#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "netlist.hpp"
#include "pinref.hpp"

struct LayoutError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Trace {
    std::string a, b;  // node names as written: a pin ("U1.12") or a via (a name without '.')
    int line = 0;
};

struct Layout {
    std::vector<Trace> traces;  // in file order
};

// The copper export from the layout tool: one "trace <node> <node>" per line, '#' starts a
// comment. Throws LayoutError("line N: ...") for malformed lines.
Layout parse_layout(std::string_view text);

// The board as a graph. Nodes [0, pin_nodes) are pins: part 0's pins 1..n, then part 1's, and
// so on, in netlist order. Node pin_nodes + i is via i, in the order vias first appear.
struct Board {
    int pin_nodes = 0;
    std::vector<int> first_pin;            // first_pin[p]: node of pin 1 of part p
    std::vector<std::string> vias;         // via names
    std::vector<std::pair<int, int>> edges;  // one per trace, in file order

    int size() const { return pin_nodes + (int)vias.size(); }
    int pin_node(int part, int pin) const { return first_pin[part] + pin - 1; }
};

// Resolves every trace end to a node. Throws LayoutError("line N: ...") for unknown parts, pins
// out of range, and a trace that starts and ends at the same node.
Board load_board(const Netlist& n, const Layout& layout);

// "U1.12" or the via's name.
std::string node_name(const Netlist& n, const Board& b, int node);
