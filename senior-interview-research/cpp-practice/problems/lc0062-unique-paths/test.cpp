// 測試：lc0062-unique-paths。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0062-unique-paths（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(int m, int n) { return Solution().uniquePaths(m, n); }

TEST("1x1") { EXPECT_EQ(call(1, 1), 1); }
TEST("single row") { EXPECT_EQ(call(1, 5), 1); }
TEST("2x2") { EXPECT_EQ(call(2, 2), 2); }
TEST("3x3") { EXPECT_EQ(call(3, 3), 6); }
TEST("3x7") { EXPECT_EQ(call(3, 7), 28); }
TEST("10x10") { EXPECT_EQ(call(10, 10), 48620); }
TEST("answer near INT_MAX") { EXPECT_EQ(call(51, 9), 1916797311); }
// 在這裡加你自己的邊界測資：

// ---- oracle：不記憶的遞迴，每一格都從上面或左邊來（指數時間） ----
static long long oracle(int m, int n) {
    if (m == 1 || n == 1) return 1;
    return oracle(m - 1, n) + oracle(m, n - 1);
}

STRESS("random sizes vs plain recursion", 300) {
    int m = rng.range(1, size), n = rng.range(1, size);
    CHECK_SAME(static_cast<long long>(call(m, n)), oracle(m, n), m, n);
}

TEST("perf 51x9 (plain recursion would visit about 2*10^9 paths)") {
    int got = 0;
    double ms = judge::time_ms([&] { got = call(51, 9); });
    EXPECT_UNDER_MS(ms, 100);
    EXPECT_EQ(got, 1916797311);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
