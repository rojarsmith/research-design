#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

struct DurationError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Parses "300ms", "1.5s", "2m", "1m30s", "0.25h" into milliseconds.
// Throws DurationError for empty input, unknown units or a missing number.
std::int64_t parse_duration_ms(std::string_view text);

// Formats milliseconds for reports:
//   below 1 s       -> "450ms"
//   below 1 minute  -> "1.5s"   (one decimal, rounded down)
//   otherwise       -> "2m 03.4s"
std::string format_duration(std::int64_t ms);
