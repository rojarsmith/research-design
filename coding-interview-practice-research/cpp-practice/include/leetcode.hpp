// leetcode.hpp: imitates the LeetCode C++ environment (common headers, using namespace std,
// ListNode, TreeNode), then adds the test harness and builders for the data structures.
// solution.hpp needs no #include of its own, and its contents can be pasted straight into LeetCode.
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "judge.hpp"

using namespace std;

struct ListNode {
    int val;
    ListNode* next;
    ListNode() : val(0), next(nullptr) {}
    ListNode(int x) : val(x), next(nullptr) {}
    ListNode(int x, ListNode* n) : val(x), next(n) {}
};

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode() : val(0), left(nullptr), right(nullptr) {}
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
    TreeNode(int x, TreeNode* l, TreeNode* r) : val(x), left(l), right(r) {}
};

// ---------- builders used by the tests (nodes are deliberately leaked; tests are short-lived) ----------

// Placeholder for a missing node in level-order tree literals, as in LeetCode's [1,null,2].
inline constexpr int null = INT_MIN;

// make_list({3, 2, 0, -4}, 1): the tail links back to index 1. Pass -1 for no cycle.
inline ListNode* make_list(const vector<int>& vals, int cycle_pos = -1) {
    ListNode dummy;
    ListNode* tail = &dummy;
    vector<ListNode*> nodes;
    for (int v : vals) {
        tail->next = new ListNode(v);
        tail = tail->next;
        nodes.push_back(tail);
    }
    if (cycle_pos >= 0 && cycle_pos < static_cast<int>(nodes.size())) tail->next = nodes[static_cast<size_t>(cycle_pos)];
    return dummy.next;
}

// Position of `target` in the list starting at head; -1 if it is nullptr or not reachable.
inline int index_of(ListNode* head, const ListNode* target) {
    if (target == nullptr) return -1;
    int i = 0;
    unordered_set<const ListNode*> seen;
    for (ListNode* p = head; p != nullptr && !seen.count(p); p = p->next, ++i) {
        if (p == target) return i;
        seen.insert(p);
    }
    return -1;
}

inline vector<int> list_values(ListNode* head, int limit = 100000) {
    vector<int> out;
    for (ListNode* p = head; p != nullptr && limit-- > 0; p = p->next) out.push_back(p->val);
    return out;
}

// make_tree({1, null, 2, 3}): LeetCode's level-order format.
inline TreeNode* make_tree(const vector<int>& level) {
    if (level.empty() || level[0] == null) return nullptr;
    auto* root = new TreeNode(level[0]);
    queue<TreeNode*> q;
    q.push(root);
    size_t i = 1;
    while (!q.empty() && i < level.size()) {
        TreeNode* n = q.front();
        q.pop();
        if (i < level.size() && level[i] != null) { n->left = new TreeNode(level[i]); q.push(n->left); }
        ++i;
        if (i < level.size() && level[i] != null) { n->right = new TreeNode(level[i]); q.push(n->right); }
        ++i;
    }
    return root;
}

inline vector<int> tree_level_order(TreeNode* root) {
    vector<int> out;
    queue<TreeNode*> q;
    q.push(root);
    while (!q.empty()) {
        TreeNode* n = q.front();
        q.pop();
        if (n == nullptr) { out.push_back(null); continue; }
        out.push_back(n->val);
        q.push(n->left);
        q.push(n->right);
    }
    while (!out.empty() && out.back() == null) out.pop_back();
    return out;
}

// A random binary tree with n nodes and values in [lo, hi].
inline TreeNode* random_tree(judge::Rng& rng, int n, int lo, int hi) {
    if (n <= 0) return nullptr;
    vector<TreeNode*> nodes{new TreeNode(rng.range(lo, hi))};
    while (static_cast<int>(nodes.size()) < n) {
        TreeNode* parent = nodes[static_cast<size_t>(rng.range(0, static_cast<int>(nodes.size()) - 1))];
        TreeNode*& slot = rng.coin() ? parent->left : parent->right;
        if (slot != nullptr) continue;
        slot = new TreeNode(rng.range(lo, hi));
        nodes.push_back(slot);
    }
    return nodes[0];
}

inline string judge_show(const ListNode* p) {
    if (p == nullptr) return "nullptr";
    return "list" + judge::show(list_values(const_cast<ListNode*>(p), 20));
}

inline string judge_show(const TreeNode* p) {
    if (p == nullptr) return "nullptr";
    string s = "[";
    vector<int> lv = tree_level_order(const_cast<TreeNode*>(p));
    for (size_t i = 0; i < lv.size() && i < 40; ++i) {
        if (i) s += ", ";
        s += lv[i] == null ? "null" : to_string(lv[i]);
    }
    return s + "]";
}

// Sorts each inner vector and then the outer one, so answers given "in any order" compare equal.
inline vector<vector<int>> normalized(vector<vector<int>> v) {
    for (auto& x : v) sort(x.begin(), x.end());
    sort(v.begin(), v.end());
    return v;
}
