#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "manifest.hpp"

struct GraphError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct BuildGraph {
    std::vector<std::string> names;              // index -> name, in manifest order
    std::vector<std::int64_t> duration_ms;       // index -> duration
    std::vector<std::vector<int>> deps;          // deps[i]: targets that must finish before i starts
    std::vector<std::vector<int>> dependents;    // dependents[j]: targets that list j as a dependency
    std::unordered_map<std::string, int> index;  // name -> index

    int size() const { return (int)names.size(); }
};

// Throws GraphError if a dependency names an unknown target or a target depends on itself.
// Repeated dependencies on the same target are kept once.
BuildGraph build_graph(const Manifest& m);

// Returns one cycle as a list of target indices (first == last), or an empty vector.
std::vector<int> find_cycle(const BuildGraph& g);

// Dependencies first. When several targets are ready, the smallest name goes first.
// Throws GraphError if the graph has a cycle.
std::vector<int> topo_order(const BuildGraph& g);

// Length of the longest dependency chain, summing durations along the chain.
std::int64_t critical_path_ms(const BuildGraph& g);
