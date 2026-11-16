#include "connectivity.hpp"

#include <stdexcept>

CheckResult check_board(const Netlist& n, const Board& b) {
    (void)n;
    (void)b;
    throw std::logic_error("check_board: not implemented");
}
