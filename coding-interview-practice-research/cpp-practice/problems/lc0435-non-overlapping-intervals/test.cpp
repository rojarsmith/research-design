// 測試：lc0435-non-overlapping-intervals。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0435-non-overlapping-intervals（VS Code 工作：pp: 開始/重做這題）"
#endif

// [1, 2] 和 [2, 3] 只是相接，不算重疊。
static int call(vector<vector<int>> intervals) { return Solution().eraseOverlapIntervals(intervals); }

TEST("example") { EXPECT_EQ(call({{1, 2}, {2, 3}, {3, 4}, {1, 3}}), 1); }
TEST("identical intervals") { EXPECT_EQ(call({{1, 2}, {1, 2}, {1, 2}}), 2); }
TEST("touching intervals do not overlap") { EXPECT_EQ(call({{1, 2}, {2, 3}}), 0); }
TEST("single interval") { EXPECT_EQ(call({{0, 1}}), 0); }
TEST("remove the long one, not the two short ones") { EXPECT_EQ(call({{1, 10}, {2, 3}, {4, 5}}), 1); }
TEST("negative coordinates") { EXPECT_EQ(call({{-5, -1}, {-3, 2}, {1, 4}}), 1); }
TEST("same end, different starts") { EXPECT_EQ(call({{1, 3}, {2, 3}, {0, 3}}), 2); }
TEST("unsorted input") { EXPECT_EQ(call({{5, 7}, {1, 3}, {2, 6}, {6, 8}}), 2); }
// 在這裡加你自己的邊界測資：

// ---- oracle：試每一個子集，找出兩兩不重疊的最大子集（O(2ⁿ · n²)） ----
static int oracle(const vector<vector<int>>& iv) {
    int n = static_cast<int>(iv.size()), best = 0;
    for (int mask = 0; mask < (1 << n); ++mask) {
        bool ok = true;
        for (int i = 0; i < n && ok; ++i)
            for (int j = i + 1; j < n && ok; ++j) {
                if (!((mask >> i) & 1) || !((mask >> j) & 1)) continue;
                const auto &a = iv[static_cast<size_t>(i)], &b = iv[static_cast<size_t>(j)];
                if (a[0] < b[1] && b[0] < a[1]) ok = false;
            }
        if (ok) best = max(best, popcount(static_cast<unsigned>(mask)));
    }
    return n - best;
}

STRESS("random intervals vs trying every subset", 500) {
    vector<vector<int>> iv;
    for (int i = rng.range(1, size); i > 0; --i) {
        int start = rng.range(-5, 5);
        iv.push_back({start, start + rng.range(1, 4)});
    }
    CHECK_SAME(call(iv), oracle(iv), iv);
}

// [i, i + 2] 兩兩相鄰的會重疊，每隔一個留一個剛好相接：留下 50000 個，刪掉 50000 個。
TEST("perf 100000 overlapping intervals in random order (O(n^2) DP is too slow)") {
    const int n = 100000;
    vector<vector<int>> iv;
    for (int i = 0; i < n; ++i) iv.push_back({i - 50000, i - 50000 + 2});
    judge::Rng r(435);
    r.shuffle(iv);
    int got = -1;
    double ms = judge::time_ms([&] { got = call(iv); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, n / 2);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
