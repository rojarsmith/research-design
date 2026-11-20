#include "auction_house.hpp"

#include <stdexcept>

void AuctionHouse::addUser(const std::string&) { throw std::logic_error("not implemented: AuctionHouse::addUser"); }

std::string AuctionHouse::open(const std::string&, const std::string&, long long, long long, long long) {
    throw std::logic_error("not implemented: AuctionHouse::open");
}

std::string AuctionHouse::bid(const std::string&, const std::string&, long long, long long) {
    throw std::logic_error("not implemented: AuctionHouse::bid");
}

std::string AuctionHouse::watch(const std::string&, const std::string&) {
    throw std::logic_error("not implemented: AuctionHouse::watch");
}

void AuctionHouse::tick(long long) { throw std::logic_error("not implemented: AuctionHouse::tick"); }

std::string AuctionHouse::leader(const std::string&) const { throw std::logic_error("not implemented: AuctionHouse::leader"); }

long long AuctionHouse::price(const std::string&) const { throw std::logic_error("not implemented: AuctionHouse::price"); }

std::string AuctionHouse::status(const std::string&) const { throw std::logic_error("not implemented: AuctionHouse::status"); }

std::vector<std::string> AuctionHouse::inbox(const std::string&) const {
    throw std::logic_error("not implemented: AuctionHouse::inbox");
}
