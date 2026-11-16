#include "router.hpp"

#include <stdexcept>

RouteTable build_table(std::vector<Route> routes) {
    (void)routes;
    throw std::logic_error("build_table: not implemented");
}

RouteResult route_request(const RouteTable& table, std::string_view method, std::string_view target) {
    (void)table;
    (void)method;
    (void)target;
    throw std::logic_error("route_request: not implemented");
}
