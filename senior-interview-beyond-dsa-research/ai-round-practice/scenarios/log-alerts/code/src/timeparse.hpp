#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

struct TimeError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// "2026-11-15T10:00:00Z", "2026-11-15T10:00:00.5Z", "2026-11-15T10:00:00.480Z",
// "2026-11-15T10:00:00.700500Z" -> milliseconds since the Unix epoch (UTC).
// Throws TimeError on anything else.
std::int64_t parse_timestamp_ms(std::string_view text);

// Always prints three fraction digits: "2026-11-15T10:00:00.480Z".
std::string format_timestamp(std::int64_t ms);
