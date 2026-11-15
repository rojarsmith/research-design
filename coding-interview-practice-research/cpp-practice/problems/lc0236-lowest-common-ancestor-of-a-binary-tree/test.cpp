// 測試：lc0236-lowest-common-ancestor-of-a-binary-tree。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0236-lowest-common-ancestor-of-a-binary-tree（VS Code 工作：pp: 開始/重做這題）"
#endif

// 節點值互不相同，所以用值找出 p、q，再回傳答案節點的值（找不到時是 INT_MIN）
static TreeNode* find_val(TreeNode* n, int v) {
    if (!n) return nullptr;
    if (n->val == v) return n;
    TreeNode* l = find_val(n->left, v);
    return l ? l : find_val(n->right, v);
}
static int call(TreeNode* root, int p, int q) {
    TreeNode* got = Solution().lowestCommonAncestor(root, find_val(root, p), find_val(root, q));
    return got ? got->val : INT_MIN;
}

static TreeNode* sample() { return make_tree({8, 3, 10, 1, 6, null, 14, null, null, 4, 7, 13}); }

TEST("siblings") { EXPECT_EQ(call(sample(), 4, 7), 6); }
TEST("different depths in one subtree") { EXPECT_EQ(call(sample(), 1, 7), 3); }
TEST("across the root") { EXPECT_EQ(call(sample(), 4, 14), 8); }
TEST("one is an ancestor of the other") { EXPECT_EQ(call(sample(), 13, 10), 10); }
TEST("ancestor given first") { EXPECT_EQ(call(sample(), 6, 4), 6); }
TEST("root is one of them") { EXPECT_EQ(call(sample(), 8, 13), 8); }
TEST("two nodes") { EXPECT_EQ(call(make_tree({1, 2}), 1, 2), 1); }
// 在這裡加你自己的邊界測資：

// ---- oracle：記下每個節點的父節點，把 p 的所有祖先放進集合，再從 q 往上找 ----
static int oracle(TreeNode* root, int p, int q) {
    unordered_map<TreeNode*, TreeNode*> parent{{root, nullptr}};
    vector<TreeNode*> st{root};
    while (!st.empty()) {
        TreeNode* n = st.back();
        st.pop_back();
        for (TreeNode* c : {n->left, n->right})
            if (c) { parent[c] = n; st.push_back(c); }
    }
    unordered_set<TreeNode*> anc;
    for (TreeNode* a = find_val(root, p); a; a = parent[a]) anc.insert(a);
    for (TreeNode* b = find_val(root, q); b; b = parent[b])
        if (anc.count(b)) return b->val;
    return INT_MIN;
}

STRESS("random trees and pairs vs oracle", 500) {
    int n = rng.range(1, size * 2);
    TreeNode* root = random_tree(rng, n, 0, 0);
    int next = 0;  // relabel so every value is unique
    vector<TreeNode*> st{root};
    while (!st.empty()) {
        TreeNode* x = st.back();
        st.pop_back();
        x->val = next++;
        if (x->left) st.push_back(x->left);
        if (x->right) st.push_back(x->right);
    }
    int p = rng.range(0, n - 1), q = rng.range(0, n - 1);
    CHECK_SAME(call(root, p, q), oracle(root, p, q), root, p, q);
}

TEST("perf: chain of 100000 nodes (checking subtrees for every node is too slow)") {
    const int n = 100000;
    auto* root = new TreeNode(0);
    TreeNode* cur = root;
    TreeNode *p = nullptr, *q = nullptr;
    for (int i = 1; i < n; ++i) {
        cur = cur->left = new TreeNode(i);
        if (i == n - 2) q = cur;
        if (i == n - 1) p = cur;
    }
    TreeNode* got = nullptr;
    double ms = judge::time_ms([&] { got = Solution().lowestCommonAncestor(root, p, q); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_TRUE(got == q);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
