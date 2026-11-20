#include "garage.hpp"

#include <stdexcept>

Garage::Garage(const std::vector<Level>&) {}

std::string Garage::enter(const std::string&, const std::string&, long long) {
    throw std::logic_error("not implemented: Garage::enter");
}

long long Garage::exit(const std::string&, long long) {
    throw std::logic_error("not implemented: Garage::exit");
}

std::string Garage::spotOf(const std::string&) const {
    throw std::logic_error("not implemented: Garage::spotOf");
}

int Garage::freeSpots(const std::string&) const {
    throw std::logic_error("not implemented: Garage::freeSpots");
}
