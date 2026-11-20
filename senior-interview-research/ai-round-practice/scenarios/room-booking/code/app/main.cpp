// roombook check <rooms> <bookings>
// roombook assign <rooms> <requests>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "allocator.hpp"
#include "calendar.hpp"
#include "catalog.hpp"

static std::string slurp(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error(std::string("cannot open ") + path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: roombook check <rooms> <bookings>\n       roombook assign <rooms> <requests>\n";
        return 2;
    }
    try {
        std::string mode = argv[1];
        auto rooms = parse_rooms(slurp(argv[2]));
        if (mode == "check") {
            auto conflicts = find_conflicts(rooms, parse_bookings(slurp(argv[3])));
            for (const Conflict& c : conflicts) std::cout << c.room << ": " << c.first << " overlaps " << c.second << "\n";
            std::cout << conflicts.size() << " conflicts\n";
        } else if (mode == "assign") {
            Allocation a = allocate(rooms, parse_requests(slurp(argv[3])));
            for (const Assignment& x : a.assigned) std::cout << x.request << " -> " << x.room << "\n";
            for (const Waitlisted& w : a.waitlist) std::cout << w.request << " waitlisted (" << w.reason << ")\n";
        } else {
            std::cerr << "unknown mode '" << mode << "'\n";
            return 2;
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
