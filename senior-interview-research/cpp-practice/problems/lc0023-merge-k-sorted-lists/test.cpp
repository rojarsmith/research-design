// 測試：lc0023-merge-k-sorted-lists。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0023-merge-k-sorted-lists（VS Code 工作：pp: 開始/重做這題）"
#endif

static vector<int> call(const vector<vector<int>>& in) {
    vector<ListNode*> lists;
    for (auto& v : in) lists.push_back(make_list(v));
    return list_values(Solution().mergeKLists(lists));
}

TEST("no lists") { EXPECT_EQ(call({}), {}); }
TEST("one empty list") { EXPECT_EQ(call({{}}), {}); }
TEST("one list plus an empty one") { EXPECT_EQ(call({{1}, {}}), {1}); }
TEST("three lists") { EXPECT_EQ(call({{1, 4, 5}, {1, 3, 4}, {2, 6}}), {1, 1, 2, 3, 4, 4, 5, 6}); }
TEST("lists in reverse order") { EXPECT_EQ(call({{5}, {4}, {3}, {2}, {1}}), {1, 2, 3, 4, 5}); }
TEST("all equal") { EXPECT_EQ(call({{-1, -1}, {-1}}), {-1, -1, -1}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：全部倒出來排序 ----
STRESS("random lists vs flatten and sort", 500) {
    vector<vector<int>> in(static_cast<size_t>(rng.range(0, size)));
    vector<int> all;
    for (auto& v : in) {
        v = rng.vec(rng.range(0, size), -9, 9);
        sort(v.begin(), v.end());
        all.insert(all.end(), v.begin(), v.end());
    }
    sort(all.begin(), all.end());
    CHECK_SAME(call(in), all, in);
}

TEST("perf 10000 lists of 10 (merging one list at a time is too slow)") {
    vector<ListNode*> lists;
    for (int i = 0; i < 10000; ++i) {
        vector<int> v;
        for (int j = 0; j < 10; ++j) v.push_back(j * 10000 + i);
        lists.push_back(make_list(v));
    }
    ListNode* got = nullptr;
    double ms = judge::time_ms([&] { got = Solution().mergeKLists(lists); });
    EXPECT_UNDER_MS(ms, 300);
    auto vals = list_values(got, 200000);
    EXPECT_EQ(vals.size(), size_t{100000});
    EXPECT_TRUE(is_sorted(vals.begin(), vals.end()));
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
