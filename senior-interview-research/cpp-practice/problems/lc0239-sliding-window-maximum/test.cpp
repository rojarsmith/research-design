// 測試：lc0239-sliding-window-maximum。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0239-sliding-window-maximum（VS Code 工作：pp: 開始/重做這題）"
#endif

static vector<int> call(vector<int> nums, int k) { return Solution().maxSlidingWindow(nums, k); }

TEST("single element") { EXPECT_EQ(call({5}, 1), {5}); }
TEST("window is the whole array") { EXPECT_EQ(call({1, 2, 3}, 3), {3}); }
TEST("maximum stays for two windows") { EXPECT_EQ(call({4, 2, 12, 3, 8, 1}, 2), {4, 12, 12, 8, 8}); }
TEST("decreasing") { EXPECT_EQ(call({9, 8, 7, 6}, 2), {9, 8, 7}); }
TEST("all equal negatives") { EXPECT_EQ(call({-1, -1, -1}, 2), {-1, -1}); }
TEST("maximum leaves the window") { EXPECT_EQ(call({1, 3, 1, 2, 0, 5}, 3), {3, 3, 2, 5}); }
TEST("k = 1") { EXPECT_EQ(call({7, 2, 4}, 1), {7, 2, 4}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：每個視窗掃一遍，O(n·k) ----
static vector<int> oracle(const vector<int>& a, int k) {
    vector<int> out;
    for (size_t i = 0; i + static_cast<size_t>(k) <= a.size(); ++i)
        out.push_back(*max_element(a.begin() + static_cast<long>(i), a.begin() + static_cast<long>(i) + k));
    return out;
}

STRESS("random vs oracle", 600) {
    auto a = rng.vec(rng.range(1, size * 2), -5, 5);
    int k = rng.range(1, static_cast<int>(a.size()));
    CHECK_SAME(call(a, k), oracle(a, k), a, k);
}

TEST("perf n=100000, k=50000 (O(n*k) is too slow)") {
    const int n = 100000, k = 50000;
    vector<int> a(n), expect;
    for (int i = 0; i < n; ++i) a[static_cast<size_t>(i)] = n - i;
    for (int i = 0; i + k <= n; ++i) expect.push_back(n - i);
    vector<int> got;
    double ms = judge::time_ms([&] { got = call(a, k); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, expect);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
