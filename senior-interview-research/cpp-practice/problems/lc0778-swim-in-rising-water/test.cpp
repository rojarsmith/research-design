// 測試：lc0778-swim-in-rising-water。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0778-swim-in-rising-water（VS Code 工作：pp: 開始/重做這題）"
#endif

// 答案是「從左上到右下的路徑上，最高的格子」在所有路徑中的最小值；只能往上下左右走。
static int call(vector<vector<int>> grid) { return Solution().swimInWater(grid); }

TEST("2x2") { EXPECT_EQ(call({{0, 2}, {1, 3}}), 3); }
TEST("spiral") {
    EXPECT_EQ(call({{0, 1, 2, 3, 4}, {24, 23, 22, 21, 5}, {12, 13, 14, 15, 16}, {11, 17, 18, 19, 20}, {10, 9, 8, 7, 6}}), 16);
}
TEST("single cell") { EXPECT_EQ(call({{0}}), 0); }
TEST("the start cell counts") { EXPECT_EQ(call({{3, 2}, {0, 1}}), 3); }
TEST("low route around a high center") { EXPECT_EQ(call({{0, 1, 2}, {7, 8, 3}, {6, 5, 4}}), 4); }
TEST("minimize the maximum, not the sum") { EXPECT_EQ(call({{0, 5, 4}, {1, 6, 3}, {8, 7, 2}}), 5); }
// 在這裡加你自己的邊界測資：

// ---- oracle：t 從 0 開始一個一個試，每次用 flood fill 看只走 <= t 的格子能不能到終點（O(n⁴)） ----
static int oracle(const vector<vector<int>>& g) {
    int n = static_cast<int>(g.size());
    const int dr[] = {1, -1, 0, 0}, dc[] = {0, 0, 1, -1};
    for (int t = 0;; ++t) {
        if (g[0][0] > t) continue;
        vector<vector<char>> seen(static_cast<size_t>(n), vector<char>(static_cast<size_t>(n), 0));
        vector<pair<int, int>> todo{{0, 0}};
        seen[0][0] = 1;
        while (!todo.empty()) {
            auto [r, c] = todo.back();
            todo.pop_back();
            for (int d = 0; d < 4; ++d) {
                int nr = r + dr[d], nc = c + dc[d];
                if (nr < 0 || nr >= n || nc < 0 || nc >= n) continue;
                size_t ur = static_cast<size_t>(nr), uc = static_cast<size_t>(nc);
                if (seen[ur][uc] || g[ur][uc] > t) continue;
                seen[ur][uc] = 1;
                todo.push_back({nr, nc});
            }
        }
        if (seen.back().back()) return t;
    }
}

static vector<vector<int>> grid_from(const vector<int>& vals, int n) {
    vector<vector<int>> g(static_cast<size_t>(n), vector<int>(static_cast<size_t>(n)));
    for (int i = 0; i < n * n; ++i) g[static_cast<size_t>(i / n)][static_cast<size_t>(i % n)] = vals[static_cast<size_t>(i)];
    return g;
}

STRESS("random permutations of 0..n^2-1 vs oracle", 500) {
    int n = rng.range(1, size);
    auto g = grid_from(rng.distinct(n * n, 0, n * n - 1), n);
    CHECK_SAME(call(g), oracle(g), g);
}

TEST("perf 300x300, the goal is the highest cell (above LeetCode's 50; trying every t is O(n^4))") {
    const int n = 300;
    judge::Rng r(778);
    auto vals = r.distinct(n * n, 0, n * n - 1);
    swap(*max_element(vals.begin(), vals.end()), vals.back());
    auto g = grid_from(vals, n);
    int got = -1;
    double ms = judge::time_ms([&] { got = call(g); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, n * n - 1);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
