// 測試：lc0743-network-delay-time。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0743-network-delay-time（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<vector<int>> times, int n, int k) { return Solution().networkDelayTime(times, n, k); }

TEST("single node") { EXPECT_EQ(call({}, 1, 1), 0); }
TEST("one edge") { EXPECT_EQ(call({{1, 2, 1}}, 2, 1), 1); }
TEST("edge points the wrong way") { EXPECT_EQ(call({{1, 2, 1}}, 2, 2), -1); }
TEST("detour is shorter") { EXPECT_EQ(call({{1, 2, 4}, {1, 3, 1}, {3, 2, 1}}, 3, 1), 2); }
TEST("zero-weight edge") { EXPECT_EQ(call({{1, 2, 5}, {2, 3, 5}, {1, 3, 20}, {3, 4, 0}}, 4, 1), 10); }
TEST("one node unreachable") { EXPECT_EQ(call({{1, 2, 1}, {2, 1, 1}}, 3, 1), -1); }
TEST("start in the middle") { EXPECT_EQ(call({{2, 1, 3}, {1, 3, 2}, {2, 3, 10}}, 3, 2), 5); }
// 在這裡加你自己的邊界測資：

// ---- oracle：Bellman–Ford，O(V·E) ----
static int oracle(const vector<vector<int>>& times, int n, int k) {
    const long long INF = LLONG_MAX / 4;
    vector<long long> d(static_cast<size_t>(n + 1), INF);
    d[static_cast<size_t>(k)] = 0;
    for (int round = 0; round < n; ++round)
        for (auto& e : times)
            if (d[static_cast<size_t>(e[0])] < INF)
                d[static_cast<size_t>(e[1])] = min(d[static_cast<size_t>(e[1])], d[static_cast<size_t>(e[0])] + e[2]);
    long long worst = 0;
    for (int v = 1; v <= n; ++v) worst = max(worst, d[static_cast<size_t>(v)]);
    return worst >= INF ? -1 : static_cast<int>(worst);
}

STRESS("random graphs vs Bellman-Ford", 500) {
    int n = rng.range(1, size);
    int k = rng.range(1, n);
    vector<vector<int>> times;
    set<pair<int, int>> used;
    int m = rng.range(0, min(n * (n - 1), 3 * n));
    while (static_cast<int>(times.size()) < m) {
        int u = rng.range(1, n), v = rng.range(1, n);
        if (u == v || used.count({u, v})) continue;
        used.insert({u, v});
        times.push_back({u, v, rng.range(0, 9)});
    }
    CHECK_SAME(call(times, n, k), oracle(times, n, k), times, n, k);
}

TEST("perf 100000 nodes, edges in reverse order (Bellman-Ford and O(V^2) Dijkstra are too slow)") {
    const int n = 100000;
    vector<vector<int>> times;
    for (int i = n - 1; i >= 1; --i) {
        times.push_back({i, i + 1, 1});
        if (i + 2 <= n) times.push_back({i, i + 2, 3});
    }
    int got = 0;
    double ms = judge::time_ms([&] { got = call(times, n, 1); });
    EXPECT_UNDER_MS(ms, 500);
    EXPECT_EQ(got, n - 1);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
