// 測試：lc0875-koko-eating-bananas。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0875-koko-eating-bananas（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> piles, int h) { return Solution().minEatingSpeed(piles, h); }

TEST("one pile, one hour") { EXPECT_EQ(call({1}, 1), 1); }
TEST("one pile, three hours") { EXPECT_EQ(call({10}, 3), 4); }
TEST("hours equal piles") { EXPECT_EQ(call({5, 5, 5}, 3), 5); }
TEST("plenty of time") { EXPECT_EQ(call({5, 5, 5}, 15), 1); }
TEST("huge pile") { EXPECT_EQ(call({1000000000}, 2), 500000000); }
TEST("huge piles, hour sum passes int at small k") { EXPECT_EQ(call({1000000000, 1000000000, 1000000000, 1000000000}, 1000000000), 4); }
TEST("uneven piles") { EXPECT_EQ(call({2, 7, 9}, 6), 4); }
// 在這裡加你自己的邊界測資：

// ---- oracle：速度從 1 開始一個一個試 ----
static int oracle(const vector<int>& piles, int h) {
    for (int k = 1;; ++k) {
        long long hours = 0;
        for (int p : piles) hours += (p + k - 1) / k;
        if (hours <= h) return k;
    }
}

STRESS("random vs oracle", 500) {
    int n = rng.range(1, size);
    auto piles = rng.vec(n, 1, 20);
    int h = rng.range(n, n + size * 4);
    CHECK_SAME(call(piles, h), oracle(piles, h), piles, h);
}

TEST("perf n=100000 piles of 1e9 (trying every k is too slow)") {
    vector<int> piles(100000, 1000000000);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(piles, 1000000000); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, 100000);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
