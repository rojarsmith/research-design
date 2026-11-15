// 測試：lc0198-house-robber。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0198-house-robber（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> nums) { return Solution().rob(nums); }

TEST("one house") { EXPECT_EQ(call({5}), 5); }
TEST("two houses, first richer") { EXPECT_EQ(call({2, 1}), 2); }
TEST("two houses, second richer") { EXPECT_EQ(call({1, 2}), 2); }
TEST("skip one then take") { EXPECT_EQ(call({2, 7, 9, 3, 1}), 12); }
TEST("best picks are not every other house") { EXPECT_EQ(call({2, 1, 1, 2}), 4); }
TEST("all zero") { EXPECT_EQ(call({0, 0, 0}), 0); }
TEST("rich ends") { EXPECT_EQ(call({400, 0, 0, 400}), 800); }
// 在這裡加你自己的邊界測資：

// ---- oracle：列舉所有不相鄰的選法 ----
static int oracle(const vector<int>& a) {
    int best = 0;
    for (unsigned mask = 0; mask < (1u << a.size()); ++mask) {
        if (mask & (mask >> 1)) continue;
        int sum = 0;
        for (size_t i = 0; i < a.size(); ++i)
            if (mask >> i & 1u) sum += a[i];
        best = max(best, sum);
    }
    return best;
}

STRESS("random streets vs subset enumeration", 500) {
    auto a = rng.vec(rng.range(1, size + size / 2), 0, 9);
    CHECK_SAME(call(a), oracle(a), a);
}

TEST("perf n=100000 (recursion without memoization never finishes)") {
    vector<int> a(100000, 1);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(a); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, 50000);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
