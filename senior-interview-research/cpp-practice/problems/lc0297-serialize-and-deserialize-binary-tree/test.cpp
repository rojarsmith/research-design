// 測試：lc0297-serialize-and-deserialize-binary-tree。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0297-serialize-and-deserialize-binary-tree（VS Code 工作：pp: 開始/重做這題）"
#endif

// 格式由你決定；測試只檢查「序列化再還原」之後，樹的結構和值都沒變。
static vector<int> round_trip(TreeNode* root) {
    Codec c;
    return tree_level_order(c.deserialize(c.serialize(root)));
}

TEST("empty tree") { EXPECT_EQ(round_trip(nullptr), {}); }
TEST("single node") { EXPECT_EQ(round_trip(make_tree({1})), {1}); }
TEST("missing children") { EXPECT_EQ(round_trip(make_tree({1, 2, 3, null, null, 4, 5})), {1, 2, 3, null, null, 4, 5}); }
TEST("negative and multi-digit values") { EXPECT_EQ(round_trip(make_tree({-1000, null, 1000, -7})), {-1000, null, 1000, -7}); }
TEST("left chain") { EXPECT_EQ(round_trip(make_tree({1, 2, null, 3, null, 4})), {1, 2, null, 3, null, 4}); }
TEST("separate Codec objects") {
    TreeNode* t = make_tree({5, 3, 8, null, 4});
    string s = Codec().serialize(t);
    EXPECT_EQ(tree_level_order(Codec().deserialize(s)), {5, 3, 8, null, 4});
}
// 在這裡加你自己的邊界測資：

STRESS("random trees survive a round trip", 500) {
    TreeNode* root = random_tree(rng, rng.range(0, size * 2), -1000, 1000);
    CHECK_SAME(round_trip(root), tree_level_order(root), root);
}

TEST("perf: chain of 100000 nodes (s = s + x copies the string every time)") {
    const int n = 100000;
    auto* root = new TreeNode(-123456);
    TreeNode* cur = root;
    for (int i = 1; i < n; ++i) cur = cur->right = new TreeNode(i);
    vector<int> got;
    double ms = judge::time_ms([&] { got = round_trip(root); });
    EXPECT_UNDER_MS(ms, 500);
    EXPECT_EQ(got, tree_level_order(root));
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
