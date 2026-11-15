// 測試：lc0338-counting-bits。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0338-counting-bits（VS Code 工作：pp: 開始/重做這題）"
#endif

static vector<int> call(int n) { return Solution().countBits(n); }

TEST("n = 0") { EXPECT_EQ(call(0), {0}); }
TEST("n = 1") { EXPECT_EQ(call(1), {0, 1}); }
TEST("n = 5") { EXPECT_EQ(call(5), {0, 1, 1, 2, 1, 2}); }
TEST("n = 8 crosses a power of two") { EXPECT_EQ(call(8), {0, 1, 1, 2, 1, 2, 2, 3, 1}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：每個數逐位數 1 ----
static vector<int> oracle(int n) {
    vector<int> out;
    for (int i = 0; i <= n; ++i) {
        int c = 0;
        for (int x = i; x; x >>= 1) c += x & 1;
        out.push_back(c);
    }
    return out;
}

STRESS("random n vs oracle", 300) {
    int n = rng.range(0, size * 30);
    CHECK_SAME(call(n), oracle(n), n);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
