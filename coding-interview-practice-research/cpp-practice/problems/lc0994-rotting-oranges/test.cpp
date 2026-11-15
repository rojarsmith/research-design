// 測試：lc0994-rotting-oranges。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0994-rotting-oranges（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<vector<int>> grid) { return Solution().orangesRotting(grid); }

TEST("empty cell only") { EXPECT_EQ(call({{0}}), 0); }
TEST("fresh with no rotten") { EXPECT_EQ(call({{1}}), -1); }
TEST("rotten only") { EXPECT_EQ(call({{2}}), 0); }
TEST("one row") { EXPECT_EQ(call({{2, 1, 1}}), 2); }
TEST("blocked by an empty cell") { EXPECT_EQ(call({{2, 1, 0, 1}}), -1); }
TEST("two sources spread at the same time") { EXPECT_EQ(call({{2, 1, 1}, {1, 1, 1}, {0, 1, 2}}), 2); }
TEST("source not at the edge") { EXPECT_EQ(call({{1, 2, 1, 1, 1}}), 3); }
TEST("no fresh at the start") { EXPECT_EQ(call({{0, 2, 0}}), 0); }
// 在這裡加你自己的邊界測資：

// ---- oracle：一分鐘一分鐘模擬整張圖 ----
static int oracle(vector<vector<int>> g) {
    int minutes = 0;
    for (;;) {
        auto next = g;
        bool changed = false;
        for (size_t r = 0; r < g.size(); ++r)
            for (size_t c = 0; c < g[r].size(); ++c) {
                if (g[r][c] != 1) continue;
                bool near = (r > 0 && g[r - 1][c] == 2) || (r + 1 < g.size() && g[r + 1][c] == 2) ||
                            (c > 0 && g[r][c - 1] == 2) || (c + 1 < g[r].size() && g[r][c + 1] == 2);
                if (near) { next[r][c] = 2; changed = true; }
            }
        if (!changed) break;
        g = next;
        ++minutes;
    }
    for (auto& row : g)
        for (int v : row)
            if (v == 1) return -1;
    return minutes;
}

STRESS("random grids vs simulation", 500) {
    int rows = rng.range(1, 1 + size / 2), cols = rng.range(1, 1 + size / 2);
    vector<vector<int>> g(static_cast<size_t>(rows));
    for (auto& row : g) row = rng.vec(cols, 0, 2);
    CHECK_SAME(call(g), oracle(g), g);
}

TEST("perf 1000x1000 grid (minute-by-minute simulation is too slow)") {
    vector<vector<int>> g(1000, vector<int>(1000, 1));
    g[0][0] = 2;
    int got = 0;
    double ms = judge::time_ms([&] { got = call(g); });
    EXPECT_UNDER_MS(ms, 500);
    EXPECT_EQ(got, 1998);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
