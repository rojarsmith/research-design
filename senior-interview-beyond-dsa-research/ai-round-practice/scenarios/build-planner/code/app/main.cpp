// buildplan <manifest> [--workers N]
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "duration.hpp"
#include "graph.hpp"
#include "manifest.hpp"
#include "report.hpp"
#include "scheduler.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: buildplan <manifest> [--workers N]\n";
        return 2;
    }
    int workers = 0;
    for (int i = 2; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--workers") workers = std::stoi(argv[i + 1]);

    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::cerr << "cannot open " << argv[1] << "\n";
        return 1;
    }
    std::stringstream text;
    text << in.rdbuf();
    try {
        BuildGraph g = build_graph(parse_manifest(text.str()));
        if (auto cycle = find_cycle(g); !cycle.empty()) {
            std::cerr << "cycle:";
            for (int v : cycle) std::cerr << " " << g.names[v];
            std::cerr << "\n";
            return 1;
        }
        std::cout << plan_report(g);
        if (workers > 0) {
            Schedule s = simulate(g, workers);
            std::cout << "With " << workers << " workers: " << format_duration(s.makespan_ms) << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
