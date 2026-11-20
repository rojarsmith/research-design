#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct Request {
    std::string method;
    std::string target;  // raw, as the client sent it
    int status = 0;
    std::int64_t bytes = 0;
    int ms = 0;
};

struct AccessLog {
    std::vector<Request> requests;  // in line order
    int malformed = 0;              // non-empty lines that did not parse
};

// One "<METHOD> <target> <status> <bytes> <ms>" per line. Lines that do not parse are
// counted in `malformed` and skipped.
AccessLog parse_access_log(std::string_view text);
