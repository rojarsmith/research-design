#include "pinreport.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace {

std::unordered_set<std::string> pins_on_nets(const Netlist& n) {
    std::unordered_set<std::string> used;
    for (const Net& net : n.nets)
        for (const PinRef& p : net.pins) used.insert(pin_key(p));
    return used;
}

std::vector<int> free_pins(const Part& part, const std::unordered_set<std::string>& used) {
    std::vector<int> out;
    for (int pin = 1; pin <= part.pins; ++pin)
        if (!used.count(pin_key({part.designator, pin}))) out.push_back(pin);
    return out;
}

// {2, 3, 4, 7, 9, 10} -> "2-4, 7, 9-10"
std::string ranges(const std::vector<int>& v) {
    std::string out;
    for (std::size_t i = 0; i < v.size();) {
        std::size_t j = i;
        while (j + 1 < v.size() && v[j + 1] == v[j] + 1) ++j;
        if (!out.empty()) out += ", ";
        out += std::to_string(v[i]);
        if (j > i) out += "-" + std::to_string(v[j]);
        i = j + 1;
    }
    return out;
}

}  // namespace

std::vector<int> unconnected_pins(const Netlist& n, std::string_view designator) {
    int part = n.find_part(designator);
    if (part < 0) throw NetlistError("unknown part '" + std::string(designator) + "'");
    return free_pins(n.parts[part], pins_on_nets(n));
}

std::string pin_report(const Netlist& n) {
    auto used = pins_on_nets(n);
    int total = 0;
    std::size_t width = 0;
    for (const Part& p : n.parts) {
        total += p.pins;
        width = std::max(width, p.designator.size());
    }
    std::vector<const Part*> order;
    for (const Part& p : n.parts) order.push_back(&p);
    std::sort(order.begin(), order.end(), [](const Part* a, const Part* b) { return natural_less(a->designator, b->designator); });

    std::ostringstream out;
    out << "Parts: " << n.parts.size() << ", nets: " << n.nets.size() << ", pins on nets: " << used.size() << " of "
        << total << "\n";
    for (const Part* p : order) {
        auto unused = free_pins(*p, used);
        out << "  " << std::left << std::setw((int)width) << p->designator << std::right << std::setw(4) << p->pins
            << (p->pins == 1 ? " pin, " : " pins, ");
        if (unused.empty()) out << "all on nets\n";
        else out << unused.size() << " not on any net: " << ranges(unused) << "\n";
    }
    std::vector<std::string> single;
    for (const Net& net : n.nets)
        if (net.pins.size() == 1) single.push_back(net.name);
    std::sort(single.begin(), single.end(), [](const std::string& a, const std::string& b) { return natural_less(a, b); });
    out << "Single-pin nets:";
    for (std::size_t i = 0; i < single.size(); ++i) out << (i ? ", " : " ") << single[i];
    if (single.empty()) out << " none";
    out << "\n";
    return out.str();
}
