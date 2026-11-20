#include "timeparse.hpp"

#include <cctype>
#include <cstdio>

namespace {

// Days since 1970-01-01 for a proleptic Gregorian date (H. Hinnant's algorithm).
std::int64_t days_from_civil(std::int64_t y, int m, int d) {
    y -= m <= 2;
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const std::int64_t yoe = y - era * 400;
    const std::int64_t doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const std::int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

void civil_from_days(std::int64_t z, int& y, int& m, int& d) {
    z += 719468;
    const std::int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const std::int64_t doe = z - era * 146097;
    const std::int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const std::int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const std::int64_t mp = (5 * doy + 2) / 153;
    d = (int)(doy - (153 * mp + 2) / 5 + 1);
    m = (int)(mp < 10 ? mp + 3 : mp - 9);
    y = (int)(yoe + era * 400 + (m <= 2));
}

int read_int(std::string_view s, std::size_t& pos, int digits) {
    if (pos + digits > s.size()) throw TimeError("timestamp too short");
    int v = 0;
    for (int i = 0; i < digits; ++i) {
        char c = s[pos++];
        if (!std::isdigit((unsigned char)c)) throw TimeError("expected a digit in timestamp");
        v = v * 10 + (c - '0');
    }
    return v;
}

void expect(std::string_view s, std::size_t& pos, char c) {
    if (pos >= s.size() || s[pos] != c) throw TimeError(std::string("expected '") + c + "' in timestamp");
    ++pos;
}

}  // namespace

std::int64_t parse_timestamp_ms(std::string_view s) {
    std::size_t pos = 0;
    int year = read_int(s, pos, 4);
    expect(s, pos, '-');
    int month = read_int(s, pos, 2);
    expect(s, pos, '-');
    int day = read_int(s, pos, 2);
    expect(s, pos, 'T');
    int hour = read_int(s, pos, 2);
    expect(s, pos, ':');
    int minute = read_int(s, pos, 2);
    expect(s, pos, ':');
    int second = read_int(s, pos, 2);
    if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59)
        throw TimeError("timestamp field out of range");

    int millis = 0;
    if (pos < s.size() && s[pos] == '.') {
        std::size_t start = ++pos;
        while (pos < s.size() && std::isdigit((unsigned char)s[pos])) ++pos;
        if (pos == start) throw TimeError("empty fraction in timestamp");
        millis = std::stoi(std::string(s.substr(start, pos - start)));
    }
    expect(s, pos, 'Z');
    if (pos != s.size()) throw TimeError("trailing characters after timestamp");

    std::int64_t days = days_from_civil(year, month, day);
    return ((days * 24 + hour) * 60 + minute) * 60000 + second * 1000 + millis;
}

std::string format_timestamp(std::int64_t ms) {
    std::int64_t days = ms >= 0 ? ms / 86400000 : (ms - 86399999) / 86400000;
    std::int64_t rest = ms - days * 86400000;
    int y, m, d;
    civil_from_days(days, y, m, d);
    char buf[40];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", y, m, d, (int)(rest / 3600000),
                  (int)(rest / 60000 % 60), (int)(rest / 1000 % 60), (int)(rest % 1000));
    return buf;
}
