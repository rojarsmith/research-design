#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "clock.hpp"

struct CatalogError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Feature bits.
enum Feature : unsigned { Tv = 1, Vc = 2, Whiteboard = 4, Accessible = 8 };
constexpr int kFeatureCount = 4;

// "tv,vc" -> Tv | Vc. "-" -> 0. Throws CatalogError for unknown names.
unsigned parse_features(std::string_view text);
std::string format_features(unsigned mask);

struct Room {
    std::string id;
    int capacity = 0;
    unsigned features = 0;
};

struct Request {
    std::string id;
    Minutes start = 0;
    Minutes end = 0;
    int size = 1;
    unsigned needs = 0;
};

struct ManualBooking {
    std::string room;
    Minutes start = 0;
    Minutes end = 0;
    std::string label;
};

// All three parsers skip blank lines and '#' comments, and throw CatalogError("line N: ...").
std::vector<Room> parse_rooms(std::string_view text);
std::vector<Request> parse_requests(std::string_view text);
std::vector<ManualBooking> parse_bookings(std::string_view text);
