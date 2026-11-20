#pragma once

#include <cstdint>
#include <vector>

#include "graph.hpp"

struct Schedule {
    std::int64_t makespan_ms = 0;        // when the last target finishes
    std::vector<std::int64_t> start_ms;  // start_ms[i]: when target i starts
    std::vector<int> order;              // target indices in the order they were started
};

// Simulates a build on `workers` parallel workers. Not implemented yet.
Schedule simulate(const BuildGraph& g, int workers);
