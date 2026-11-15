// Existing tests for roombook. Do not edit them to make them pass.
#include <string>
#include <vector>

#include "calendar.hpp"
#include "catalog.hpp"
#include "check.hpp"
#include "clock.hpp"

namespace {

template <class E, class F>
std::string error_of(F&& f) {
    try {
        f();
    } catch (const E& e) {
        return e.what();
    }
    return "(no error)";
}

}  // namespace

TEST("parse_clock and format_clock", 1) {
    EXPECT_EQ(parse_clock("00:00"), 0);
    EXPECT_EQ(parse_clock("9:30"), 570);
    EXPECT_EQ(parse_clock("24:00"), 1440);
    EXPECT_EQ(format_clock(615), std::string("10:15"));
    EXPECT_EQ(error_of<ClockError>([] { parse_clock("24:01"); }), std::string("bad time '24:01'"));
    EXPECT_EQ(error_of<ClockError>([] { parse_clock("9h30"); }), std::string("bad time '9h30'"));
    EXPECT_EQ(error_of<ClockError>([] { parse_span("10:00-10:00"); }), std::string("span ends before it starts: '10:00-10:00'"));
}

TEST("features parse and format", 1) {
    EXPECT_EQ(parse_features("-"), 0u);
    EXPECT_EQ(parse_features("vc,tv"), (unsigned)(Vc | Tv));
    EXPECT_EQ(format_features(Accessible | Tv), std::string("tv,accessible"));
    EXPECT_EQ(error_of<CatalogError>([] { parse_features("tv,laser"); }), std::string("unknown feature 'laser'"));
}

TEST("parse_rooms and parse_requests report the failing line", 1) {
    auto rooms = parse_rooms(check::read_file("data/rooms.txt"));
    EXPECT_EQ(rooms.size(), std::size_t(6));
    EXPECT_EQ(rooms[4].capacity, 20);
    EXPECT_EQ(rooms[4].features, (unsigned)(Tv | Vc | Whiteboard | Accessible));
    auto reqs = parse_requests(check::read_file("data/requests.txt"));
    EXPECT_EQ(reqs.size(), std::size_t(5));
    EXPECT_EQ(reqs[2].start, 570);
    EXPECT_EQ(reqs[2].needs, (unsigned)Vc);
    EXPECT_EQ(error_of<CatalogError>([] { parse_rooms("room A cap=4 features=tv\nroom B cap=0 features=-\n"); }),
              std::string("line 2: number must be at least 1"));
}

TEST("calendar keeps bookings sorted and detects clear overlaps", 1) {
    Calendar cal(2);
    cal.book(0, {600, 660, "b"});
    cal.book(0, {540, 570, "a"});
    cal.book(0, {720, 780, "c"});
    std::vector<std::string> labels;
    for (const Booking& b : cal.bookings(0)) labels.push_back(b.label);
    EXPECT_EQ(labels, (std::vector<std::string>{"a", "b", "c"}));
    EXPECT_TRUE(!cal.is_free(0, 620, 640));   // inside b
    EXPECT_TRUE(!cal.is_free(0, 500, 800));   // covers everything
    EXPECT_TRUE(cal.is_free(0, 575, 595));    // gap between a and b
    EXPECT_TRUE(cal.is_free(1, 0, 1440));     // empty room
}

TEST("conflict report for the front-desk bookings", 1) {
    auto rooms = parse_rooms(check::read_file("data/rooms.txt"));
    auto conflicts = find_conflicts(rooms, parse_bookings(check::read_file("data/bookings.txt")));
    std::vector<std::string> got;
    for (const Conflict& c : conflicts) got.push_back(c.room + ": " + c.first + " / " + c.second);
    EXPECT_EQ(got, (std::vector<std::string>{"A101: design-review / interview", "A301: workshop / all-hands"}));
}

TEST("conflict report rejects unknown rooms", 1) {
    auto rooms = parse_rooms("room A cap=4 features=-\n");
    EXPECT_EQ(error_of<CatalogError>([&] { find_conflicts(rooms, parse_bookings("booking Z 09:00-10:00 x\n")); }),
              std::string("booking 'x' names unknown room 'Z'"));
}
