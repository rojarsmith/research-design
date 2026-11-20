#include "routes.hpp"

#include <cctype>

namespace {

bool is_name(std::string_view s) {
    if (s.empty() || !(std::islower((unsigned char)s[0]) || s[0] == '_')) return false;
    for (char c : s)
        if (!(std::islower((unsigned char)c) || std::isdigit((unsigned char)c) || c == '_')) return false;
    return true;
}

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

}  // namespace

bool is_route_method(std::string_view m) {
    return m == "GET" || m == "POST" || m == "PUT" || m == "PATCH" || m == "DELETE" || m == "ANY";
}

std::vector<Segment> parse_pattern(std::string_view pattern) {
    if (pattern.empty() || pattern[0] != '/') throw RouteError("pattern must start with '/': '" + std::string(pattern) + "'");
    std::vector<Segment> segs;
    if (pattern == "/") return segs;
    std::size_t start = 1;
    while (start <= pattern.size()) {
        std::size_t slash = pattern.find('/', start);
        if (slash == std::string_view::npos) slash = pattern.size();
        std::string_view piece = pattern.substr(start, slash - start);
        start = slash + 1;
        if (piece.empty()) throw RouteError("empty segment in '" + std::string(pattern) + "'");
        if (!segs.empty() && segs.back().kind == SegKind::Wildcard)
            throw RouteError("wildcard must be the last segment in '" + std::string(pattern) + "'");
        Segment s;
        if (piece[0] == ':' || piece[0] == '*') {
            s.kind = piece[0] == ':' ? SegKind::Param : SegKind::Wildcard;
            s.text = std::string(piece.substr(1));
            if (!is_name(s.text)) throw RouteError("bad parameter name '" + std::string(piece) + "'");
            for (const Segment& other : segs)
                if (other.kind != SegKind::Literal && other.text == s.text)
                    throw RouteError("parameter '" + s.text + "' appears twice in '" + std::string(pattern) + "'");
        } else {
            if (piece == "." || piece == "..") throw RouteError("dot segment in '" + std::string(pattern) + "'");
            s.text = std::string(piece);
        }
        segs.push_back(std::move(s));
    }
    return segs;
}

std::vector<Route> parse_routes(std::string_view text) {
    std::vector<Route> routes;
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
        std::string where = "line " + std::to_string(line_no) + ": ";
        if (w[0] != "route" || w.size() != 4) throw RouteError(where + "expected: route <METHOD> <pattern> <upstream>");
        if (!is_route_method(w[1])) throw RouteError(where + "unknown method '" + std::string(w[1]) + "'");
        Route r;
        r.method = std::string(w[1]);
        r.pattern = std::string(w[2]);
        r.upstream = std::string(w[3]);
        r.line = line_no;
        try {
            r.segments = parse_pattern(w[2]);
        } catch (const RouteError& e) {
            throw RouteError(where + e.what());
        }
        routes.push_back(std::move(r));
    }
    return routes;
}
