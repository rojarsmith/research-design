// Phase 2 tests: allocate(rooms, requests).
#include <stdexcept>
#include <string>
#include <vector>

#include "allocator.hpp"
#include "catalog.hpp"
#include "check.hpp"

namespace {

std::vector<std::string> assigned(const Allocation& a) {
    std::vector<std::string> out;
    for (const Assignment& x : a.assigned) out.push_back(x.request + "->" + x.room);
    return out;
}

std::vector<std::string> waiting(const Allocation& a) {
    std::vector<std::string> out;
    for (const Waitlisted& w : a.waitlist) out.push_back(w.request + ":" + w.reason);
    return out;
}

std::uint64_t allocation_hash(const Allocation& a) {
    std::string text;
    for (const Assignment& x : a.assigned) text += x.request + ">" + x.room + ",";
    text += "|";
    for (const Waitlisted& w : a.waitlist) text += w.request + ":" + w.reason + ",";
    return check::fnv1a(text);
}

}  // namespace

TEST("sample rooms and requests", 2) {
    Allocation a = allocate(parse_rooms(check::read_file("data/rooms.txt")), parse_requests(check::read_file("data/requests.txt")));
    EXPECT_EQ(assigned(a), (std::vector<std::string>{"r1->A101", "r2->A102", "r3->A202", "r4->A005"}));
    EXPECT_EQ(waiting(a), (std::vector<std::string>{"r5:no-room"}));
}

TEST("a room is free again when the previous booking ends", 2) {
    Allocation a = allocate(parse_rooms("room X cap=4 features=-\n"),
                            parse_requests("request a 09:00-10:00 size=2 needs=-\nrequest b 10:00-11:00 size=2 needs=-\n"));
    EXPECT_EQ(assigned(a), (std::vector<std::string>{"a->X", "b->X"}));
    EXPECT_TRUE(a.waitlist.empty());
}

TEST("busy versus no-room", 2) {
    Allocation a = allocate(parse_rooms("room X cap=4 features=tv\n"),
                            parse_requests("request a 09:00-10:00 size=2 needs=tv\n"
                                           "request b 09:30-10:00 size=2 needs=tv\n"
                                           "request c 09:30-10:00 size=2 needs=vc\n"
                                           "request d 09:45-10:00 size=5 needs=-\n"));
    EXPECT_EQ(assigned(a), (std::vector<std::string>{"a->X"}));
    EXPECT_EQ(waiting(a), (std::vector<std::string>{"b:busy", "c:no-room", "d:no-room"}));
}

TEST("ties: smallest capacity, then fewest features, then room id", 2) {
    Allocation a = allocate(parse_rooms("room P cap=6 features=tv,vc\n"
                                        "room R cap=6 features=tv\n"
                                        "room S cap=8 features=tv\n"
                                        "room Q cap=6 features=tv\n"),
                            parse_requests("request m3 09:00-10:00 size=4 needs=tv\n"
                                           "request m1 09:00-10:00 size=4 needs=tv\n"
                                           "request m5 09:00-10:00 size=4 needs=tv\n"
                                           "request m2 09:00-10:00 size=4 needs=tv\n"
                                           "request m4 09:00-10:00 size=4 needs=tv\n"));
    EXPECT_EQ(assigned(a), (std::vector<std::string>{"m1->Q", "m2->R", "m3->P", "m4->S"}));
    EXPECT_EQ(waiting(a), (std::vector<std::string>{"m5:busy"}));
}

TEST("requests are processed by start time, then id", 2) {
    Allocation a = allocate(parse_rooms("room X cap=4 features=-\n"),
                            parse_requests("request z 08:00-08:30 size=1 needs=-\n"
                                           "request a 09:00-10:00 size=1 needs=-\n"
                                           "request b 08:00-09:30 size=1 needs=-\n"));
    EXPECT_EQ(assigned(a), (std::vector<std::string>{"b->X"}));
    EXPECT_EQ(waiting(a), (std::vector<std::string>{"z:busy", "a:busy"}));
}

TEST("invalid requests and empty input", 2) {
    auto rooms = parse_rooms("room X cap=4 features=-\n");
    bool bad_size = false, bad_span = false;
    try { allocate(rooms, {Request{"q", 600, 660, 0, 0}}); } catch (const std::invalid_argument&) { bad_size = true; }
    try { allocate(rooms, {Request{"q", 660, 660, 1, 0}}); } catch (const std::invalid_argument&) { bad_span = true; }
    EXPECT_TRUE(bad_size);
    EXPECT_TRUE(bad_span);
    Allocation none = allocate({}, parse_requests("request q 09:00-10:00 size=1 needs=-\n"));
    EXPECT_EQ(waiting(none), (std::vector<std::string>{"q:no-room"}));
    EXPECT_TRUE(allocate(rooms, {}).assigned.empty());
}

TEST("medium random days", 2) {
    struct Case { int rooms; int requests; std::uint64_t seed; std::size_t assigned; std::uint64_t hash; };
    const Case cases[] = {{40, 1000, 31, 460, 3754319743336103757ULL}, {200, 8000, 32, 2388, 2576557303022102586ULL}};
    for (const Case& c : cases) {
        check::Rng rng(c.seed);
        std::vector<Room> rooms;
        for (int i = 0; i < c.rooms; ++i)
            rooms.push_back({"R" + std::to_string(rng.range(100, 999)) + "-" + std::to_string(i), (int)rng.range(2, 30),
                             (unsigned)rng.range(0, 15)});
        std::vector<Request> reqs;
        for (int i = 0; i < c.requests; ++i) {
            int start = (int)rng.range(7 * 60, 19 * 60) / 5 * 5;
            int len = (int)rng.range(1, 24) * 5;
            reqs.push_back({"q" + std::to_string(rng.range(0, 99999)) + "-" + std::to_string(i), start, std::min(1440, start + len),
                            (int)rng.range(1, 24), (unsigned)(rng.range(0, 3) == 0 ? rng.range(0, 15) : 0)});
        }
        Allocation a = allocate(rooms, reqs);
        EXPECT_EQ(a.assigned.size(), c.assigned);
        EXPECT_EQ(allocation_hash(a), c.hash);
    }
}
