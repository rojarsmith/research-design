#include "duration.hpp"

#include <cctype>
#include <cstdio>

namespace {

std::int64_t unit_ms(std::string_view unit) {
    if (unit == "ms") return 1;
    if (unit == "s") return 1000;
    if (unit == "m") return 60 * 1000;
    if (unit == "h") return 60 * 60 * 1000;
    throw DurationError("unknown duration unit '" + std::string(unit) + "'");
}

}  // namespace

std::int64_t parse_duration_ms(std::string_view text) {
    if (text.empty()) throw DurationError("empty duration");
    std::int64_t total = 0;
    std::size_t i = 0;
    while (i < text.size()) {
        std::size_t start = i;
        while (i < text.size() && (std::isdigit((unsigned char)text[i]) || text[i] == '.')) ++i;
        if (i == start) throw DurationError("expected a number in '" + std::string(text) + "'");
        std::string number(text.substr(start, i - start));

        std::size_t unit_start = i;
        while (i < text.size() && std::isalpha((unsigned char)text[i])) ++i;
        if (i == unit_start) throw DurationError("missing unit in '" + std::string(text) + "'");
        std::int64_t scale = unit_ms(text.substr(unit_start, i - unit_start));

        long long value = std::stoll(number);
        total += value * scale;
    }
    return total;
}

std::string format_duration(std::int64_t ms) {
    char buf[64];
    if (ms < 1000) {
        std::snprintf(buf, sizeof buf, "%lldms", (long long)ms);
    } else if (ms < 60 * 1000) {
        std::snprintf(buf, sizeof buf, "%lld.%llds", (long long)(ms / 1000), (long long)(ms % 1000 / 100));
    } else {
        long long minutes = ms / 60000;
        long long rest = ms % 60000;
        std::snprintf(buf, sizeof buf, "%lldm %02lld.%llds", minutes, rest / 1000, rest % 1000 / 100);
    }
    return buf;
}
