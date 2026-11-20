#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "event.hpp"

struct RuleError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Rule {
    std::string name;
    std::string service = "*";  // "*" matches any service
    Level min_level = Level::Error;
    std::string code = "*";     // "*" matches any code
    int threshold = 1;
    std::int64_t window_ms = 0;
    std::int64_t cooldown_ms = 0;
};

// One rule per line, '#' starts a comment. Throws RuleError("line N: ...").
std::vector<Rule> parse_rules(std::string_view text);

// True if the event passes the rule's service, level and code filters.
bool rule_matches(const Rule& rule, const Event& e);
