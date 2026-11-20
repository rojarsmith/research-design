#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct MarksError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// The interview marks one intern got from one team's panel, one per interviewer.
using Marks = std::vector<double>;

// "4.1,4.3" -> {4.1, 4.3}. Each mark is 1.0 to 5.0 with exactly one decimal; 1 to 9 marks.
// Throws MarksError otherwise.
Marks parse_marks(std::string_view text);

double average(const Marks& marks);

// Average with two decimals, for reports: {4.1, 4.3} -> "4.20".
std::string format_average(const Marks& marks);
