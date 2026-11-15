// 測試：lc0684-redundant-connection。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0684-redundant-connection（VS Code 工作：pp: 開始/重做這題）"
#endif

static vector<int> call(vector<vector<int>> edges) { return Solution().findRedundantConnection(edges); }

TEST("triangle") { EXPECT_EQ(call({{1, 2}, {1, 3}, {2, 3}}), {2, 3}); }
TEST("square with a tail") { EXPECT_EQ(call({{1, 2}, {2, 3}, {3, 4}, {1, 4}, {1, 5}}), {1, 4}); }
TEST("cycle edges are not adjacent in the input") { EXPECT_EQ(call({{3, 4}, {1, 2}, {2, 4}, {3, 5}, {2, 5}}), {2, 5}); }
TEST("cycle closes before the last edge") { EXPECT_EQ(call({{1, 4}, {3, 4}, {1, 3}, {1, 2}, {4, 5}}), {1, 3}); }
TEST("last edge closes the cycle") { EXPECT_EQ(call({{2, 3}, {1, 2}, {1, 3}}), {1, 3}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：從最後一條邊往前試著刪掉，剩下的圖連通就是答案 ----
static vector<int> oracle(const vector<vector<int>>& edges) {
    int n = static_cast<int>(edges.size());
    for (int skip = n - 1; skip >= 0; --skip) {
        vector<vector<int>> adj(static_cast<size_t>(n + 1));
        for (int i = 0; i < n; ++i)
            if (i != skip) {
                adj[static_cast<size_t>(edges[static_cast<size_t>(i)][0])].push_back(edges[static_cast<size_t>(i)][1]);
                adj[static_cast<size_t>(edges[static_cast<size_t>(i)][1])].push_back(edges[static_cast<size_t>(i)][0]);
            }
        vector<char> seen(static_cast<size_t>(n + 1), 0);
        vector<int> st{1};
        seen[1] = 1;
        int count = 0;
        while (!st.empty()) {
            int u = st.back();
            st.pop_back();
            ++count;
            for (int v : adj[static_cast<size_t>(u)])
                if (!seen[static_cast<size_t>(v)]) { seen[static_cast<size_t>(v)] = 1; st.push_back(v); }
        }
        if (count == n) return edges[static_cast<size_t>(skip)];
    }
    return {};
}

STRESS("random trees plus one edge vs oracle", 500) {
    int n = rng.range(3, 3 + size);
    vector<vector<int>> edges;
    set<pair<int, int>> used;
    for (int v = 2; v <= n; ++v) {
        int p = rng.range(1, v - 1);
        edges.push_back({p, v});
        used.insert({p, v});
    }
    for (;;) {
        int a = rng.range(1, n), b = rng.range(1, n);
        if (a > b) swap(a, b);
        if (a == b || used.count({a, b})) continue;
        edges.push_back({a, b});
        break;
    }
    rng.shuffle(edges);
    CHECK_SAME(call(edges), oracle(edges), edges);
}

TEST("perf chain of 100000 nodes plus one edge (a DFS per edge is too slow)") {
    const int n = 100000;
    vector<vector<int>> edges;
    for (int i = 1; i < n; ++i) edges.push_back({i, i + 1});
    edges.push_back({1, n});
    vector<int> got;
    double ms = judge::time_ms([&] { got = call(edges); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, {1, n});
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
