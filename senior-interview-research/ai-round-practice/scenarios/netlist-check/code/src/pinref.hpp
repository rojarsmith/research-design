#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

struct PinError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// One pin of one part: "U1.12" is pin 12 of part U1.
struct PinRef {
    std::string part;  // designator: capital letters then digits, such as R1, U12, LED3, TP2
    int pin = 0;       // 1-based

    bool operator==(const PinRef&) const = default;
};

bool is_designator(std::string_view text);

// "U1.12" -> {"U1", 12}. Throws PinError for anything else, including pin 0.
PinRef parse_pin(std::string_view text);

// {"U1", 12} -> "U1.12"
std::string format_pin(const PinRef& p);

// Natural order, as engineers read designators: runs of digits compare as numbers, everything
// else byte by byte. "R2" < "R10", "U1.2" < "U1.10", "C9" < "R1".
bool natural_less(std::string_view a, std::string_view b);

// Compact key for hash maps and sets of pins.
std::string pin_key(const PinRef& p);
