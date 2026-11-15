// 測試：lc0974-subarray-sums-divisible-by-k。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0974-subarray-sums-divisible-by-k（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> nums, int k) { return Solution().subarraysDivByK(nums, k); }

TEST("example") { EXPECT_EQ(call({4, 5, 0, -2, -3, 1}, 5), 7); }
TEST("single element, not divisible") { EXPECT_EQ(call({5}, 9), 0); }
TEST("single element, divisible") { EXPECT_EQ(call({5}, 5), 1); }
TEST("negative multiple of k") { EXPECT_EQ(call({-5}, 5), 1); }
TEST("negative remainder must be fixed up") { EXPECT_EQ(call({-1, 2, 9}, 2), 2); }
TEST("negative and positive prefix sums in the same class") { EXPECT_EQ(call({-1, 5}, 5), 1); }
TEST("all zeros: every subarray counts") { EXPECT_EQ(call({0, 0, 0}, 2), 6); }
// 在這裡加你自己的邊界測資：

// ---- oracle：列出每個子陣列，直接算總和 ----
static int oracle(const vector<int>& nums, int k) {
    int count = 0;
    for (size_t i = 0; i < nums.size(); ++i) {
        long long sum = 0;
        for (size_t j = i; j < nums.size(); ++j) {
            sum += nums[j];
            if (sum % k == 0) ++count;
        }
    }
    return count;
}

STRESS("random arrays with negatives vs oracle", 600) {
    auto nums = rng.vec(rng.range(1, size * 3), -9, 9);
    int k = rng.range(2, 7);
    CHECK_SAME(call(nums, k), oracle(nums, k), nums, k);
}

TEST("perf n=30000, every subarray divisible (O(n^2) is too slow)") {
    const int n = 30000;
    vector<int> nums(n);
    for (int i = 0; i < n; ++i) nums[static_cast<size_t>(i)] = i % 2 == 0 ? -10000 : 10000;
    int got = 0;
    double ms = judge::time_ms([&] { got = call(nums, 10000); });
    EXPECT_UNDER_MS(ms, 100);
    EXPECT_EQ(got, n * (n + 1) / 2);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
