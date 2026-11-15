#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

struct ClockError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

using Minutes = int;  // minutes since 00:00, 0..1440

// "09:30" or "9:30" -> 570. "24:00" -> 1440. Throws ClockError otherwise.
Minutes parse_clock(std::string_view text);

// 570 -> "09:30"
std::string format_clock(Minutes m);

// "09:30-10:15" -> {570, 615}. Throws ClockError if end <= start.
std::pair<Minutes, Minutes> parse_span(std::string_view text);
