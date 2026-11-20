// 測試：lc0098-validate-binary-search-tree。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0098-validate-binary-search-tree（VS Code 工作：pp: 開始/重做這題）"
#endif

static bool call(TreeNode* root) { return Solution().isValidBST(root); }

TEST("small valid") { EXPECT_EQ(call(make_tree({2, 1, 3})), true); }
TEST("right child too small") { EXPECT_EQ(call(make_tree({5, 1, 4, null, null, 3, 6})), false); }
TEST("grandchild breaks an ancestor's bound") { EXPECT_EQ(call(make_tree({5, 4, 6, null, null, 3, 7})), false); }
TEST("another deep violation") { EXPECT_EQ(call(make_tree({10, 5, 15, null, null, 6, 20})), false); }
TEST("duplicate value") { EXPECT_EQ(call(make_tree({1, 1})), false); }
TEST("single INT_MAX") { EXPECT_EQ(call(make_tree({INT_MAX})), true); }
TEST("INT_MIN root, INT_MAX right child") {
    auto* root = new TreeNode(INT_MIN);
    root->right = new TreeNode(INT_MAX);
    EXPECT_EQ(call(root), true);
}
// 在這裡加你自己的邊界測資：

// ---- oracle：每個節點都和整棵左子樹、整棵右子樹的所有值比較（O(n²)） ----
static void collect(TreeNode* n, vector<int>& out) {
    if (!n) return;
    out.push_back(n->val);
    collect(n->left, out);
    collect(n->right, out);
}
static bool oracle(TreeNode* n) {
    if (!n) return true;
    vector<int> l, r;
    collect(n->left, l);
    collect(n->right, r);
    for (int x : l) if (x >= n->val) return false;
    for (int x : r) if (x <= n->val) return false;
    return oracle(n->left) && oracle(n->right);
}

static TreeNode* insert_bst(TreeNode* root, int v) {
    if (!root) return new TreeNode(v);
    if (v < root->val) root->left = insert_bst(root->left, v);
    else root->right = insert_bst(root->right, v);
    return root;
}

STRESS("random BSTs, sometimes broken, vs oracle", 600) {
    TreeNode* root = nullptr;
    vector<TreeNode*> nodes;
    for (int v : rng.distinct(rng.range(1, size * 2), -20, 20)) root = insert_bst(root, v);
    vector<TreeNode*> st{root};
    while (!st.empty()) {
        TreeNode* n = st.back();
        st.pop_back();
        nodes.push_back(n);
        if (n->left) st.push_back(n->left);
        if (n->right) st.push_back(n->right);
    }
    if (rng.coin()) nodes[static_cast<size_t>(rng.range(0, static_cast<int>(nodes.size()) - 1))]->val = rng.range(-21, 21);
    CHECK_SAME(call(root), oracle(root), root);
}

TEST("perf: increasing chain of 100000 nodes (O(n^2) checks are too slow)") {
    const int n = 100000;
    auto* root = new TreeNode(0);
    TreeNode* cur = root;
    for (int i = 1; i < n; ++i) cur = cur->right = new TreeNode(i);
    bool got = false;
    double ms = judge::time_ms([&] { got = call(root); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, true);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
