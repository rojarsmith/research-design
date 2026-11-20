#include "catalog.hpp"

#include <cctype>
#include <functional>

namespace {

const char* kNames[kFeatureCount] = {"tv", "vc", "whiteboard", "accessible"};

std::vector<std::string_view> words_of(std::string_view line) {
    std::vector<std::string_view> out;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && std::isspace((unsigned char)line[i])) ++i;
        std::size_t start = i;
        while (i < line.size() && !std::isspace((unsigned char)line[i])) ++i;
        if (i > start) out.push_back(line.substr(start, i - start));
    }
    return out;
}

std::string_view value_of(std::string_view word, std::string_view key) {
    if (word.size() <= key.size() || word.substr(0, key.size()) != key || word[key.size()] != '=')
        throw CatalogError("expected " + std::string(key) + "=..., got '" + std::string(word) + "'");
    return word.substr(key.size() + 1);
}

int positive_int(std::string_view text) {
    if (text.empty() || text.size() > 6) throw CatalogError("bad number '" + std::string(text) + "'");
    for (char c : text)
        if (!std::isdigit((unsigned char)c)) throw CatalogError("bad number '" + std::string(text) + "'");
    int v = std::stoi(std::string(text));
    if (v < 1) throw CatalogError("number must be at least 1");
    return v;
}

// Calls fn(words) for each non-empty line; prefixes errors with the line number.
void for_each_line(std::string_view text, std::string_view keyword, std::size_t nwords,
                   const std::function<void(const std::vector<std::string_view>&)>& fn) {
    int line_no = 0;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) nl = text.size();
        std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        ++line_no;
        if (auto h = line.find('#'); h != std::string_view::npos) line = line.substr(0, h);
        auto w = words_of(line);
        if (w.empty()) continue;
        try {
            if (w[0] != keyword || w.size() != nwords)
                throw CatalogError("expected a '" + std::string(keyword) + "' line with " + std::to_string(nwords - 1) + " fields");
            fn(w);
        } catch (const std::exception& e) {
            throw CatalogError("line " + std::to_string(line_no) + ": " + e.what());
        }
    }
}

}  // namespace

unsigned parse_features(std::string_view text) {
    if (text == "-") return 0;
    unsigned mask = 0;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t comma = text.find(',', start);
        if (comma == std::string_view::npos) comma = text.size();
        std::string_view name = text.substr(start, comma - start);
        bool found = false;
        for (int i = 0; i < kFeatureCount; ++i)
            if (name == kNames[i]) { mask |= 1u << i; found = true; }
        if (!found) throw CatalogError("unknown feature '" + std::string(name) + "'");
        start = comma + 1;
    }
    return mask;
}

std::string format_features(unsigned mask) {
    std::string out;
    for (int i = 0; i < kFeatureCount; ++i)
        if (mask & (1u << i)) out += (out.empty() ? "" : ",") + std::string(kNames[i]);
    return out.empty() ? "-" : out;
}

std::vector<Room> parse_rooms(std::string_view text) {
    std::vector<Room> rooms;
    for_each_line(text, "room", 4, [&](const auto& w) {
        rooms.push_back({std::string(w[1]), positive_int(value_of(w[2], "cap")), parse_features(value_of(w[3], "features"))});
    });
    return rooms;
}

std::vector<Request> parse_requests(std::string_view text) {
    std::vector<Request> out;
    for_each_line(text, "request", 5, [&](const auto& w) {
        auto [start, end] = parse_span(w[2]);
        out.push_back({std::string(w[1]), start, end, positive_int(value_of(w[3], "size")), parse_features(value_of(w[4], "needs"))});
    });
    return out;
}

std::vector<ManualBooking> parse_bookings(std::string_view text) {
    std::vector<ManualBooking> out;
    for_each_line(text, "booking", 4, [&](const auto& w) {
        auto [start, end] = parse_span(w[2]);
        out.push_back({std::string(w[1]), start, end, std::string(w[3])});
    });
    return out;
}
