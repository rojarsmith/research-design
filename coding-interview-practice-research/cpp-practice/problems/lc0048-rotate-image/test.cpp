// 測試：lc0048-rotate-image。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0048-rotate-image（VS Code 工作：pp: 開始/重做這題）"
#endif

static vector<vector<int>> call(vector<vector<int>> m) {
    Solution().rotate(m);
    return m;
}

TEST("1x1") { EXPECT_EQ(call({{1}}), {{1}}); }
TEST("2x2") { EXPECT_EQ(call({{1, 2}, {3, 4}}), {{3, 1}, {4, 2}}); }
TEST("3x3") { EXPECT_EQ(call({{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}), {{7, 4, 1}, {8, 5, 2}, {9, 6, 3}}); }
TEST("4x4") {
    EXPECT_EQ(call({{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}}),
              {{13, 9, 5, 1}, {14, 10, 6, 2}, {15, 11, 7, 3}, {16, 12, 8, 4}});
}
TEST("negatives and repeats") { EXPECT_EQ(call({{-1, -1}, {0, 5}}), {{0, -1}, {5, -1}}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：開一個新矩陣，照座標公式搬過去 ----
static vector<vector<int>> oracle(const vector<vector<int>>& m) {
    size_t n = m.size();
    vector<vector<int>> r(n, vector<int>(n));
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j) r[j][n - 1 - i] = m[i][j];
    return r;
}

STRESS("random matrices vs oracle", 400) {
    int n = rng.range(1, size);
    vector<vector<int>> m(static_cast<size_t>(n));
    for (auto& row : m) row = rng.vec(n, -9, 9);
    CHECK_SAME(call(m), oracle(m), m);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
