// auction_house.hpp: the public interface the tests use.
// Keep these signatures. Everything behind them is your design: add members here, and add your own
// classes in new files under src/.
#pragma once

#include <string>
#include <vector>

class AuctionHouse {
public:
    void addUser(const std::string& id);
    // "A1", "A2", ..., or "" if the seller is unknown or a price is less than 1.
    std::string open(const std::string& sellerId, const std::string& item, long long startPrice, long long increment,
                     long long endsAt);

    // "ok" or an error code (see TASK.md).
    std::string bid(const std::string& auctionId, const std::string& userId, long long amount, long long minute);
    std::string watch(const std::string& auctionId, const std::string& userId);
    // Closes every open auction whose end time is at most `minute`.
    void tick(long long minute);

    std::string leader(const std::string& auctionId) const;  // "" if none or unknown
    long long price(const std::string& auctionId) const;      // 0 if no bids, -1 if unknown
    std::string status(const std::string& auctionId) const;   // "open", "sold", "unsold", or ""
    std::vector<std::string> inbox(const std::string& userId) const;
};
