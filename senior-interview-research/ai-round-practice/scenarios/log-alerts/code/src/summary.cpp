#include "summary.hpp"

#include <algorithm>
#include <map>
#include <sstream>

#include "timeparse.hpp"

std::string summary_report(const std::vector<Event>& timeline) {
    std::ostringstream out;
    out << "Events: " << timeline.size() << "\n";
    if (timeline.empty()) return out.str();

    out << "From " << format_timestamp(timeline.front().ts_ms) << " to " << format_timestamp(timeline.back().ts_ms) << "\n";

    int by_level[5] = {0, 0, 0, 0, 0};
    std::map<std::string, int> error_codes;
    for (const Event& e : timeline) {
        ++by_level[(int)e.level];
        if (e.level >= Level::Error) ++error_codes[e.code];
    }
    out << "By level:";
    for (int i = 0; i < 5; ++i) out << (i ? ", " : " ") << level_name((Level)i) << " " << by_level[i];
    out << "\n";

    std::vector<std::pair<std::string, int>> top(error_codes.begin(), error_codes.end());
    std::stable_sort(top.begin(), top.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    out << "Top error codes:";
    for (std::size_t i = 0; i < top.size() && i < 3; ++i)
        out << (i ? ", " : " ") << top[i].first << " x" << top[i].second;
    if (top.empty()) out << " none";
    out << "\n";
    return out.str();
}
