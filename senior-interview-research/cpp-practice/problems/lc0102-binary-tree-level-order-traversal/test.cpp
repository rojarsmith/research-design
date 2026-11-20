// 測試：lc0102-binary-tree-level-order-traversal。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0102-binary-tree-level-order-traversal（VS Code 工作：pp: 開始/重做這題）"
#endif

static vector<vector<int>> call(TreeNode* root) { return Solution().levelOrder(root); }

TEST("empty tree") { EXPECT_EQ(call(nullptr), {}); }
TEST("single node") { EXPECT_EQ(call(make_tree({1})), {{1}}); }
TEST("missing children on the left") { EXPECT_EQ(call(make_tree({3, 9, 20, null, null, 15, 7})), {{3}, {9, 20}, {15, 7}}); }
TEST("left chain") { EXPECT_EQ(call(make_tree({1, 2, null, 3, null, 4})), {{1}, {2}, {3}, {4}}); }
TEST("full tree") { EXPECT_EQ(call(make_tree({1, 2, 3, 4, 5, 6, 7})), {{1}, {2, 3}, {4, 5, 6, 7}}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：DFS 帶著深度，把值放進對應的那一層 ----
static void oracle_dfs(TreeNode* n, size_t depth, vector<vector<int>>& out) {
    if (!n) return;
    if (out.size() == depth) out.push_back({});
    out[depth].push_back(n->val);
    oracle_dfs(n->left, depth + 1, out);
    oracle_dfs(n->right, depth + 1, out);
}
static vector<vector<int>> oracle(TreeNode* root) {
    vector<vector<int>> out;
    oracle_dfs(root, 0, out);
    return out;
}

STRESS("random trees vs DFS", 500) {
    TreeNode* root = random_tree(rng, rng.range(0, size * 2), -9, 9);
    CHECK_SAME(call(root), oracle(root), root);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
