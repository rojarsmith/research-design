#include "calendar.hpp"

#include <algorithm>
#include <map>
#include <tuple>
#include <unordered_map>

bool overlaps(const Booking& a, const Booking& b) { return a.start <= b.end && b.start <= a.end; }

bool Calendar::is_free(int room, Minutes start, Minutes end) const {
    Booking probe{start, end, ""};
    for (const Booking& b : by_room_[room])
        if (overlaps(b, probe)) return false;
    return true;
}

void Calendar::book(int room, Booking b) {
    auto& list = by_room_[room];
    auto it = std::upper_bound(list.begin(), list.end(), b.start, [](Minutes s, const Booking& x) { return s < x.start; });
    list.insert(it, std::move(b));
}

std::vector<Conflict> find_conflicts(const std::vector<Room>& rooms, const std::vector<ManualBooking>& bookings) {
    std::unordered_map<std::string, int> index;
    for (int i = 0; i < (int)rooms.size(); ++i) index[rooms[i].id] = i;

    Calendar cal((int)rooms.size());
    std::vector<std::tuple<std::string, Minutes, Minutes, Conflict>> found;
    for (const ManualBooking& m : bookings) {
        auto it = index.find(m.room);
        if (it == index.end()) throw CatalogError("booking '" + m.label + "' names unknown room '" + m.room + "'");
        Booking b{m.start, m.end, m.label};
        for (const Booking& other : cal.bookings(it->second)) {
            if (!overlaps(other, b)) continue;
            const Booking& first = std::tie(other.start, other.label) <= std::tie(b.start, b.label) ? other : b;
            const Booking& second = &first == &other ? b : other;
            found.push_back({m.room, first.start, second.start, Conflict{m.room, first.label, second.label}});
        }
        cal.book(it->second, b);
    }
    std::sort(found.begin(), found.end(), [](const auto& x, const auto& y) {
        return std::tie(std::get<0>(x), std::get<1>(x), std::get<2>(x)) < std::tie(std::get<0>(y), std::get<1>(y), std::get<2>(y));
    });
    std::vector<Conflict> out;
    for (auto& f : found) out.push_back(std::get<3>(f));
    return out;
}
