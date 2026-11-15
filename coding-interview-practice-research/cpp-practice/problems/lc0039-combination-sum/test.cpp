// 測試：lc0039-combination-sum。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0039-combination-sum（VS Code 工作：pp: 開始/重做這題）"
#endif

// 答案順序不限：比對前先排序（normalized）
static vector<vector<int>> call(vector<int> c, int target) { return normalized(Solution().combinationSum(c, target)); }

TEST("impossible") { EXPECT_EQ(call({2}, 1), {}); }
TEST("reuse one candidate") { EXPECT_EQ(call({2}, 4), {{2, 2}}); }
TEST("two ways") { EXPECT_EQ(call({3, 4, 5}, 8), {{3, 5}, {4, 4}}); }
TEST("unsorted candidates") { EXPECT_EQ(call({7, 3, 2}, 7), {{2, 2, 3}, {7}}); }
TEST("all candidates too large") { EXPECT_EQ(call({5, 10}, 3), {}); }
TEST("one and many copies") { EXPECT_EQ(call({2, 4}, 6), {{2, 2, 2}, {2, 4}}); }
TEST("only one combination works") { EXPECT_EQ(call({8, 6, 9}, 17), {{8, 9}}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：用「列表版完全背包」產生所有組合（不同的方法，用來對拍） ----
static vector<vector<int>> oracle(vector<int> c, int target) {
    sort(c.begin(), c.end());
    vector<vector<vector<int>>> ways(static_cast<size_t>(target + 1));
    ways[0] = {{}};
    for (int x : c)
        for (int s = x; s <= target; ++s)
            for (auto comb : ways[static_cast<size_t>(s - x)]) {
                comb.push_back(x);
                ways[static_cast<size_t>(s)].push_back(comb);
            }
    return normalized(ways[static_cast<size_t>(target)]);
}

STRESS("random candidates vs oracle", 400) {
    auto c = rng.distinct(rng.range(1, 4), 2, 12);
    int target = rng.range(1, 4 + size * 2);
    CHECK_SAME(call(c, target), oracle(c, target), c, target);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
