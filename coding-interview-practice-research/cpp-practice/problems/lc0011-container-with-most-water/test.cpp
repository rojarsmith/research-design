// 測試：lc0011-container-with-most-water。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0011-container-with-most-water（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> h) { return Solution().maxArea(h); }

TEST("two lines") { EXPECT_EQ(call({1, 1}), 1); }
TEST("unequal pair") { EXPECT_EQ(call({2, 3}), 2); }
TEST("tall ends") { EXPECT_EQ(call({5, 1, 1, 5}), 15); }
TEST("best pair is inside") { EXPECT_EQ(call({1, 2, 4, 3}), 4); }
TEST("all zero") { EXPECT_EQ(call({0, 0, 0}), 0); }
// 在這裡加你自己的邊界測資：

// ---- oracle：O(n²) 枚舉所有配對 ----
static int oracle(const vector<int>& h) {
    int best = 0;
    for (size_t i = 0; i < h.size(); ++i)
        for (size_t j = i + 1; j < h.size(); ++j) best = max(best, min(h[i], h[j]) * static_cast<int>(j - i));
    return best;
}

STRESS("random vs oracle", 500) {
    auto h = rng.vec(rng.range(2, 2 + size * 2), 0, 10);
    CHECK_SAME(call(h), oracle(h), h);
}

TEST("perf n=100000 (O(n^2) is too slow)") {
    const int n = 100000;
    vector<int> h(n, 1);
    h[10] = h[n - 11] = 10000;
    int got = 0;
    double ms = judge::time_ms([&] { got = call(h); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, 10000 * (n - 21));
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
