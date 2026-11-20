#include "allocator.hpp"

#include <stdexcept>

Allocation allocate(const std::vector<Room>& rooms, const std::vector<Request>& requests) {
    (void)rooms;
    (void)requests;
    throw std::logic_error("allocate: not implemented");
}
