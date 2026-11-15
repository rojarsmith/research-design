// Phase 3 tests: a whole company. Each test writes its rooms and requests to
// data/generated/ (open them: their shapes differ), parses them, and then times allocate.
// Parsing is not timed.
#include <string>
#include <vector>

#include "allocator.hpp"
#include "catalog.hpp"
#include "check.hpp"
#include "clock.hpp"

namespace {

std::uint64_t allocation_hash(const Allocation& a) {
    std::string text;
    for (const Assignment& x : a.assigned) text += x.request + ">" + x.room + ",";
    text += "|";
    for (const Waitlisted& w : a.waitlist) text += w.request + ":" + w.reason + ",";
    return check::fnv1a(text);
}

struct Day {
    std::string rooms, requests;
};

std::string room_line(int i, int cap, unsigned features) {
    return "room B" + std::to_string(i / 100) + "-" + std::to_string(i) + " cap=" + std::to_string(cap) +
           " features=" + format_features(features) + "\n";
}

std::string request_line(int i, int start, int len, int size, unsigned needs) {
    int end = std::min(1440, start + len);
    return "request q" + std::to_string(i) + " " + format_clock(start) + "-" + format_clock(end) +
           " size=" + std::to_string(size) + " needs=" + format_features(needs) + "\n";
}

// 12,000 rooms and 200,000 short requests spread over the working day: most requests get a room.
Day campus() {
    check::Rng rng(501);
    Day d;
    for (int i = 0; i < 12000; ++i) d.rooms += room_line(i, (int)rng.range(2, 40), (unsigned)rng.range(0, 15));
    for (int i = 0; i < 200000; ++i)
        d.requests += request_line(i, (int)rng.range(7 * 12, 19 * 12) * 5, (int)rng.range(3, 12) * 5, (int)rng.range(1, 12),
                                   rng.range(0, 9) == 0 ? (unsigned)rng.range(1, 15) : 0u);
    return d;
}

// 12,000 rooms, but video and accessible rooms are rare, and rooms with both are small.
// Many requests need both, for more people than any such room holds.
Day scarce() {
    check::Rng rng(502);
    Day d;
    for (int i = 0; i < 12000; ++i) {
        unsigned f = (unsigned)rng.range(0, 15) & (Tv | Whiteboard);
        if (rng.range(0, 49) == 0) f |= Vc;
        if (rng.range(0, 49) == 0) f |= Accessible;
        bool both = (f & (Vc | Accessible)) == (Vc | Accessible);
        d.rooms += room_line(i, both ? (int)rng.range(2, 8) : (int)rng.range(2, 30), f);
    }
    for (int i = 0; i < 150000; ++i) {
        int kind = (int)rng.range(0, 19);
        unsigned needs = kind < 7 ? (Vc | Accessible) : kind < 11 ? Vc : 0u;
        int size = kind < 7 ? (int)rng.range(9, 24) : (int)rng.range(1, 20);
        d.requests += request_line(i, (int)rng.range(8 * 12, 18 * 12) * 5, (int)rng.range(6, 24) * 5, size, needs);
    }
    return d;
}

// 5,000 rooms and 200,000 requests that all start between 08:00 and 10:00.
Day rush() {
    check::Rng rng(503);
    Day d;
    for (int i = 0; i < 5000; ++i) d.rooms += room_line(i, (int)rng.range(2, 16), (unsigned)rng.range(0, 15));
    for (int i = 0; i < 200000; ++i)
        d.requests += request_line(i, (int)rng.range(8 * 60, 10 * 60), (int)rng.range(15, 240), (int)rng.range(1, 10),
                                   rng.range(0, 4) == 0 ? (unsigned)rng.range(1, 15) : 0u);
    return d;
}

void run_large(const std::string& name, const Day& day, double limit_ms, std::size_t assigned, std::uint64_t hash) {
    auto rooms = parse_rooms(check::read_file(check::write_generated(name + "-rooms.txt", day.rooms)));
    auto requests = parse_requests(check::read_file(check::write_generated(name + "-requests.txt", day.requests)));
    Allocation a;
    check::measure(limit_ms, [&] { a = allocate(rooms, requests); });
    EXPECT_EQ(a.assigned.size(), assigned);
    EXPECT_EQ(allocation_hash(a), hash);
}

}  // namespace

TEST("large-1 campus: many rooms, requests all day", 3) { run_large("large-1-campus", campus(), 1000, 200000, 18098142932626458475ULL); }

TEST("large-2 scarce video rooms, long waitlist", 3) { run_large("large-2-scarce", scarce(), 1000, 69331, 10851085560880474683ULL); }

TEST("large-3 morning rush", 3) { run_large("large-3-rush", rush(), 1000, 7772, 9138714514755088367ULL); }
