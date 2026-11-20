// 測試：lc0215-kth-largest-element-in-an-array。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0215-kth-largest-element-in-an-array（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> nums, int k) { return Solution().findKthLargest(nums, k); }

TEST("single") { EXPECT_EQ(call({7}, 1), 7); }
TEST("largest") { EXPECT_EQ(call({3, 1, 2}, 1), 3); }
TEST("smallest") { EXPECT_EQ(call({3, 1, 2}, 3), 1); }
TEST("duplicates count separately") { EXPECT_EQ(call({2, 2, 2, 1}, 2), 2); }
TEST("negatives") { EXPECT_EQ(call({-5, -1, -3}, 2), -3); }
TEST("repeated maximum, k = 2") { EXPECT_EQ(call({6, 4, 9, 1, 9, 3}, 2), 9); }
TEST("repeated maximum, k = 3") { EXPECT_EQ(call({6, 4, 9, 1, 9, 3}, 3), 6); }
// 在這裡加你自己的邊界測資：

// ---- oracle：整個排序 ----
static int oracle(vector<int> a, int k) {
    sort(a.rbegin(), a.rend());
    return a[static_cast<size_t>(k - 1)];
}

STRESS("random vs oracle", 600) {
    auto a = rng.vec(rng.range(1, size * 2), -5, 5);
    int k = rng.range(1, static_cast<int>(a.size()));
    CHECK_SAME(call(a, k), oracle(a, k), a, k);
}

TEST("perf n=100000 all equal (naive quickselect degrades to O(n^2))") {
    vector<int> a(100000, 1);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(a, 50000); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, 1);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
