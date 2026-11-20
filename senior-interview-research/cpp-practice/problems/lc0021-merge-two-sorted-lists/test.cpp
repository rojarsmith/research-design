// 測試：lc0021-merge-two-sorted-lists。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0021-merge-two-sorted-lists（VS Code 工作：pp: 開始/重做這題）"
#endif

static vector<int> call(const vector<int>& a, const vector<int>& b) {
    return list_values(Solution().mergeTwoLists(make_list(a), make_list(b)));
}

TEST("both empty") { EXPECT_EQ(call({}, {}), {}); }
TEST("one empty") { EXPECT_EQ(call({}, {0}), {0}); }
TEST("interleaved with ties") { EXPECT_EQ(call({1, 2, 4}, {1, 3, 4}), {1, 1, 2, 3, 4, 4}); }
TEST("one list entirely after the other") { EXPECT_EQ(call({5}, {1, 2, 3}), {1, 2, 3, 5}); }
TEST("negatives") { EXPECT_EQ(call({-3, 0}, {-3, -1, 7}), {-3, -3, -1, 0, 7}); }
// 在這裡加你自己的邊界測資：

STRESS("random sorted lists vs std::merge", 500) {
    auto a = rng.vec(rng.range(0, size * 2), -9, 9), b = rng.vec(rng.range(0, size * 2), -9, 9);
    sort(a.begin(), a.end());
    sort(b.begin(), b.end());
    vector<int> expect;
    merge(a.begin(), a.end(), b.begin(), b.end(), back_inserter(expect));
    CHECK_SAME(call(a, b), expect, a, b);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
