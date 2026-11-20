#include "layout.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <unordered_map>

namespace {

// Via names are short ("V12", "TV3"), so the first few characters are plenty to hash.
struct ViaHash {
    std::size_t operator()(const std::string& name) const {
        return std::hash<std::string_view>{}(std::string_view(name).substr(0, 4));
    }
};

bool is_via_name(std::string_view s) {
    if (s.empty() || s.size() > 32) return false;
    for (char c : s)
        if (!(std::isalnum((unsigned char)c) || c == '_' || c == '-')) return false;
    return true;
}

}  // namespace

Layout parse_layout(std::string_view text) {
    Layout l;
    int line_no = 0;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) nl = text.size();
        std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        ++line_no;
        if (auto h = line.find('#'); h != std::string_view::npos) line = line.substr(0, h);
        std::vector<std::string_view> w;
        std::size_t i = 0;
        while (i < line.size()) {
            while (i < line.size() && std::isspace((unsigned char)line[i])) ++i;
            std::size_t start = i;
            while (i < line.size() && !std::isspace((unsigned char)line[i])) ++i;
            if (i > start) w.push_back(line.substr(start, i - start));
        }
        if (w.empty()) continue;
        if (w[0] != "trace" || w.size() != 3)
            throw LayoutError("line " + std::to_string(line_no) + ": expected: trace <node> <node>");
        l.traces.push_back({std::string(w[1]), std::string(w[2]), line_no});
    }
    return l;
}

Board load_board(const Netlist& n, const Layout& layout) {
    Board b;
    for (const Part& p : n.parts) {
        b.first_pin.push_back(b.pin_nodes);
        b.pin_nodes += p.pins;
    }
    std::unordered_map<std::string, int, ViaHash> via_index;
    auto fail = [](int line, const std::string& msg) { throw LayoutError("line " + std::to_string(line) + ": " + msg); };
    auto node_of = [&](const std::string& name, int line) {
        if (name.find('.') != std::string::npos) {
            PinRef p;
            try {
                p = parse_pin(name);
            } catch (const PinError& e) {
                fail(line, e.what());
            }
            int part = n.find_part(p.part);
            if (part < 0) fail(line, "unknown part '" + p.part + "'");
            if (p.pin > n.parts[part].pins) fail(line, p.part + " has no pin " + std::to_string(p.pin));
            return b.pin_node(part, p.pin);
        }
        if (!is_via_name(name)) fail(line, "bad via name '" + name + "'");
        auto [it, fresh] = via_index.emplace(name, b.pin_nodes + (int)b.vias.size());
        if (fresh) b.vias.push_back(name);
        return it->second;
    };
    b.edges.reserve(layout.traces.size());
    for (const Trace& t : layout.traces) {
        int a = node_of(t.a, t.line), c = node_of(t.b, t.line);
        if (a == c) throw LayoutError("line " + std::to_string(t.line) + ": trace from " + t.a + " to itself");
        b.edges.push_back({a, c});
    }
    return b;
}

std::string node_name(const Netlist& n, const Board& b, int node) {
    if (node >= b.pin_nodes) return b.vias[node - b.pin_nodes];
    // The part whose pins contain this node: the last first_pin that is <= node.
    int part = (int)(std::upper_bound(b.first_pin.begin(), b.first_pin.end(), node) - b.first_pin.begin()) - 1;
    return format_pin({n.parts[part].designator, node - b.first_pin[part] + 1});
}
