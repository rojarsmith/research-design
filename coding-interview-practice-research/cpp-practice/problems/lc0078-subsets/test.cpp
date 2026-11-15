// 測試：lc0078-subsets。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0078-subsets（VS Code 工作：pp: 開始/重做這題）"
#endif

// 答案順序不限：比對前先排序（normalized）
static vector<vector<int>> call(vector<int> nums) { return normalized(Solution().subsets(nums)); }

TEST("one element") { EXPECT_EQ(call({0}), {{}, {0}}); }
TEST("two elements") { EXPECT_EQ(call({1, 2}), {{}, {1}, {1, 2}, {2}}); }
TEST("three unsorted") { EXPECT_EQ(call({3, 1, 2}), {{}, {1}, {1, 2}, {1, 2, 3}, {1, 3}, {2}, {2, 3}, {3}}); }
TEST("negatives") { EXPECT_EQ(call({-1, 5}), {{}, {-1}, {-1, 5}, {5}}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：位元列舉，mask 的第 i 位代表選不選第 i 個數 ----
static vector<vector<int>> oracle(const vector<int>& a) {
    vector<vector<int>> out;
    for (unsigned mask = 0; mask < (1u << a.size()); ++mask) {
        vector<int> s;
        for (size_t i = 0; i < a.size(); ++i)
            if (mask >> i & 1u) s.push_back(a[i]);
        out.push_back(s);
    }
    return normalized(out);
}

STRESS("random sets vs bitmask enumeration", 300) {
    auto a = rng.distinct(rng.range(1, size), -10, 10);
    CHECK_SAME(call(a), oracle(a), a);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
