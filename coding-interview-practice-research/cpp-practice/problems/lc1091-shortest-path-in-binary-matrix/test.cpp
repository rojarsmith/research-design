// 測試：lc1091-shortest-path-in-binary-matrix。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc1091-shortest-path-in-binary-matrix（VS Code 工作：pp: 開始/重做這題）"
#endif

// 路徑長度算的是經過的格子數（包含起點和終點），可以往 8 個方向走。
static int call(vector<vector<int>> grid) { return Solution().shortestPathBinaryMatrix(grid); }

TEST("one diagonal step") { EXPECT_EQ(call({{0, 1}, {1, 0}}), 2); }
TEST("example") { EXPECT_EQ(call({{0, 0, 0}, {1, 1, 0}, {1, 1, 0}}), 4); }
TEST("start blocked") { EXPECT_EQ(call({{1, 0, 0}, {1, 1, 0}, {1, 1, 0}}), -1); }
TEST("end blocked") { EXPECT_EQ(call({{0, 0}, {0, 1}}), -1); }
TEST("single open cell") { EXPECT_EQ(call({{0}}), 1); }
TEST("single blocked cell") { EXPECT_EQ(call({{1}}), -1); }
TEST("walled in") { EXPECT_EQ(call({{0, 1, 0}, {1, 1, 1}, {0, 1, 0}}), -1); }
TEST("path has to climb back up") { EXPECT_EQ(call({{0, 1, 1, 1}, {0, 1, 0, 1}, {0, 1, 0, 1}, {1, 0, 1, 0}}), 6); }
// 在這裡加你自己的邊界測資：

// ---- oracle：一直鬆弛到沒有變化為止（Bellman–Ford 的網格版） ----
static int oracle(const vector<vector<int>>& g) {
    int n = static_cast<int>(g.size());
    if (g[0][0] || g[static_cast<size_t>(n - 1)][static_cast<size_t>(n - 1)]) return -1;
    const int INF = INT_MAX / 2;
    vector<vector<int>> d(static_cast<size_t>(n), vector<int>(static_cast<size_t>(n), INF));
    d[0][0] = 1;
    for (bool changed = true; changed;) {
        changed = false;
        for (int r = 0; r < n; ++r)
            for (int c = 0; c < n; ++c) {
                if (d[static_cast<size_t>(r)][static_cast<size_t>(c)] == INF) continue;
                for (int dr = -1; dr <= 1; ++dr)
                    for (int dc = -1; dc <= 1; ++dc) {
                        int nr = r + dr, nc = c + dc;
                        if (nr < 0 || nr >= n || nc < 0 || nc >= n || g[static_cast<size_t>(nr)][static_cast<size_t>(nc)]) continue;
                        int cand = d[static_cast<size_t>(r)][static_cast<size_t>(c)] + 1;
                        int& cur = d[static_cast<size_t>(nr)][static_cast<size_t>(nc)];
                        if (cand < cur) {
                            cur = cand;
                            changed = true;
                        }
                    }
            }
    }
    int ans = d[static_cast<size_t>(n - 1)][static_cast<size_t>(n - 1)];
    return ans == INF ? -1 : ans;
}

STRESS("random grids vs repeated relaxation", 500) {
    int n = rng.range(1, size);
    vector<vector<int>> g(static_cast<size_t>(n), vector<int>(static_cast<size_t>(n), 0));
    for (auto& row : g)
        for (auto& x : row) x = rng.coin(0.3) ? 1 : 0;
    if (rng.coin(0.8)) g[0][0] = g.back().back() = 0;
    CHECK_SAME(call(g), oracle(g), g);
}

// 蛇形迷宮：奇數列整列是牆，只在右端、左端交替留一個缺口。最短路徑必須穿過每個缺口，
// 從一個缺口到下一個缺口要 n - 1 步，所以 W 道牆共 W·(n - 1) 步，再走 1 步到終點；格數比步數多 1。
TEST("perf 99x99 snake maze and 100x100 open grid (trying every path is exponential)") {
    const int n = 99, walls = (n - 1) / 2;
    vector<vector<int>> maze(n, vector<int>(n, 0));
    for (int r = 1; r < n; r += 2) {
        for (int c = 0; c < n; ++c) maze[static_cast<size_t>(r)][static_cast<size_t>(c)] = 1;
        maze[static_cast<size_t>(r)][static_cast<size_t>(r % 4 == 1 ? n - 1 : 0)] = 0;
    }
    vector<vector<int>> open(100, vector<int>(100, 0));
    int a = 0, b = 0;
    double ms = judge::time_ms([&] {
        a = call(maze);
        b = call(open);
    });
    EXPECT_UNDER_MS(ms, 50);
    EXPECT_EQ(a, walls * (n - 1) + 2);
    EXPECT_EQ(b, 100);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
