// 測試：lc0191-number-of-1-bits。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0191-number-of-1-bits（VS Code 工作：pp: 開始/重做這題）"
#endif

// 題目範圍是 1 <= n <= 2^31 - 1。
static int call(int n) { return Solution().hammingWeight(n); }

TEST("11 = 1011") { EXPECT_EQ(call(11), 3); }
TEST("128 = 10000000") { EXPECT_EQ(call(128), 1); }
TEST("2147483645") { EXPECT_EQ(call(2147483645), 30); }
TEST("one") { EXPECT_EQ(call(1), 1); }
TEST("INT_MAX has 31 ones") { EXPECT_EQ(call(INT_MAX), 31); }
TEST("highest power of two") { EXPECT_EQ(call(1 << 30), 1); }
TEST("every number below 2^20 (each bit is set in exactly half of them)") {
    long long total = 0;
    for (int i = 1; i < (1 << 20); ++i) total += call(i);
    EXPECT_EQ(total, 20LL << 19);
}
// 在這裡加你自己的邊界測資：

// ---- oracle：一位一位檢查 ----
static int oracle(int n) {
    int c = 0;
    for (int b = 0; b < 31; ++b) c += (n >> b) & 1;
    return c;
}

STRESS("random positive ints vs bit-by-bit count", 500) {
    int n = rng.range(1, INT_MAX);
    CHECK_SAME(call(n), oracle(n), n);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
