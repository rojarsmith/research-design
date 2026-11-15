// 測試：lc0054-spiral-matrix。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0054-spiral-matrix（VS Code 工作：pp: 開始/重做這題）"
#endif

static vector<int> call(vector<vector<int>> m) { return Solution().spiralOrder(m); }

TEST("1x1") { EXPECT_EQ(call({{1}}), {1}); }
TEST("single row") { EXPECT_EQ(call({{1, 2, 3}}), {1, 2, 3}); }
TEST("single column") { EXPECT_EQ(call({{1}, {2}, {3}}), {1, 2, 3}); }
TEST("2x2") { EXPECT_EQ(call({{1, 2}, {3, 4}}), {1, 2, 4, 3}); }
TEST("wide") { EXPECT_EQ(call({{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}}), {1, 2, 3, 4, 8, 12, 11, 10, 9, 5, 6, 7}); }
TEST("tall") { EXPECT_EQ(call({{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}}), {1, 2, 3, 6, 9, 12, 11, 10, 7, 4, 5, 8}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：照方向前進，碰到邊界或走過的格子就右轉 ----
static vector<int> oracle(const vector<vector<int>>& m) {
    int R = static_cast<int>(m.size()), C = static_cast<int>(m[0].size());
    vector<vector<char>> seen(static_cast<size_t>(R), vector<char>(static_cast<size_t>(C), 0));
    const int dr[] = {0, 1, 0, -1}, dc[] = {1, 0, -1, 0};
    vector<int> out;
    int r = 0, c = 0, d = 0;
    for (int k = 0; k < R * C; ++k) {
        out.push_back(m[static_cast<size_t>(r)][static_cast<size_t>(c)]);
        seen[static_cast<size_t>(r)][static_cast<size_t>(c)] = 1;
        int nr = r + dr[d], nc = c + dc[d];
        if (nr < 0 || nc < 0 || nr >= R || nc >= C || seen[static_cast<size_t>(nr)][static_cast<size_t>(nc)]) {
            d = (d + 1) % 4;
            nr = r + dr[d];
            nc = c + dc[d];
        }
        r = nr;
        c = nc;
    }
    return out;
}

STRESS("random shapes vs oracle", 500) {
    int R = rng.range(1, size), C = rng.range(1, size), v = 0;
    vector<vector<int>> m(static_cast<size_t>(R), vector<int>(static_cast<size_t>(C)));
    for (auto& row : m)
        for (auto& x : row) x = v++;  // distinct values, so a repeated cell shows up
    CHECK_SAME(call(m), oracle(m), m);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
