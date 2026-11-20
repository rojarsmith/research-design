// 測試：lc0124-binary-tree-maximum-path-sum。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0124-binary-tree-maximum-path-sum（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(TreeNode* root) { return Solution().maxPathSum(root); }

TEST("single positive") { EXPECT_EQ(call(make_tree({5})), 5); }
TEST("single negative") { EXPECT_EQ(call(make_tree({-3})), -3); }
TEST("full small tree") { EXPECT_EQ(call(make_tree({1, 2, 3})), 6); }
TEST("skip negative child") { EXPECT_EQ(call(make_tree({2, -1})), 2); }
TEST("all negative") { EXPECT_EQ(call(make_tree({-2, -1})), -1); }
TEST("bend at root over a negative child") { EXPECT_EQ(call(make_tree({1, -2, 3})), 4); }
TEST("best path does not need the right subtree") {
    EXPECT_EQ(call(make_tree({10, 2, 10, 20, 1, null, -25, null, null, null, null, 3, 4})), 42);
}
TEST("best path below a negative root") { EXPECT_EQ(call(make_tree({-1, 5, null, 4, null, null, 2, -4})), 11); }
// 在這裡加你自己的邊界測資：

// ---- oracle：枚舉所有節點對，沿父節點走到共同祖先求路徑和 ----
static int oracle(TreeNode* root) {
    vector<TreeNode*> nodes;
    unordered_map<TreeNode*, TreeNode*> parent;
    unordered_map<TreeNode*, int> depth;
    vector<TreeNode*> stack{root};
    parent[root] = nullptr;
    depth[root] = 0;
    while (!stack.empty()) {
        TreeNode* n = stack.back();
        stack.pop_back();
        nodes.push_back(n);
        for (TreeNode* c : {n->left, n->right})
            if (c) { parent[c] = n; depth[c] = depth[n] + 1; stack.push_back(c); }
    }
    int best = INT_MIN;
    for (TreeNode* u : nodes)
        for (TreeNode* v : nodes) {
            TreeNode *a = u, *b = v;
            int sum = 0;
            while (a != b) {
                if (depth[a] >= depth[b]) { sum += a->val; a = parent[a]; }
                else { sum += b->val; b = parent[b]; }
            }
            best = max(best, sum + a->val);
        }
    return best;
}

STRESS("random trees vs oracle", 500) {
    TreeNode* root = random_tree(rng, rng.range(1, size * 2), -5, 5);
    CHECK_SAME(call(root), oracle(root), root);
}

TEST("perf: chain of 100000 nodes (deep recursion, O(n^2) is too slow)") {
    const int n = 100000;
    auto* root = new TreeNode(1);
    TreeNode* cur = root;
    for (int i = 1; i < n; ++i) cur = cur->right = new TreeNode(1);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(root); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, n);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
