#include "merge.hpp"

#include <algorithm>

std::vector<Event> merge_logs(const std::vector<LogFile>& files) {
    std::vector<Event> out;
    for (const LogFile& f : files) {
        for (const Event& e : f.events) {
            // Insert after every event with the same or an earlier timestamp, which keeps
            // file order and line order for ties.
            auto it = std::upper_bound(out.begin(), out.end(), e.ts_ms,
                                       [](std::int64_t t, const Event& x) { return t < x.ts_ms; });
            out.insert(it, e);
        }
    }
    return out;
}
