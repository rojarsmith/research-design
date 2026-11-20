#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct ManifestError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Target {
    std::string name;
    std::int64_t duration_ms = 0;
    std::vector<std::string> deps;  // as written, duplicates possible
    int line = 0;                   // 1-based line in the manifest
};

struct Manifest {
    std::vector<Target> targets;  // in file order
};

// Throws ManifestError("line N: ...") on malformed lines or duplicate target names.
// Unknown dependencies are reported later, by build_graph.
Manifest parse_manifest(std::string_view text);
