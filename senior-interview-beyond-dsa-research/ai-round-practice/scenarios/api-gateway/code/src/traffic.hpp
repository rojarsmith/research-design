#pragma once

#include <string>

#include "accesslog.hpp"

// Traffic summary for the on-call channel:
//
//   Requests: 22 (1 with an invalid target)
//   Status: 2xx 15, 3xx 1, 4xx 4, 5xx 2
//   Bytes: 1048576
//   Top paths:
//     /users/42  5
//     /health    4
//
// Paths are normalized first (see README), so "//users/42/" counts as "/users/42".
// "Top paths" lists at most five, most requests first, ties by path (byte order).
// Requests whose target does not normalize are counted but never listed.
std::string traffic_report(const AccessLog& log);
