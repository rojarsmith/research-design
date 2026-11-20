#include "alerts.hpp"

#include <stdexcept>

std::vector<Alert> evaluate(const std::vector<Event>& timeline, const std::vector<Rule>& rules) {
    (void)timeline;
    (void)rules;
    throw std::logic_error("evaluate: not implemented");
}
