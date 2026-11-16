// seat_holds.hpp: the public interface the tests use. Every method may be called from many threads at once.
// Keep these signatures. Everything behind them is your design: add members here, and add your own
// classes in new files under src/.
#pragma once

#include <string>
#include <vector>

class SeatHolds {
public:
    // seats: the seat ids. holdSeconds: how long a hold lasts.
    SeatHolds(const std::vector<std::string>& seats, long long holdSeconds);

    // All the seats or none: "H1", "H2", ..., or "bad-request", "unknown-seat", "unavailable".
    std::string hold(const std::string& customer, const std::vector<std::string>& seats, long long now);
    // "ok", or "unknown-hold", "not-yours", "done", "expired".
    std::string confirm(const std::string& holdId, const std::string& customer, long long now);
    std::string release(const std::string& holdId, const std::string& customer, long long now);

    // "free", "held", "sold", or "unknown-seat".
    std::string seatState(const std::string& seat, long long now);
    int freeCount(long long now);
};
