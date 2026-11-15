#pragma once

#include <string>
#include <vector>

#include "event.hpp"

// On-call summary for a merged timeline:
//
//   Events: 12
//   From 2026-11-15T10:00:00.120Z to 2026-11-15T10:00:04.000Z
//   By level: DEBUG 0, INFO 5, WARN 1, ERROR 5, FATAL 1
//   Top error codes: E5003 x3, E4100 x2, F9000 x1
//
// "Top error codes" counts ERROR and FATAL events, shows at most three codes,
// most frequent first, ties by code.
std::string summary_report(const std::vector<Event>& timeline);
