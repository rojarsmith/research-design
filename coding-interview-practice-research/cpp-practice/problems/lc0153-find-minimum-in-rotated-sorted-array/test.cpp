// 測試：lc0153-find-minimum-in-rotated-sorted-array。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0153-find-minimum-in-rotated-sorted-array（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> nums) { return Solution().findMin(nums); }

TEST("single") { EXPECT_EQ(call({-5}), -5); }
TEST("two, rotated") { EXPECT_EQ(call({2, 1}), 1); }
TEST("two, sorted") { EXPECT_EQ(call({1, 2}), 1); }
TEST("rotated in the middle") { EXPECT_EQ(call({3, 4, 5, 1, 2}), 1); }
TEST("minimum is zero") { EXPECT_EQ(call({4, 5, 6, 7, 0, 1, 2}), 0); }
TEST("not rotated") { EXPECT_EQ(call({11, 13, 15, 17}), 11); }
TEST("minimum is last") { EXPECT_EQ(call({2, 3, 4, 5, 1}), 1); }
// 在這裡加你自己的邊界測資：

// ---- oracle：min_element ----
static int oracle(const vector<int>& a) { return *min_element(a.begin(), a.end()); }

STRESS("random rotations vs min_element", 600) {
    auto a = rng.distinct(rng.range(1, size * 2), -20, 20);
    sort(a.begin(), a.end());
    rotate(a.begin(), a.begin() + rng.range(0, static_cast<int>(a.size()) - 1), a.end());
    CHECK_SAME(call(a), oracle(a), a);
}

TEST("perf 100000 calls on n=100000 (a linear scan per call is too slow)") {
    const int n = 100000;
    vector<int> a(n);
    for (int i = 0; i < n; ++i) a[static_cast<size_t>(i)] = (i + 61000) % n;
    long long sum = 0;
    double ms = judge::time_ms([&] {
        Solution s;
        for (int k = 0; k < n; ++k) sum += s.findMin(a);
    });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(sum, 0LL);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
