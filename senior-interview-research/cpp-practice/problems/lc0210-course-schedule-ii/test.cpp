// 測試：lc0210-course-schedule-ii。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0210-course-schedule-ii（VS Code 工作：pp: 開始/重做這題）"
#endif

// 合法順序不唯一，所以用檢查器驗證，而不是比對固定答案。
static vector<int> call(int n, vector<vector<int>> pre) { return Solution().findOrder(n, pre); }

// ---- oracle：Floyd–Warshall 可達性判斷有沒有環（O(n³)，刻意寫慢） ----
static bool has_cycle(int n, const vector<vector<int>>& pre) {
    vector<vector<char>> r(static_cast<size_t>(n), vector<char>(static_cast<size_t>(n), 0));
    for (auto& e : pre) r[static_cast<size_t>(e[1])][static_cast<size_t>(e[0])] = 1;
    for (size_t k = 0; k < r.size(); ++k)
        for (size_t i = 0; i < r.size(); ++i)
            for (size_t j = 0; j < r.size(); ++j)
                if (r[i][k] && r[k][j]) r[i][j] = 1;
    for (size_t i = 0; i < r.size(); ++i)
        if (r[i][i]) return true;
    return false;
}

static string check(int n, const vector<vector<int>>& pre, const vector<int>& order, bool cyclic) {
    if (cyclic) return order.empty() ? "" : "has a cycle, so expected [], got " + judge::show(order);
    if (static_cast<int>(order.size()) != n) return "expected an order of all " + to_string(n) + " courses, got " + judge::show(order);
    vector<int> pos(static_cast<size_t>(n), -1);
    for (int i = 0; i < n; ++i) {
        int c = order[static_cast<size_t>(i)];
        if (c < 0 || c >= n || pos[static_cast<size_t>(c)] != -1) return "not a permutation of 0..n-1: " + judge::show(order);
        pos[static_cast<size_t>(c)] = i;
    }
    for (auto& e : pre)
        if (pos[static_cast<size_t>(e[1])] > pos[static_cast<size_t>(e[0])])
            return "course " + to_string(e[1]) + " must come before " + to_string(e[0]) + ": " + judge::show(order);
    return "";
}

static void verify(int n, const vector<vector<int>>& pre) {
    CHECK_VALID(check(n, pre, call(n, pre), has_cycle(n, pre)), n, pre);
}

TEST("one course") { verify(1, {}); }
TEST("one prerequisite") { EXPECT_EQ(call(2, {{1, 0}}), {0, 1}); }
TEST("two-course cycle") { EXPECT_EQ(call(2, {{1, 0}, {0, 1}}), {}); }
TEST("no prerequisites") { verify(3, {}); }
TEST("diamond") { verify(4, {{1, 0}, {2, 0}, {3, 1}, {3, 2}}); }
TEST("three-course cycle") { EXPECT_EQ(call(3, {{1, 0}, {2, 1}, {0, 2}}), {}); }
TEST("separate components") { verify(5, {{1, 0}, {2, 1}, {4, 3}}); }
// 在這裡加你自己的邊界測資：

STRESS("random graphs vs checker", 500) {
    int n = rng.range(1, size);
    vector<vector<int>> pre;
    set<pair<int, int>> used;
    int m = rng.range(0, min(n * (n - 1), 2 * n));
    while (static_cast<int>(pre.size()) < m) {
        int a = rng.range(0, n - 1), b = rng.range(0, n - 1);
        if (a == b || used.count({a, b})) continue;
        used.insert({a, b});
        pre.push_back({a, b});
    }
    verify(n, pre);
}

TEST("perf chain of 100000 courses (repeated full scans are too slow)") {
    const int n = 100000;
    vector<vector<int>> pre;
    for (int i = 0; i + 1 < n; ++i) pre.push_back({i + 1, i});
    vector<int> got;
    double ms = judge::time_ms([&] { got = call(n, pre); });
    EXPECT_UNDER_MS(ms, 300);
    CHECK_VALID(check(n, pre, got, false), n);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
