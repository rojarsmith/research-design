// garage.hpp: the public interface the tests use.
// Keep these signatures. Everything behind them is your design: add members here, and add your own
// classes in new files under src/.
#pragma once

#include <string>
#include <vector>

// How many spots of each size one level has.
struct Level {
    int small = 0;
    int regular = 0;
    int large = 0;
};

class Garage {
public:
    // levels[0] is level 1.
    explicit Garage(const std::vector<Level>& levels);

    // A ticket id "T1", "T2", ..., or "bad-vehicle", "already-inside", "full".
    std::string enter(const std::string& plate, const std::string& vehicle, long long minute);
    // The fee in cents, or -1 for an unknown or used ticket.
    long long exit(const std::string& ticket, long long minute);

    // The spot id while the vehicle is inside, else "".
    std::string spotOf(const std::string& ticket) const;
    // Free spots of "small", "regular" or "large" on all levels; -1 for an unknown size.
    int freeSpots(const std::string& size) const;
};
