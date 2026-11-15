// 測試：lc0141-linked-list-cycle。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0141-linked-list-cycle（VS Code 工作：pp: 開始/重做這題）"
#endif

// pos 是尾端接回去的位置，-1 代表沒有環。
static bool call(const vector<int>& vals, int pos) { return Solution().hasCycle(make_list(vals, pos)); }

TEST("empty list") { EXPECT_EQ(call({}, -1), false); }
TEST("single node, no cycle") { EXPECT_EQ(call({1}, -1), false); }
TEST("single node pointing to itself") { EXPECT_EQ(call({1}, 0), true); }
TEST("two nodes, cycle at head") { EXPECT_EQ(call({1, 2}, 0), true); }
TEST("two nodes, no cycle") { EXPECT_EQ(call({1, 2}, -1), false); }
TEST("cycle in the middle") { EXPECT_EQ(call({3, 2, 0, -4}, 1), true); }
TEST("tail points to itself") { EXPECT_EQ(call({1, 2, 3, 4, 5, 6}, 5), true); }
TEST("duplicate values are not a cycle") { EXPECT_EQ(call({7, 7, 7, 7}, -1), false); }
TEST("list is not modified") {
    vector<int> vals{4, 5, 6, 7};
    ListNode* head = make_list(vals, -1);
    Solution().hasCycle(head);
    EXPECT_EQ(list_values(head), vals);
}
// 在這裡加你自己的邊界測資：

STRESS("random lists and cycle positions", 500) {
    int n = rng.range(0, size * 3);
    int pos = n == 0 ? -1 : rng.range(-1, n - 1);
    auto vals = rng.vec(n, -5, 5);
    CHECK_SAME(call(vals, pos), pos >= 0, vals, pos);
}

TEST("perf n=100000 (scanning back for every node is too slow)") {
    vector<int> vals(100000, 0);
    bool with_cycle = false, without = true;
    double ms = judge::time_ms([&] {
        with_cycle = call(vals, 50000);
        without = call(vals, -1);
    });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(with_cycle, true);
    EXPECT_EQ(without, false);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
