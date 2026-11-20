#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class Level { Debug, Info, Warn, Error, Fatal };

// "DEBUG" ... "FATAL"; nullopt for anything else.
std::optional<Level> parse_level(std::string_view text);
const char* level_name(Level level);

struct Event {
    std::int64_t ts_ms = 0;
    std::string service;
    Level level = Level::Info;
    std::string code;
    std::string message;
};

// nullopt if the line is not a valid event.
std::optional<Event> parse_event(std::string_view line);

struct LogFile {
    std::vector<Event> events;  // in line order
    int malformed = 0;          // non-empty lines that did not parse
};

LogFile parse_log(std::string_view text);
