#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "routes.hpp"

struct RouteTable {
    std::vector<Route> routes;  // in file order; RouteResult::route indexes into this
    // Add whatever index the router needs.
};

// Builds the lookup structure. Throws RouteError if two routes have the same method and
// the same shape (the same segments, ignoring parameter names). Not implemented yet.
RouteTable build_table(std::vector<Route> routes);

struct RouteResult {
    int status = 404;  // 200, 400, 404 or 405
    int route = -1;    // index into RouteTable::routes when status is 200
    std::vector<std::pair<std::string, std::string>> params;  // when 200: name and value, in pattern order
    std::vector<std::string> allow;                           // when 405: allowed methods, sorted

    bool operator==(const RouteResult&) const = default;
};

// Picks the route for one request. `target` is the raw request target, for example
// "/users//42/?expand=orders". Not implemented yet.
RouteResult route_request(const RouteTable& table, std::string_view method, std::string_view target);
