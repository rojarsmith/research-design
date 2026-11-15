#pragma once

#include <string>

#include "graph.hpp"

// Plain-text plan used by the nightly CI summary:
//
//   Build plan: 3 targets
//     1. core  300ms
//     2. util  1.5s
//     3. app   2m 00.0s
//   Critical path: 2m 01.8s
std::string plan_report(const BuildGraph& g);
