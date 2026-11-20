#pragma once

#include <string>
#include <vector>

#include "catalog.hpp"

struct Assignment {
    std::string request;
    std::string room;
    bool operator==(const Assignment&) const = default;
};

struct Waitlisted {
    std::string request;
    std::string reason;  // "no-room" or "busy"
    bool operator==(const Waitlisted&) const = default;
};

struct Allocation {
    std::vector<Assignment> assigned;  // in the order requests were processed
    std::vector<Waitlisted> waitlist;  // in the order requests were processed
};

// Assigns rooms to booking requests. Not implemented yet.
Allocation allocate(const std::vector<Room>& rooms, const std::vector<Request>& requests);
