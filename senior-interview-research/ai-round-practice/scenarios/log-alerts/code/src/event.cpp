#include "event.hpp"

#include "timeparse.hpp"

std::optional<Level> parse_level(std::string_view text) {
    if (text == "DEBUG") return Level::Debug;
    if (text == "INFO") return Level::Info;
    if (text == "WARN") return Level::Warn;
    if (text == "ERROR") return Level::Error;
    if (text == "FATAL") return Level::Fatal;
    return std::nullopt;
}

const char* level_name(Level level) {
    switch (level) {
        case Level::Debug: return "DEBUG";
        case Level::Info: return "INFO";
        case Level::Warn: return "WARN";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
    }
    return "?";
}

namespace {

// Next space-separated word starting at pos; advances pos past trailing spaces.
std::string_view next_word(std::string_view line, std::size_t& pos) {
    std::size_t start = pos;
    while (pos < line.size() && line[pos] != ' ') ++pos;
    std::string_view word = line.substr(start, pos - start);
    while (pos < line.size() && line[pos] == ' ') ++pos;
    return word;
}

}  // namespace

std::optional<Event> parse_event(std::string_view line) {
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    std::size_t pos = 0;
    std::string_view ts = next_word(line, pos);
    std::string_view service = next_word(line, pos);
    std::string_view level = next_word(line, pos);
    std::string_view code = next_word(line, pos);
    if (ts.empty() || service.empty() || level.empty() || code.empty()) return std::nullopt;

    Event e;
    try {
        e.ts_ms = parse_timestamp_ms(ts);
    } catch (const TimeError&) {
        return std::nullopt;
    }
    auto lv = parse_level(level);
    if (!lv) return std::nullopt;
    e.level = *lv;
    e.service = std::string(service);
    e.code = std::string(code);
    e.message = std::string(line.substr(pos));
    return e;
}

LogFile parse_log(std::string_view text) {
    LogFile f;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) nl = text.size();
        std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        if (line.empty() || line == "\r") continue;
        if (auto e = parse_event(line)) f.events.push_back(std::move(*e));
        else ++f.malformed;
    }
    return f;
}
