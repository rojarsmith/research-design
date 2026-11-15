// 測試：lc0128-longest-consecutive-sequence。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0128-longest-consecutive-sequence（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> nums) { return Solution().longestConsecutive(nums); }

TEST("empty") { EXPECT_EQ(call({}), 0); }
TEST("single") { EXPECT_EQ(call({7}), 1); }
TEST("duplicates inside a run") { EXPECT_EQ(call({1, 2, 0, 1}), 3); }
TEST("two runs, unsorted") { EXPECT_EQ(call({9, 1, 4, 7, 3, -1, 0, 5, 8, -1, 6}), 7); }
TEST("extreme values") { EXPECT_EQ(call({1000000000, -1000000000}), 1); }
TEST("all same") { EXPECT_EQ(call({5, 5, 5}), 1); }
TEST("scattered") { EXPECT_EQ(call({10, 5, 12, 3, 55, 30, 4, 11, 2}), 4); }
// 在這裡加你自己的邊界測資：

// ---- oracle：排序後掃描（O(n log n)；題目要求 O(n)） ----
static int oracle(vector<int> a) {
    sort(a.begin(), a.end());
    a.erase(unique(a.begin(), a.end()), a.end());
    int best = 0, run = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        run = (i > 0 && a[i] == a[i - 1] + 1) ? run + 1 : 1;
        best = max(best, run);
    }
    return best;
}

STRESS("random vs oracle", 600) {
    auto a = rng.vec(rng.range(0, size * 2), -6, 6);
    CHECK_SAME(call(a), oracle(a), a);
}

TEST("perf n=100000 shuffled range (O(n^2) is too slow)") {
    const int n = 100000;
    vector<int> a(n);
    iota(a.begin(), a.end(), 0);
    shuffle(a.begin(), a.end(), mt19937(7));
    int got = 0;
    double ms = judge::time_ms([&] { got = call(a); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, n);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
