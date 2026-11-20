// 測試：lc0206-reverse-linked-list。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0206-reverse-linked-list（VS Code 工作：pp: 開始/重做這題）"
#endif

static vector<int> call(const vector<int>& vals) { return list_values(Solution().reverseList(make_list(vals))); }

TEST("empty") { EXPECT_EQ(call({}), {}); }
TEST("single") { EXPECT_EQ(call({1}), {1}); }
TEST("two") { EXPECT_EQ(call({1, 2}), {2, 1}); }
TEST("five") { EXPECT_EQ(call({1, 2, 3, 4, 5}), {5, 4, 3, 2, 1}); }
TEST("duplicates") { EXPECT_EQ(call({7, 7, 3}), {3, 7, 7}); }
TEST("reuses the original nodes") {
    ListNode* head = make_list({1, 2, 3, 4});
    vector<ListNode*> before;
    for (ListNode* p = head; p; p = p->next) before.push_back(p);
    vector<ListNode*> after;
    for (ListNode* p = Solution().reverseList(head); p; p = p->next) after.push_back(p);
    reverse(before.begin(), before.end());
    EXPECT_TRUE(after == before);
}
// 在這裡加你自己的邊界測資：

STRESS("random lists vs std::reverse", 500) {
    auto v = rng.vec(rng.range(0, size * 3), -9, 9);
    auto expect = v;
    reverse(expect.begin(), expect.end());
    CHECK_SAME(call(v), expect, v);
}

TEST("perf n=100000 (walking to the tail each time is too slow)") {
    vector<int> v(100000);
    iota(v.begin(), v.end(), 0);
    ListNode* head = make_list(v);
    ListNode* got = nullptr;
    double ms = judge::time_ms([&] { got = Solution().reverseList(head); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got ? got->val : -1, 99999);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
