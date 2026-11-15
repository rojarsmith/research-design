// 測試：lc0200-number-of-islands。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0200-number-of-islands（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<vector<char>> grid) { return Solution().numIslands(grid); }

TEST("water only") { EXPECT_EQ(call({{'0'}}), 0); }
TEST("one cell of land") { EXPECT_EQ(call({{'1'}}), 1); }
TEST("two separated cells") { EXPECT_EQ(call({{'1', '0', '1'}}), 2); }
TEST("one block") { EXPECT_EQ(call({{'1', '1'}, {'1', '1'}}), 1); }
TEST("diagonal is not connected") { EXPECT_EQ(call({{'1', '0'}, {'0', '1'}}), 2); }
TEST("three islands") {
    EXPECT_EQ(call({{'1', '1', '0', '0'}, {'1', '0', '0', '1'}, {'0', '0', '1', '1'}, {'0', '1', '0', '0'}}), 3);
}
// 在這裡加你自己的邊界測資：

// ---- oracle：併查集把相鄰的陸地合併，再數有幾個代表元 ----
static int find_root(vector<int>& p, int x) { return p[static_cast<size_t>(x)] == x ? x : p[static_cast<size_t>(x)] = find_root(p, p[static_cast<size_t>(x)]); }
static int oracle(const vector<vector<char>>& g) {
    int R = static_cast<int>(g.size()), C = static_cast<int>(g[0].size());
    vector<int> p(static_cast<size_t>(R * C));
    iota(p.begin(), p.end(), 0);
    auto land = [&](int r, int c) { return g[static_cast<size_t>(r)][static_cast<size_t>(c)] == '1'; };
    for (int r = 0; r < R; ++r)
        for (int c = 0; c < C; ++c) {
            if (!land(r, c)) continue;
            if (r + 1 < R && land(r + 1, c)) p[static_cast<size_t>(find_root(p, r * C + c))] = find_root(p, (r + 1) * C + c);
            if (c + 1 < C && land(r, c + 1)) p[static_cast<size_t>(find_root(p, r * C + c))] = find_root(p, r * C + c + 1);
        }
    int count = 0;
    for (int r = 0; r < R; ++r)
        for (int c = 0; c < C; ++c) count += land(r, c) && find_root(p, r * C + c) == r * C + c;
    return count;
}

STRESS("random grids vs union-find", 500) {
    int R = rng.range(1, size), C = rng.range(1, size);
    vector<vector<char>> g(static_cast<size_t>(R));
    for (auto& row : g) {
        string s = rng.str(C, "01");
        row.assign(s.begin(), s.end());
    }
    CHECK_SAME(call(g), oracle(g), g);
}

TEST("perf 1000x1000 all land (deep DFS must not crash)") {
    vector<vector<char>> g(1000, vector<char>(1000, '1'));
    int got = 0;
    double ms = judge::time_ms([&] { got = call(g); });
    EXPECT_UNDER_MS(ms, 500);
    EXPECT_EQ(got, 1);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
