// 測試：lc0322-coin-change。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0322-coin-change（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> coins, int amount) { return Solution().coinChange(coins, amount); }

TEST("amount zero") { EXPECT_EQ(call({1}, 0), 0); }
TEST("impossible") { EXPECT_EQ(call({2}, 3), -1); }
TEST("only ones") { EXPECT_EQ(call({1}, 7), 7); }
TEST("largest coin first is wrong") { EXPECT_EQ(call({1, 3, 4}, 6), 2); }
TEST("two coins") { EXPECT_EQ(call({5, 10}, 15), 2); }
TEST("unsorted coins") { EXPECT_EQ(call({2, 5, 10, 1}, 27), 4); }
TEST("no combination") { EXPECT_EQ(call({3, 7}, 5), -1); }
TEST("coin of INT_MAX") { EXPECT_EQ(call({1, 2147483647}, 2), 2); }
// 在這裡加你自己的邊界測資：

// ---- oracle：枚舉每種硬幣用幾枚（指數時間，只適合小金額） ----
static int oracle_rec(const vector<int>& c, size_t i, int remain) {
    if (remain == 0) return 0;
    if (i == c.size()) return INT_MAX;
    int best = INT_MAX;
    for (long long cnt = 0; cnt * c[i] <= remain; ++cnt) {
        int sub = oracle_rec(c, i + 1, remain - static_cast<int>(cnt * c[i]));
        if (sub != INT_MAX) best = min(best, sub + static_cast<int>(cnt));
    }
    return best;
}
static int oracle(const vector<int>& c, int amount) {
    int r = oracle_rec(c, 0, amount);
    return r == INT_MAX ? -1 : r;
}

STRESS("random coins vs oracle", 500) {
    auto coins = rng.distinct(rng.range(1, 4), 1, 12);
    int amount = rng.range(0, size * 3);
    CHECK_SAME(call(coins, amount), oracle(coins, amount), coins, amount);
}

TEST("perf amount 10000 (recursion without memoization never finishes)") {
    int got = 0;
    double ms = judge::time_ms([&] { got = call({3, 7, 11, 13}, 10000); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, 770);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
