#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct PathError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Splits a request target into path segments (see README, "Paths"):
//   "/users//42/./orders/?page=2"  -> {"users", "42", "orders"}
//   "/a/b/../c"                    -> {"a", "c"}
//   "/files/q3%2Freport.pdf"       -> {"files", "q3/report.pdf"}
// Throws PathError for a bad percent-escape.
std::vector<std::string> normalize_path(std::string_view target);

// Segments back to a path, escaping the characters that cannot appear inside a segment:
//   {"files", "q3/report.pdf"} -> "/files/q3%2Freport.pdf"
//   {}                         -> "/"
std::string format_path(const std::vector<std::string>& segments);

// "%41b%2F" -> "Ab/". Throws PathError if a '%' is not followed by two hex digits.
std::string percent_decode(std::string_view text);
