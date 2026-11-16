#include "pinref.hpp"

#include <cctype>

namespace {

bool digit(char c) { return c >= '0' && c <= '9'; }

}  // namespace

bool is_designator(std::string_view s) {
    std::size_t i = 0;
    while (i < s.size() && s[i] >= 'A' && s[i] <= 'Z') ++i;
    if (i == 0 || i == s.size()) return false;
    for (; i < s.size(); ++i)
        if (!digit(s[i])) return false;
    return true;
}

PinRef parse_pin(std::string_view text) {
    auto dot = text.find('.');
    if (dot == std::string_view::npos) throw PinError("expected <part>.<pin>, got '" + std::string(text) + "'");
    std::string_view part = text.substr(0, dot), pin = text.substr(dot + 1);
    if (!is_designator(part)) throw PinError("bad designator in '" + std::string(text) + "'");
    if (pin.empty() || pin.size() > 4 || pin[0] == '0') throw PinError("bad pin number in '" + std::string(text) + "'");
    int n = 0;
    for (char c : pin) {
        if (!digit(c)) throw PinError("bad pin number in '" + std::string(text) + "'");
        n = n * 10 + (c - '0');
    }
    return {std::string(part), n};
}

std::string format_pin(const PinRef& p) { return p.part + "." + std::to_string(p.pin); }

bool natural_less(std::string_view a, std::string_view b) {
    std::size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (digit(a[i]) && digit(b[j])) {
            std::size_t i0 = i, j0 = j;
            while (i0 < a.size() && a[i0] == '0') ++i0;  // leading zeros do not count
            while (j0 < b.size() && b[j0] == '0') ++j0;
            std::size_t i1 = i0, j1 = j0;
            while (i1 < a.size() && digit(a[i1])) ++i1;
            while (j1 < b.size() && digit(b[j1])) ++j1;
            if (i1 - i0 != j1 - j0) return i1 - i0 < j1 - j0;  // fewer digits, smaller number
            int c = a.substr(i0, i1 - i0).compare(b.substr(j0, j1 - j0));
            if (c != 0) return c < 0;
            i = i1;
            j = j1;
        } else {
            if (a[i] != b[j]) return (unsigned char)a[i] < (unsigned char)b[j];
            ++i;
            ++j;
        }
    }
    if (i == a.size() && j == b.size()) return a < b;  // equal apart from leading zeros
    return i == a.size();
}

std::string pin_key(const PinRef& p) { return p.part + std::to_string(p.pin); }
