#include "accesslog.hpp"

#include <cctype>
#include <optional>

namespace {

std::optional<std::int64_t> number(std::string_view s, std::size_t max_digits) {
    if (s.empty() || s.size() > max_digits) return std::nullopt;
    std::int64_t v = 0;
    for (char c : s) {
        if (!std::isdigit((unsigned char)c)) return std::nullopt;
        v = v * 10 + (c - '0');
    }
    return v;
}

bool is_method_word(std::string_view s) {
    if (s.empty() || s.size() > 10) return false;
    for (char c : s)
        if (!std::isupper((unsigned char)c)) return false;
    return true;
}

}  // namespace

AccessLog parse_access_log(std::string_view text) {
    AccessLog log;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) nl = text.size();
        std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.empty()) continue;

        std::string_view w[6];
        int n = 0;
        std::size_t i = 0;
        while (i < line.size() && n < 6) {
            while (i < line.size() && line[i] == ' ') ++i;
            std::size_t start = i;
            while (i < line.size() && line[i] != ' ') ++i;
            if (i > start) w[n++] = line.substr(start, i - start);
        }
        auto status = n == 5 ? number(w[2], 3) : std::nullopt;
        auto bytes = n == 5 ? number(w[3], 15) : std::nullopt;
        auto ms = n == 5 ? number(w[4], 7) : std::nullopt;
        if (n != 5 || !is_method_word(w[0]) || !status || *status < 100 || *status > 599 || !bytes || !ms) {
            ++log.malformed;
            continue;
        }
        log.requests.push_back({std::string(w[0]), std::string(w[1]), (int)*status, *bytes, (int)*ms});
    }
    return log;
}
