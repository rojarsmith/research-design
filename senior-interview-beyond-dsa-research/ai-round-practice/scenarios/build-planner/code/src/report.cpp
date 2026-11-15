#include "report.hpp"

#include <algorithm>
#include <sstream>

#include "duration.hpp"

std::string plan_report(const BuildGraph& g) {
    std::vector<int> order = topo_order(g);
    std::size_t width = 0;
    for (const auto& name : g.names) width = std::max(width, name.size());

    std::ostringstream out;
    out << "Build plan: " << g.size() << (g.size() == 1 ? " target\n" : " targets\n");
    int step = 0;
    for (int v : order) {
        out << "  " << ++step << ". " << g.names[v] << std::string(width - g.names[v].size() + 2, ' ')
            << format_duration(g.duration_ms[v]) << "\n";
    }
    out << "Critical path: " << format_duration(critical_path_ms(g)) << "\n";
    return out.str();
}
