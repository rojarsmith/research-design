// 測試：lc0033-search-in-rotated-sorted-array。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0033-search-in-rotated-sorted-array（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> nums, int target) { return Solution().search(nums, target); }

TEST("single, found") { EXPECT_EQ(call({1}, 1), 0); }
TEST("single, missing") { EXPECT_EQ(call({1}, 0), -1); }
TEST("target in the right part") { EXPECT_EQ(call({5, 6, 7, 1, 2, 3, 4}, 2), 4); }
TEST("target in the left part") { EXPECT_EQ(call({5, 6, 7, 1, 2, 3, 4}, 6), 1); }
TEST("two elements, rotated") { EXPECT_EQ(call({3, 1}, 1), 1); }
TEST("two elements, first") { EXPECT_EQ(call({3, 1}, 3), 0); }
TEST("not rotated") { EXPECT_EQ(call({1, 3}, 3), 1); }
TEST("missing, larger than all") { EXPECT_EQ(call({6, 7, 1, 2, 3, 4, 5}, 8), -1); }
// 在這裡加你自己的邊界測資：

// ---- oracle：從頭找到尾 ----
static int oracle(const vector<int>& a, int t) {
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i] == t) return static_cast<int>(i);
    return -1;
}

STRESS("random rotations vs linear scan", 600) {
    auto a = rng.distinct(rng.range(1, size * 2), -20, 20);
    sort(a.begin(), a.end());
    rotate(a.begin(), a.begin() + rng.range(0, static_cast<int>(a.size()) - 1), a.end());
    int t = rng.coin() ? a[static_cast<size_t>(rng.range(0, static_cast<int>(a.size()) - 1))] : rng.range(-21, 21);
    CHECK_SAME(call(a, t), oracle(a, t), a, t);
}

TEST("perf 100000 searches in n=100000 (a linear scan per search is too slow)") {
    const int n = 100000;
    vector<int> a(n);
    for (int i = 0; i < n; ++i) a[static_cast<size_t>(i)] = (i + 37000) % n;
    long long hits = 0;
    double ms = judge::time_ms([&] {
        Solution s;
        for (int t = 0; t < n; ++t) hits += s.search(a, t) >= 0;
    });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(hits, 1LL * n);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
