#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct RouteError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum class SegKind { Literal, Param, Wildcard };

struct Segment {
    SegKind kind = SegKind::Literal;
    std::string text;  // the literal, or the parameter name without the ':' or '*'
};

struct Route {
    std::string method;             // GET, POST, PUT, PATCH, DELETE or ANY
    std::string pattern;            // as written, for messages
    std::vector<Segment> segments;  // empty for the pattern "/"
    std::string upstream;
    int line = 0;                   // 1-based line in the route file
};

bool is_route_method(std::string_view method);

// "/users/:id/*rest" -> segments. Throws RouteError (the message has no line number).
std::vector<Segment> parse_pattern(std::string_view pattern);

// One "route <METHOD> <pattern> <upstream>" per line; '#' starts a comment.
// Throws RouteError("line N: ...").
std::vector<Route> parse_routes(std::string_view text);
