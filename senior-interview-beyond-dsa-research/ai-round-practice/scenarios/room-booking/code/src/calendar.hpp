#pragma once

#include <string>
#include <vector>

#include "catalog.hpp"
#include "clock.hpp"

struct Booking {
    Minutes start = 0;
    Minutes end = 0;  // exclusive
    std::string label;
};

// True if the two bookings share at least one minute.
bool overlaps(const Booking& a, const Booking& b);

// Bookings for each room, kept sorted by start time.
class Calendar {
public:
    explicit Calendar(int rooms) : by_room_(rooms) {}

    bool is_free(int room, Minutes start, Minutes end) const;
    void book(int room, Booking b);
    const std::vector<Booking>& bookings(int room) const { return by_room_[room]; }

private:
    std::vector<std::vector<Booking>> by_room_;
};

struct Conflict {
    std::string room;
    std::string first;   // label of the earlier booking
    std::string second;  // label of the later booking
    bool operator==(const Conflict&) const = default;
};

// Every pair of manual bookings in the same room that overlap, sorted by room id,
// then by the start of the first booking, then the start of the second.
// Throws CatalogError if a booking names an unknown room.
std::vector<Conflict> find_conflicts(const std::vector<Room>& rooms, const std::vector<ManualBooking>& bookings);
