#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "event.hpp"
#include "rules.hpp"

struct Alert {
    std::string rule;
    std::string service;
    std::string code;
    std::int64_t at_ms = 0;  // timestamp of the event that fired the alert
    int count = 0;           // matching events in the window at that moment

    bool operator==(const Alert&) const = default;
};

// Evaluates alert rules over a merged timeline. Not implemented yet.
std::vector<Alert> evaluate(const std::vector<Event>& timeline, const std::vector<Rule>& rules);
