// 測試：lc0207-course-schedule。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0207-course-schedule（VS Code 工作：pp: 開始/重做這題）"
#endif

// [a, b] 的意思是 b 要先修，也就是 b → a 的邊。
static bool call(int n, vector<vector<int>> pre) { return Solution().canFinish(n, pre); }

TEST("one prerequisite") { EXPECT_EQ(call(2, {{1, 0}}), true); }
TEST("two-course cycle") { EXPECT_EQ(call(2, {{1, 0}, {0, 1}}), false); }
TEST("one course, no prerequisites") { EXPECT_EQ(call(1, {}), true); }
TEST("a course that requires itself") { EXPECT_EQ(call(1, {{0, 0}}), false); }
TEST("a shared prerequisite is not a cycle") { EXPECT_EQ(call(3, {{1, 0}, {2, 0}, {2, 1}}), true); }
TEST("diamond") { EXPECT_EQ(call(4, {{1, 0}, {2, 0}, {3, 1}, {3, 2}}), true); }
TEST("cycle in a separate component") { EXPECT_EQ(call(5, {{1, 0}, {3, 2}, {4, 3}, {2, 4}}), false); }
TEST("three-course cycle") { EXPECT_EQ(call(3, {{1, 0}, {2, 1}, {0, 2}}), false); }
// 在這裡加你自己的邊界測資：

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

STRESS("random graphs vs Floyd-Warshall", 500) {
    int n = rng.range(1, size);
    vector<vector<int>> pre;
    set<pair<int, int>> used;
    int m = rng.range(0, min(n * n, 2 * n));
    while (static_cast<int>(pre.size()) < m) {
        int a = rng.range(0, n - 1), b = rng.range(0, n - 1);
        if ((a == b && !rng.coin(0.1)) || used.count({a, b})) continue;  // 自己指向自己的邊偶爾出現
        used.insert({a, b});
        pre.push_back({a, b});
    }
    CHECK_SAME(call(n, pre), !has_cycle(n, pre), n, pre);
}

TEST("perf 100000 courses, each a prerequisite of the next two (DFS that re-explores finished courses is exponential)") {
    const int n = 100000;
    vector<vector<int>> pre;
    for (int i = 0; i + 1 < n; ++i) {
        pre.push_back({i + 1, i});
        if (i + 2 < n) pre.push_back({i + 2, i});
    }
    bool got = false;
    double ms = judge::time_ms([&] { got = call(n, pre); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, true);
    pre.push_back({0, n - 1});  // 最後一門課又是第一門課的先修：形成一個大環
    ms = judge::time_ms([&] { got = call(n, pre); });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(got, false);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
