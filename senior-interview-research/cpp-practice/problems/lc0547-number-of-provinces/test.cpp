// 測試：lc0547-number-of-provinces。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0547-number-of-provinces（VS Code 工作：pp: 開始/重做這題）"
#endif

// 輸入是鄰接矩陣：isConnected[i][j] = 1 代表 i、j 直接相連；矩陣對稱，對角線都是 1。
static int call(vector<vector<int>> m) { return Solution().findCircleNum(m); }

TEST("example") { EXPECT_EQ(call({{1, 1, 0}, {1, 1, 0}, {0, 0, 1}}), 2); }
TEST("no connections") { EXPECT_EQ(call({{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}), 3); }
TEST("single city") { EXPECT_EQ(call({{1}}), 1); }
TEST("everyone connected") { EXPECT_EQ(call({{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}}), 1); }
TEST("connected through a third city") { EXPECT_EQ(call({{1, 1, 0}, {1, 1, 1}, {0, 1, 1}}), 1); }
TEST("chain 0-3-2-1") { EXPECT_EQ(call({{1, 0, 0, 1}, {0, 1, 1, 0}, {0, 1, 1, 1}, {1, 0, 1, 1}}), 1); }
TEST("two pairs") { EXPECT_EQ(call({{1, 0, 0, 1}, {0, 1, 1, 0}, {0, 1, 1, 0}, {1, 0, 0, 1}}), 2); }
// 在這裡加你自己的邊界測資：

// ---- oracle：Floyd–Warshall 算出誰能到誰，再數「沒有更小編號的城市能到它」的城市（O(n³)） ----
static int oracle(vector<vector<int>> r) {
    size_t n = r.size();
    for (size_t k = 0; k < n; ++k)
        for (size_t i = 0; i < n; ++i)
            for (size_t j = 0; j < n; ++j)
                if (r[i][k] && r[k][j]) r[i][j] = 1;
    int groups = 0;
    for (size_t i = 0; i < n; ++i) {
        bool first = true;
        for (size_t j = 0; j < i; ++j)
            if (r[i][j]) first = false;
        groups += first;
    }
    return groups;
}

STRESS("random symmetric matrices vs oracle", 500) {
    size_t n = static_cast<size_t>(rng.range(1, size));
    vector<vector<int>> m(n, vector<int>(n, 0));
    for (size_t i = 0; i < n; ++i) {
        m[i][i] = 1;
        for (size_t j = i + 1; j < n; ++j) m[i][j] = m[j][i] = rng.coin(0.25) ? 1 : 0;
    }
    CHECK_SAME(call(m), oracle(m), m);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
