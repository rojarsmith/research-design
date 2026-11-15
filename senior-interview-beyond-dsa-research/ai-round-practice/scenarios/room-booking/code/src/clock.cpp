#include "clock.hpp"

#include <cctype>
#include <cstdio>

Minutes parse_clock(std::string_view s) {
    auto colon = s.find(':');
    if (colon == std::string_view::npos || colon == 0 || colon > 2 || s.size() != colon + 3)
        throw ClockError("bad time '" + std::string(s) + "'");
    for (std::size_t i = 0; i < s.size(); ++i)
        if (i != colon && !std::isdigit((unsigned char)s[i])) throw ClockError("bad time '" + std::string(s) + "'");
    int h = std::stoi(std::string(s.substr(0, colon)));
    int m = std::stoi(std::string(s.substr(colon + 1)));
    if (m > 59 || h > 24 || (h == 24 && m != 0)) throw ClockError("bad time '" + std::string(s) + "'");
    return h * 60 + m;
}

std::string format_clock(Minutes m) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "%02d:%02d", m / 60, m % 60);
    return buf;
}

std::pair<Minutes, Minutes> parse_span(std::string_view s) {
    auto dash = s.find('-');
    if (dash == std::string_view::npos) throw ClockError("bad span '" + std::string(s) + "'");
    Minutes start = parse_clock(s.substr(0, dash));
    Minutes end = parse_clock(s.substr(dash + 1));
    if (end <= start) throw ClockError("span ends before it starts: '" + std::string(s) + "'");
    return {start, end};
}
