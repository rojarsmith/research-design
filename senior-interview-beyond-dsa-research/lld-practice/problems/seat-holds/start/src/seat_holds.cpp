#include "seat_holds.hpp"

#include <stdexcept>

SeatHolds::SeatHolds(const std::vector<std::string>&, long long) {}

std::string SeatHolds::hold(const std::string&, const std::vector<std::string>&, long long) {
    throw std::logic_error("not implemented: SeatHolds::hold");
}

std::string SeatHolds::confirm(const std::string&, const std::string&, long long) {
    throw std::logic_error("not implemented: SeatHolds::confirm");
}

std::string SeatHolds::release(const std::string&, const std::string&, long long) {
    throw std::logic_error("not implemented: SeatHolds::release");
}

std::string SeatHolds::seatState(const std::string&, long long) {
    throw std::logic_error("not implemented: SeatHolds::seatState");
}

int SeatHolds::freeCount(long long) { throw std::logic_error("not implemented: SeatHolds::freeCount"); }
