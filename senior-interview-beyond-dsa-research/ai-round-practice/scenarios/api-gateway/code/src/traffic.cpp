#include "traffic.hpp"

#include <algorithm>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

#include "pathnorm.hpp"

std::string traffic_report(const AccessLog& log) {
    int by_class[6] = {0, 0, 0, 0, 0, 0};
    std::int64_t bytes = 0;
    int invalid = 0;
    std::map<std::string, int> hits;
    for (const Request& r : log.requests) {
        ++by_class[r.status / 100];
        bytes += r.bytes;
        try {
            ++hits[format_path(normalize_path(r.target))];
        } catch (const PathError&) {
            ++invalid;
        }
    }

    std::ostringstream out;
    out << "Requests: " << log.requests.size();
    if (invalid) out << " (" << invalid << " with an invalid target)";
    out << "\n";
    out << "Status:";
    bool first = true;
    for (int c = 1; c <= 5; ++c) {
        if (!by_class[c]) continue;
        out << (first ? " " : ", ") << c << "xx " << by_class[c];
        first = false;
    }
    out << "\n";
    out << "Bytes: " << bytes << "\n";

    // std::map iterates in path order, so a stable sort by count keeps ties in path order.
    std::vector<std::pair<std::string, int>> top(hits.begin(), hits.end());
    std::stable_sort(top.begin(), top.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    if (top.size() > 5) top.resize(5);
    std::size_t width = 0;
    for (const auto& [path, n] : top) width = std::max(width, path.size());
    out << "Top paths:" << (top.empty() ? " none" : "") << "\n";
    for (const auto& [path, n] : top) out << "  " << path << std::string(width - path.size() + 2, ' ') << n << "\n";
    return out.str();
}
