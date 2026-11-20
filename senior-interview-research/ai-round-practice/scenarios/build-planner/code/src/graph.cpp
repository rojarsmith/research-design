#include "graph.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <utility>

BuildGraph build_graph(const Manifest& m) {
    BuildGraph g;
    int n = (int)m.targets.size();
    g.names.reserve(n);
    g.duration_ms.reserve(n);
    g.deps.resize(n);
    g.dependents.resize(n);
    for (int i = 0; i < n; ++i) {
        g.names.push_back(m.targets[i].name);
        g.duration_ms.push_back(m.targets[i].duration_ms);
        g.index.emplace(m.targets[i].name, i);
    }
    for (int i = 0; i < n; ++i) {
        const Target& t = m.targets[i];
        for (const std::string& d : t.deps) {
            auto it = g.index.find(d);
            if (it == g.index.end())
                throw GraphError("target '" + t.name + "' (line " + std::to_string(t.line) +
                                 ") depends on unknown target '" + d + "'");
            int j = it->second;
            if (j == i) throw GraphError("target '" + t.name + "' depends on itself");
            // A manifest may list the same dependency twice; keep a single edge.
            if (std::find(g.deps[i].begin(), g.deps[i].end(), j) == g.deps[i].end()) {
                g.deps[i].push_back(j);
                g.dependents[j].push_back(i);
            }
        }
    }
    return g;
}

std::vector<int> find_cycle(const BuildGraph& g) {
    int n = g.size();
    enum Color : char { White, Gray, Black };
    std::vector<char> color(n, White);
    std::vector<int> parent(n, -1);
    // Iterative DFS so deep manifests do not overflow the stack.
    std::vector<std::pair<int, std::size_t>> stack;
    for (int root = 0; root < n; ++root) {
        if (color[root] != White) continue;
        stack.push_back({root, 0});
        color[root] = Gray;
        while (!stack.empty()) {
            auto& [v, next] = stack.back();
            if (next < g.deps[v].size()) {
                int w = g.deps[v][next++];
                if (color[w] == White) {
                    color[w] = Gray;
                    parent[w] = v;
                    stack.push_back({w, 0});
                } else if (color[w] == Gray) {
                    std::vector<int> cycle{w};
                    for (int x = v; x != w; x = parent[x]) cycle.push_back(x);
                    cycle.push_back(w);
                    std::reverse(cycle.begin(), cycle.end());
                    return cycle;
                }
            } else {
                color[v] = Black;
                stack.pop_back();
            }
        }
    }
    return {};
}

std::vector<int> topo_order(const BuildGraph& g) {
    int n = g.size();
    std::vector<int> waiting(n);
    for (int i = 0; i < n; ++i) waiting[i] = (int)g.deps[i].size();
    auto by_name = [&](int a, int b) { return g.names[a] > g.names[b]; };  // min-heap on name
    std::priority_queue<int, std::vector<int>, decltype(by_name)> ready(by_name);
    for (int i = 0; i < n; ++i)
        if (waiting[i] == 0) ready.push(i);
    std::vector<int> order;
    order.reserve(n);
    while (!ready.empty()) {
        int v = ready.top();
        ready.pop();
        order.push_back(v);
        for (int w : g.dependents[v])
            if (--waiting[w] == 0) ready.push(w);
    }
    if ((int)order.size() != n) throw GraphError("dependency cycle detected");
    return order;
}

std::int64_t critical_path_ms(const BuildGraph& g) {
    std::vector<std::int64_t> finish(g.size(), 0);
    std::int64_t best = 0;
    for (int v : topo_order(g)) {
        std::int64_t start = 0;
        for (int d : g.deps[v]) start = std::max(start, finish[d]);
        finish[v] = start + g.duration_ms[v];
        best = std::max(best, finish[v]);
    }
    return best;
}
