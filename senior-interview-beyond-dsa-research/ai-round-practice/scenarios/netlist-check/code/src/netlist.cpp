#include "netlist.hpp"

#include <cctype>

namespace {

std::vector<std::string_view> words_of(std::string_view line) {
    std::vector<std::string_view> out;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && std::isspace((unsigned char)line[i])) ++i;
        std::size_t start = i;
        while (i < line.size() && !std::isspace((unsigned char)line[i])) ++i;
        if (i > start) out.push_back(line.substr(start, i - start));
    }
    return out;
}

bool is_net_name(std::string_view s) {
    if (s.empty()) return false;
    for (char c : s)
        if (!(std::isalnum((unsigned char)c) || c == '_' || c == '-' || c == '+')) return false;
    return true;
}

int pin_count(std::string_view word) {
    if (word.substr(0, 5) != "pins=" || word.size() < 6 || word.size() > 9 || word[5] == '0')
        throw NetlistError("expected pins=<n>, got '" + std::string(word) + "'");
    int n = 0;
    for (char c : word.substr(5)) {
        if (!std::isdigit((unsigned char)c)) throw NetlistError("expected pins=<n>, got '" + std::string(word) + "'");
        n = n * 10 + (c - '0');
    }
    return n;
}

}  // namespace

int Netlist::find_part(std::string_view designator) const {
    auto it = part_index.find(std::string(designator));
    return it == part_index.end() ? -1 : it->second;
}

Netlist parse_netlist(std::string_view text) {
    Netlist n;
    std::unordered_map<std::string, int> net_index;
    std::unordered_map<std::string, int> owner;  // pin key -> net that lists it
    int line_no = 0;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) nl = text.size();
        std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        ++line_no;
        if (auto h = line.find('#'); h != std::string_view::npos) line = line.substr(0, h);
        auto w = words_of(line);
        if (w.empty()) continue;
        try {
            if (w[0] == "part") {
                if (w.size() < 3 || w.size() > 4) throw NetlistError("expected: part <designator> pins=<n> [value]");
                if (!is_designator(w[1])) throw NetlistError("bad designator '" + std::string(w[1]) + "'");
                Part p{std::string(w[1]), pin_count(w[2]), w.size() == 4 ? std::string(w[3]) : std::string()};
                if (!n.part_index.emplace(p.designator, (int)n.parts.size()).second)
                    throw NetlistError("part " + p.designator + " is declared twice");
                n.parts.push_back(std::move(p));
            } else if (w[0] == "net") {
                if (w.size() < 3) throw NetlistError("expected: net <name> <pin> ...");
                if (!is_net_name(w[1])) throw NetlistError("bad net name '" + std::string(w[1]) + "'");
                Net net{std::string(w[1]), {}};
                int id = (int)n.nets.size();
                if (!net_index.emplace(net.name, id).second) throw NetlistError("net " + net.name + " is declared twice");
                for (std::size_t k = 2; k < w.size(); ++k) {
                    PinRef p = parse_pin(w[k]);
                    int part = n.find_part(p.part);
                    if (part < 0) throw NetlistError("unknown part '" + p.part + "'");
                    if (p.pin > n.parts[part].pins)
                        throw NetlistError(p.part + " has " + std::to_string(n.parts[part].pins) + " pins, not " +
                                           std::to_string(p.pin));
                    auto [it, fresh] = owner.emplace(pin_key(p), id);
                    if (!fresh) {
                        const std::string& other = it->second == id ? net.name : n.nets[it->second].name;
                        throw NetlistError("pin " + format_pin(p) + " is already on net " + other);
                    }
                    net.pins.push_back(std::move(p));
                }
                n.nets.push_back(std::move(net));
            } else {
                throw NetlistError("expected 'part' or 'net', got '" + std::string(w[0]) + "'");
            }
        } catch (const std::exception& e) {
            throw NetlistError("line " + std::to_string(line_no) + ": " + e.what());
        }
    }
    return n;
}
