// 測試：lc0055-jump-game。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0055-jump-game（VS Code 工作：pp: 開始/重做這題）"
#endif

static bool call(vector<int> nums) { return Solution().canJump(nums); }

TEST("example") { EXPECT_EQ(call({2, 3, 1, 1, 4}), true); }
TEST("stuck on a zero") { EXPECT_EQ(call({3, 2, 1, 0, 4}), false); }
TEST("single element") { EXPECT_EQ(call({0}), true); }
TEST("zero at the start") { EXPECT_EQ(call({0, 1}), false); }
TEST("zero at the end is fine") { EXPECT_EQ(call({1, 0}), true); }
TEST("jump over zeros") { EXPECT_EQ(call({2, 0, 0}), true); }
TEST("one long jump") { EXPECT_EQ(call({5, 0, 0, 0, 0}), true); }
TEST("falls one short") { EXPECT_EQ(call({1, 1, 0, 1}), false); }
// 在這裡加你自己的邊界測資：

// ---- oracle：從每個到得了的位置，把它跳得到的位置全部標記起來（O(n · 最大步數)） ----
static bool oracle(const vector<int>& a) {
    vector<char> reach(a.size(), 0);
    reach[0] = 1;
    for (size_t i = 0; i < a.size(); ++i)
        if (reach[i])
            for (size_t j = i + 1; j <= i + static_cast<size_t>(a[i]) && j < a.size(); ++j) reach[j] = 1;
    return reach.back();
}

STRESS("random arrays vs oracle", 600) {
    auto nums = rng.vec(rng.range(1, size * 3), 0, 3);
    CHECK_SAME(call(nums), oracle(nums), nums);
}

// 每一格都剛好跳得到倒數第二格，而倒數第二格是 0：答案是 false，而且沒辦法提早結束。
TEST("perf n=50000, every jump lands on the last zero (above LeetCode's 10^4; marking every reachable index is O(n^2))") {
    const int n = 50000;
    vector<int> nums(n, 0);
    for (int i = 0; i < n - 1; ++i) nums[static_cast<size_t>(i)] = n - 2 - i;
    bool got = true;
    double ms = judge::time_ms([&] { got = call(nums); });
    EXPECT_UNDER_MS(ms, 50);
    EXPECT_EQ(got, false);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
