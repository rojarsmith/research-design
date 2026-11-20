// 測試：lc0142-linked-list-cycle-ii。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0142-linked-list-cycle-ii（VS Code 工作：pp: 開始/重做這題）"
#endif

// 回傳環入口在串列中的位置（沒有環是 -1），方便比對
static int call(const vector<int>& vals, int pos) {
    ListNode* head = make_list(vals, pos);
    return index_of(head, Solution().detectCycle(head));
}

TEST("empty list") { EXPECT_EQ(call({}, -1), -1); }
TEST("single node, no cycle") { EXPECT_EQ(call({1}, -1), -1); }
TEST("single node pointing to itself") { EXPECT_EQ(call({1}, 0), 0); }
TEST("two nodes, cycle at head") { EXPECT_EQ(call({1, 2}, 0), 0); }
TEST("two nodes, no cycle") { EXPECT_EQ(call({1, 2}, -1), -1); }
TEST("cycle in the middle") { EXPECT_EQ(call({3, 2, 0, -4}, 1), 1); }
TEST("tail points to itself") { EXPECT_EQ(call({1, 2, 3, 4, 5, 6}, 5), 5); }
TEST("long tail before the cycle") { EXPECT_EQ(call({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, 3), 3); }
TEST("list is not modified") {
    vector<int> vals{4, 5, 6, 7};
    ListNode* head = make_list(vals, -1);
    Solution().detectCycle(head);
    EXPECT_EQ(list_values(head), vals);
}
// 在這裡加你自己的邊界測資：

STRESS("random lists and cycle positions", 500) {
    int n = rng.range(0, size * 3);
    int pos = n == 0 ? -1 : rng.range(-1, n - 1);
    auto vals = rng.vec(n, -5, 5);
    CHECK_SAME(call(vals, pos), pos, vals, pos);
}

TEST("perf n=100000 (scanning back for every node is too slow)") {
    vector<int> vals(100000, 0);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(vals, 50000); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, 50000);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
