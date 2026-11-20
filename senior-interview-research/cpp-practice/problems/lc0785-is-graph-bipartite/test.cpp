// 測試：lc0785-is-graph-bipartite。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0785-is-graph-bipartite（VS Code 工作：pp: 開始/重做這題）"
#endif

static bool call(vector<vector<int>> graph) { return Solution().isBipartite(graph); }

TEST("single node") { EXPECT_EQ(call({{}}), true); }
TEST("one edge") { EXPECT_EQ(call({{1}, {0}}), true); }
TEST("triangle") { EXPECT_EQ(call({{1, 2}, {0, 2}, {0, 1}}), false); }
TEST("square") { EXPECT_EQ(call({{1, 3}, {0, 2}, {1, 3}, {0, 2}}), true); }
TEST("odd cycle in the second component") { EXPECT_EQ(call({{1}, {0}, {3, 4}, {2, 4}, {2, 3}}), false); }
TEST("two separate edges") { EXPECT_EQ(call({{1}, {0}, {3}, {2}}), true); }
// 在這裡加你自己的邊界測資：

// ---- oracle：試遍所有 2^n 種塗色 ----
static bool oracle(const vector<vector<int>>& g) {
    size_t n = g.size();
    for (unsigned mask = 0; mask < (1u << n); ++mask) {
        bool ok = true;
        for (size_t u = 0; u < n && ok; ++u)
            for (int v : g[u])
                if ((mask >> u & 1u) == (mask >> v & 1u)) { ok = false; break; }
        if (ok) return true;
    }
    return false;
}

STRESS("random graphs vs all colorings", 500) {
    int n = rng.range(1, size);
    vector<vector<int>> g(static_cast<size_t>(n));
    set<pair<int, int>> used;
    int m = rng.range(0, n * (n - 1) / 2);
    for (int tries = 0; static_cast<int>(used.size()) < m && tries < 200; ++tries) {
        int a = rng.range(0, n - 1), b = rng.range(0, n - 1);
        if (a == b || used.count({min(a, b), max(a, b)})) continue;
        used.insert({min(a, b), max(a, b)});
        g[static_cast<size_t>(a)].push_back(b);
        g[static_cast<size_t>(b)].push_back(a);
    }
    CHECK_SAME(call(g), oracle(g), g);
}

TEST("perf path of 100000 nodes") {
    const int n = 100000;
    vector<vector<int>> g(n);
    for (int i = 0; i + 1 < n; ++i) {
        g[static_cast<size_t>(i)].push_back(i + 1);
        g[static_cast<size_t>(i + 1)].push_back(i);
    }
    bool got = false;
    double ms = judge::time_ms([&] { got = call(g); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, true);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
