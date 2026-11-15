// 測試：lc0300-longest-increasing-subsequence。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0300-longest-increasing-subsequence（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> nums) { return Solution().lengthOfLIS(nums); }

TEST("single") { EXPECT_EQ(call({7}), 1); }
TEST("all equal (strictly increasing only)") { EXPECT_EQ(call({3, 3, 3}), 1); }
TEST("already increasing") { EXPECT_EQ(call({1, 2, 3}), 3); }
TEST("decreasing") { EXPECT_EQ(call({3, 2, 1}), 1); }
TEST("mixed") { EXPECT_EQ(call({10, 9, 2, 5, 3, 7, 101, 18}), 4); }
TEST("two interleaved runs") { EXPECT_EQ(call({1, 5, 2, 6, 3, 7}), 4); }
TEST("negatives") { EXPECT_EQ(call({-2, -1}), 2); }
// 在這裡加你自己的邊界測資：

// ---- oracle：列舉所有子序列 ----
static int oracle(const vector<int>& a) {
    int best = 0;
    for (unsigned mask = 1; mask < (1u << a.size()); ++mask) {
        int len = 0, last = INT_MIN;
        bool ok = true;
        for (size_t i = 0; i < a.size() && ok; ++i)
            if (mask >> i & 1u) {
                ok = len == 0 || a[i] > last;
                last = a[i];
                ++len;
            }
        if (ok) best = max(best, len);
    }
    return best;
}

STRESS("random arrays vs subsequence enumeration", 500) {
    auto a = rng.vec(rng.range(1, min(12, size + 2)), -5, 5);
    CHECK_SAME(call(a), oracle(a), a);
}

TEST("perf n=100000 increasing (O(n^2) DP is too slow)") {
    vector<int> a(100000);
    iota(a.begin(), a.end(), -50000);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(a); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, 100000);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
