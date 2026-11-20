// 測試：lc0787-cheapest-flights-within-k-stops。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0787-cheapest-flights-within-k-stops（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(int n, vector<vector<int>> flights, int src, int dst, int k) {
    return Solution().findCheapestPrice(n, flights, src, dst, k);
}

TEST("one stop allowed") { EXPECT_EQ(call(3, {{0, 1, 100}, {1, 2, 100}, {0, 2, 500}}, 0, 2, 1), 200); }
TEST("no stops allowed") { EXPECT_EQ(call(3, {{0, 1, 100}, {1, 2, 100}, {0, 2, 500}}, 0, 2, 0), 500); }
TEST("cheapest route has too many stops") { EXPECT_EQ(call(4, {{0, 1, 1}, {1, 2, 1}, {2, 3, 1}, {0, 3, 10}}, 0, 3, 1), 10); }
TEST("enough stops for the cheap route") { EXPECT_EQ(call(4, {{0, 1, 1}, {1, 2, 1}, {2, 3, 1}, {0, 3, 10}}, 0, 3, 2), 3); }
TEST("no flights") { EXPECT_EQ(call(2, {}, 0, 1, 0), -1); }
TEST("reachable only with a stop") { EXPECT_EQ(call(3, {{0, 1, 5}, {1, 2, 5}}, 0, 2, 0), -1); }
// 在這裡加你自己的邊界測資：

// ---- oracle：DFS 列舉所有最多 k + 1 段的航線（指數時間，只適合小圖） ----
static void oracle_dfs(const vector<vector<int>>& f, int u, int dst, int legs_left, int cost, int& best) {
    if (u == dst) { best = min(best, cost); return; }
    if (legs_left == 0) return;
    for (auto& e : f)
        if (e[0] == u) oracle_dfs(f, e[1], dst, legs_left - 1, cost + e[2], best);
}
static int oracle(const vector<vector<int>>& f, int src, int dst, int k) {
    int best = INT_MAX;
    oracle_dfs(f, src, dst, k + 1, 0, best);
    return best == INT_MAX ? -1 : best;
}

STRESS("random graphs vs path enumeration", 500) {
    int n = rng.range(2, 2 + size / 3);  // at most 5 nodes: the oracle enumerates every route
    vector<vector<int>> f;
    set<pair<int, int>> used;
    int m = rng.range(0, n * (n - 1));
    for (int tries = 0; static_cast<int>(f.size()) < m && tries < 200; ++tries) {
        int a = rng.range(0, n - 1), b = rng.range(0, n - 1);
        if (a == b || used.count({a, b})) continue;
        used.insert({a, b});
        f.push_back({a, b, rng.range(1, 9)});
    }
    int src = rng.range(0, n - 1), dst = rng.range(0, n - 1);
    if (src == dst) dst = (dst + 1) % n;
    int k = rng.range(0, n - 1);
    CHECK_SAME(call(n, f, src, dst, k), oracle(f, src, dst, k), n, f, src, dst, k);
}

TEST("perf n=100, all forward flights (enumerating routes is too slow)") {
    const int n = 100;
    vector<vector<int>> f;
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j) f.push_back({i, j, (j - i) * (j - i)});
    int got = 0;
    double ms = judge::time_ms([&] { got = call(n, f, 0, n - 1, n - 1); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, n - 1);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
