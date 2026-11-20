// 測試：lc0560-subarray-sum-equals-k。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0560-subarray-sum-equals-k（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> nums, int k) { return Solution().subarraySum(nums, k); }

TEST("single match") { EXPECT_EQ(call({1}, 1), 1); }
TEST("single no match") { EXPECT_EQ(call({1}, 0), 0); }
TEST("zeros, k = 0") { EXPECT_EQ(call({0, 0, 0}, 0), 6); }
TEST("alternating signs") { EXPECT_EQ(call({1, -1, 1, -1}, 0), 4); }
TEST("mixed with negatives") { EXPECT_EQ(call({3, 4, 7, 2, -3, 1, 4, 2}, 7), 4); }
TEST("negative prefix") { EXPECT_EQ(call({-1, -1, 1}, 0), 1); }
TEST("overlapping windows") { EXPECT_EQ(call({2, 2, 2}, 4), 2); }
// 在這裡加你自己的邊界測資：

// ---- oracle：O(n²) 枚舉所有子陣列 ----
static int oracle(const vector<int>& a, int k) {
    int count = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        long long sum = 0;
        for (size_t j = i; j < a.size(); ++j) count += (sum += a[j]) == k;
    }
    return count;
}

STRESS("random vs oracle", 600) {
    auto a = rng.vec(rng.range(1, size * 2), -3, 3);
    int k = rng.range(-3, 3);
    CHECK_SAME(call(a, k), oracle(a, k), a, k);
}

TEST("perf n=100000 (O(n^2) is too slow)") {
    const int n = 100000;
    vector<int> a(n, 1);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(a, 1000); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, n - 999);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
