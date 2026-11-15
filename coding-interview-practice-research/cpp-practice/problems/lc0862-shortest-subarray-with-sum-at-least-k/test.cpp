// 測試：lc0862-shortest-subarray-with-sum-at-least-k。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0862-shortest-subarray-with-sum-at-least-k（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> nums, int k) { return Solution().shortestSubarray(nums, k); }

TEST("single element") { EXPECT_EQ(call({1}, 1), 1); }
TEST("not reachable") { EXPECT_EQ(call({1, 2}, 4), -1); }
TEST("negative in the middle") { EXPECT_EQ(call({2, -1, 2}, 3), 3); }
TEST("negatives break the sliding window") { EXPECT_EQ(call({84, -37, 32, 40, 95}, 167), 3); }
TEST("negative at both ends") { EXPECT_EQ(call({-28, 81, -20, 28, -29}, 89), 3); }
TEST("one big element") { EXPECT_EQ(call({1, 1, 100, 1, 1}, 100), 1); }
TEST("all negative") { EXPECT_EQ(call({-1, -1}, 1), -1); }
TEST("two adjacent values") { EXPECT_EQ(call({17, 85, 93, -45, -21}, 150), 2); }
// 在這裡加你自己的邊界測資：

// ---- oracle：列出每個子陣列（O(n²)），總和用 long long ----
static int oracle(const vector<int>& nums, int k) {
    size_t best = SIZE_MAX;
    for (size_t i = 0; i < nums.size(); ++i) {
        long long sum = 0;
        for (size_t j = i; j < nums.size(); ++j) {
            sum += nums[j];
            if (sum >= k) {
                best = min(best, j - i + 1);
                break;
            }
        }
    }
    return best == SIZE_MAX ? -1 : static_cast<int>(best);
}

STRESS("random arrays with negatives vs oracle", 600) {
    auto nums = rng.vec(rng.range(1, size * 3), -6, 6);
    int k = rng.range(1, 12);
    CHECK_SAME(call(nums, k), oracle(nums, k), nums, k);
}

TEST("perf n=100000, values 10^5, k=10^9 (prefix sums exceed int; O(n^2) is too slow)") {
    vector<int> nums(100000, 100000);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(nums, 1000000000); });
    EXPECT_UNDER_MS(ms, 100);
    EXPECT_EQ(got, 10000);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
