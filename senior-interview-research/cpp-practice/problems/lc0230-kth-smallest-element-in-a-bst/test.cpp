// 測試：lc0230-kth-smallest-element-in-a-bst。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0230-kth-smallest-element-in-a-bst（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(TreeNode* root, int k) { return Solution().kthSmallest(root, k); }

TEST("example") { EXPECT_EQ(call(make_tree({3, 1, 4, null, 2}), 1), 1); }
TEST("example 2") { EXPECT_EQ(call(make_tree({5, 3, 6, 2, 4, null, null, 1}), 3), 3); }
TEST("k = n is the largest") { EXPECT_EQ(call(make_tree({5, 3, 6, 2, 4, null, null, 1}), 6), 6); }
TEST("single node") { EXPECT_EQ(call(make_tree({7}), 1), 7); }
TEST("root is the answer") { EXPECT_EQ(call(make_tree({2, 1, 3}), 2), 2); }
TEST("left chain") { EXPECT_EQ(call(make_tree({3, 2, null, 1}), 2), 2); }
TEST("right chain") { EXPECT_EQ(call(make_tree({1, null, 2, null, 3}), 3), 3); }
TEST("value 0") { EXPECT_EQ(call(make_tree({1, 0}), 1), 0); }
// 在這裡加你自己的邊界測資：

// ---- oracle：收集所有值，排序後取第 k 個 ----
static void collect(TreeNode* n, vector<int>& out) {
    if (!n) return;
    out.push_back(n->val);
    collect(n->left, out);
    collect(n->right, out);
}

static int oracle(TreeNode* root, int k) {
    vector<int> v;
    collect(root, v);
    sort(v.begin(), v.end());
    return v[static_cast<size_t>(k - 1)];
}

static TreeNode* insert_bst(TreeNode* root, int v) {
    if (!root) return new TreeNode(v);
    if (v < root->val) root->left = insert_bst(root->left, v);
    else root->right = insert_bst(root->right, v);
    return root;
}

STRESS("random BSTs vs sorting all values", 500) {
    TreeNode* root = nullptr;
    auto vals = rng.distinct(rng.range(1, size * 2), 0, 40);
    for (int v : vals) root = insert_bst(root, v);
    int k = rng.range(1, static_cast<int>(vals.size()));
    CHECK_SAME(call(root, k), oracle(root, k), root, k);
}

TEST("perf left chain of 100000 nodes (recounting subtree sizes at every step is too slow)") {
    const int n = 100000;
    auto* root = new TreeNode(n - 1);
    TreeNode* cur = root;
    for (int v = n - 2; v >= 0; --v) cur = cur->left = new TreeNode(v);
    int got = -1;
    double ms = judge::time_ms([&] { got = call(root, n / 2); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, n / 2 - 1);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
