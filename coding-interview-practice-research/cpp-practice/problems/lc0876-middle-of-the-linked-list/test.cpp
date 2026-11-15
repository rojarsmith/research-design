// 測試：lc0876-middle-of-the-linked-list。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0876-middle-of-the-linked-list（VS Code 工作：pp: 開始/重做這題）"
#endif

// 回傳中間節點在原串列中的位置，方便比對；回傳新建的節點或 nullptr 會得到 -1。
static int call(const vector<int>& vals) {
    ListNode* head = make_list(vals);
    return index_of(head, Solution().middleNode(head));
}

TEST("odd length") { EXPECT_EQ(call({1, 2, 3, 4, 5}), 2); }
TEST("even length returns the second middle") { EXPECT_EQ(call({1, 2, 3, 4, 5, 6}), 3); }
TEST("single node") { EXPECT_EQ(call({1}), 0); }
TEST("two nodes") { EXPECT_EQ(call({1, 2}), 1); }
TEST("three nodes") { EXPECT_EQ(call({1, 2, 3}), 1); }
TEST("list is not modified") {
    vector<int> vals{4, 5, 6, 7};
    ListNode* head = make_list(vals);
    Solution().middleNode(head);
    EXPECT_EQ(list_values(head), vals);
}
// 在這裡加你自己的邊界測資：

// ---- oracle：位置就是 n / 2（從 0 起算） ----
STRESS("random lengths", 300) {
    int n = rng.range(1, size * 3);
    auto vals = rng.vec(n, -5, 5);
    CHECK_SAME(call(vals), n / 2, vals);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
