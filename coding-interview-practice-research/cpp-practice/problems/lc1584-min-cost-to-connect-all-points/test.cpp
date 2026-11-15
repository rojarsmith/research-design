// 測試：lc1584-min-cost-to-connect-all-points。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc1584-min-cost-to-connect-all-points（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<vector<int>> points) { return Solution().minCostConnectPoints(points); }

TEST("single point") { EXPECT_EQ(call({{0, 0}}), 0); }
TEST("two points") { EXPECT_EQ(call({{0, 0}, {2, 2}}), 4); }
TEST("points on a line") { EXPECT_EQ(call({{0, 0}, {1, 0}, {2, 0}}), 2); }
TEST("three points") { EXPECT_EQ(call({{3, 12}, {-2, 5}, {-4, 1}}), 18); }
TEST("square") { EXPECT_EQ(call({{0, 0}, {0, 5}, {5, 0}, {5, 5}}), 15); }
// 在這裡加你自己的邊界測資：

// ---- oracle：試遍所有 n - 1 條邊的組合，連通且總長最小的就是答案（只適合很少的點） ----
static int oracle(const vector<vector<int>>& p) {
    int n = static_cast<int>(p.size());
    if (n == 1) return 0;
    vector<tuple<int, int, int>> edges;
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            edges.push_back({abs(p[static_cast<size_t>(i)][0] - p[static_cast<size_t>(j)][0]) +
                                 abs(p[static_cast<size_t>(i)][1] - p[static_cast<size_t>(j)][1]),
                             i, j});
    int m = static_cast<int>(edges.size()), best = INT_MAX;
    for (unsigned mask = 0; mask < (1u << m); ++mask) {
        if (__builtin_popcount(mask) != n - 1) continue;
        vector<int> comp(static_cast<size_t>(n));
        iota(comp.begin(), comp.end(), 0);
        int cost = 0;
        for (int e = 0; e < m; ++e)
            if (mask >> e & 1u) {
                auto [w, a, b] = edges[static_cast<size_t>(e)];
                cost += w;
                int ca = comp[static_cast<size_t>(a)], cb = comp[static_cast<size_t>(b)];
                for (auto& c : comp) if (c == cb) c = ca;
            }
        if (count(comp.begin(), comp.end(), comp[0]) == n) best = min(best, cost);
    }
    return best;
}

STRESS("random point sets vs all spanning trees", 300) {
    int n = rng.range(1, min(5, 1 + size / 2));
    set<pair<int, int>> pts;
    while (static_cast<int>(pts.size()) < n) pts.insert({rng.range(-5, 5), rng.range(-5, 5)});
    vector<vector<int>> p;
    for (auto [x, y] : pts) p.push_back({x, y});
    rng.shuffle(p);
    CHECK_SAME(call(p), oracle(p), p);
}

TEST("perf 1000 points (Prim with an O(n^2) scan per step is too slow)") {
    vector<vector<int>> p;
    for (int i = 0; i < 1000; ++i) p.push_back({i, (i % 2) * 1000000});
    int got = 0;
    double ms = judge::time_ms([&] { got = call(p); });
    EXPECT_UNDER_MS(ms, 500);
    EXPECT_EQ(got, 2 * 998 + 1000001);  // two chains of step 2, plus one jump of 1 + 10^6
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
