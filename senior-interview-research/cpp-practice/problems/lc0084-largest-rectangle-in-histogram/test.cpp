// 測試：lc0084-largest-rectangle-in-histogram。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0084-largest-rectangle-in-histogram（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> heights) { return Solution().largestRectangleArea(heights); }

TEST("example") { EXPECT_EQ(call({2, 1, 5, 6, 2, 3}), 10); }
TEST("two bars") { EXPECT_EQ(call({2, 4}), 4); }
TEST("single bar") { EXPECT_EQ(call({1}), 1); }
TEST("single zero") { EXPECT_EQ(call({0}), 0); }
TEST("all equal") { EXPECT_EQ(call({3, 3, 3}), 9); }
TEST("increasing") { EXPECT_EQ(call({1, 2, 3, 4, 5}), 9); }
TEST("decreasing") { EXPECT_EQ(call({5, 4, 3, 2, 1}), 9); }
TEST("a zero splits the histogram") { EXPECT_EQ(call({2, 0, 2}), 2); }
TEST("best is neither the tallest nor the widest") { EXPECT_EQ(call({6, 2, 5, 4, 5, 1, 6}), 12); }
// 在這裡加你自己的邊界測資：

// ---- oracle：每個起點往右延伸，邊走邊記最矮的高度（O(n²)） ----
static int oracle(const vector<int>& h) {
    int best = 0;
    for (size_t i = 0; i < h.size(); ++i) {
        int low = INT_MAX;
        for (size_t j = i; j < h.size(); ++j) {
            low = min(low, h[j]);
            best = max(best, low * static_cast<int>(j - i + 1));
        }
    }
    return best;
}

STRESS("random histograms vs oracle", 600) {
    auto h = rng.vec(rng.range(1, size * 3), 0, 6);
    CHECK_SAME(call(h), oracle(h), h);
}

TEST("perf n=100000 equal bars (O(n^2), and divide and conquer on the minimum, are too slow)") {
    vector<int> h(100000, 10000);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(h); });
    EXPECT_UNDER_MS(ms, 100);
    EXPECT_EQ(got, 1000000000);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
