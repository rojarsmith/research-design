// 測試：lc0046-permutations。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0046-permutations（VS Code 工作：pp: 開始/重做這題）"
#endif

// 每個排列內的順序有意義，只把排列之間排序（sorted_outer）
static vector<vector<int>> call(vector<int> nums) { return sorted_outer(Solution().permute(nums)); }

TEST("one element") { EXPECT_EQ(call({1}), {{1}}); }
TEST("two elements") { EXPECT_EQ(call({0, 1}), {{0, 1}, {1, 0}}); }
TEST("three elements") { EXPECT_EQ(call({1, 2, 3}), {{1, 2, 3}, {1, 3, 2}, {2, 1, 3}, {2, 3, 1}, {3, 1, 2}, {3, 2, 1}}); }
TEST("negatives") { EXPECT_EQ(call({2, -1}), {{-1, 2}, {2, -1}}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：std::next_permutation ----
static vector<vector<int>> oracle(vector<int> a) {
    sort(a.begin(), a.end());
    vector<vector<int>> out;
    do out.push_back(a);
    while (next_permutation(a.begin(), a.end()));
    return out;
}

STRESS("random sets vs next_permutation", 200) {
    auto a = rng.distinct(rng.range(1, min(size, 6)), -10, 10);
    CHECK_SAME(call(a), oracle(a), a);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
