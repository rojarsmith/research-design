// 測試：lc0417-pacific-atlantic-water-flow。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0417-pacific-atlantic-water-flow（VS Code 工作：pp: 開始/重做這題）"
#endif

// 答案是 [r, c] 座標的清單，順序不限：只把座標之間排序（sorted_outer）
static vector<vector<int>> call(vector<vector<int>> h) { return sorted_outer(Solution().pacificAtlantic(h)); }

TEST("1x1 touches both oceans") { EXPECT_EQ(call({{1}}), {{0, 0}}); }
TEST("2x2") { EXPECT_EQ(call({{1, 2}, {4, 3}}), {{0, 1}, {1, 0}, {1, 1}}); }
TEST("flat ring around a pit") {
    EXPECT_EQ(call({{3, 3, 3}, {3, 1, 3}, {3, 3, 3}}), {{0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 2}, {2, 0}, {2, 1}, {2, 2}});
}
TEST("all equal") { EXPECT_EQ(call({{1, 1}, {1, 1}}), {{0, 0}, {0, 1}, {1, 0}, {1, 1}}); }
TEST("slope down to the bottom right") {
    EXPECT_EQ(call({{5, 4, 3}, {4, 3, 2}, {3, 2, 1}}), {{0, 0}, {0, 1}, {0, 2}, {1, 0}, {2, 0}});
}
// 在這裡加你自己的邊界測資：

// ---- oracle：每一格各自往下坡（高度 ≤）流，看能碰到哪些海洋，O((mn)²) ----
static vector<vector<int>> oracle(const vector<vector<int>>& h) {
    int R = static_cast<int>(h.size()), C = static_cast<int>(h[0].size());
    vector<vector<int>> out;
    for (int sr = 0; sr < R; ++sr)
        for (int sc = 0; sc < C; ++sc) {
            vector<vector<char>> seen(static_cast<size_t>(R), vector<char>(static_cast<size_t>(C), 0));
            vector<pair<int, int>> st{{sr, sc}};
            seen[static_cast<size_t>(sr)][static_cast<size_t>(sc)] = 1;
            bool pac = false, atl = false;
            while (!st.empty()) {
                auto [r, c] = st.back();
                st.pop_back();
                pac |= r == 0 || c == 0;
                atl |= r == R - 1 || c == C - 1;
                const int dr[] = {1, -1, 0, 0}, dc[] = {0, 0, 1, -1};
                for (int d = 0; d < 4; ++d) {
                    int nr = r + dr[d], nc = c + dc[d];
                    if (nr < 0 || nc < 0 || nr >= R || nc >= C || seen[static_cast<size_t>(nr)][static_cast<size_t>(nc)]) continue;
                    if (h[static_cast<size_t>(nr)][static_cast<size_t>(nc)] > h[static_cast<size_t>(r)][static_cast<size_t>(c)]) continue;
                    seen[static_cast<size_t>(nr)][static_cast<size_t>(nc)] = 1;
                    st.push_back({nr, nc});
                }
            }
            if (pac && atl) out.push_back({sr, sc});
        }
    return out;
}

STRESS("random height maps vs per-cell flow", 500) {
    int R = rng.range(1, 1 + size / 2), C = rng.range(1, 1 + size / 2);
    vector<vector<int>> h(static_cast<size_t>(R));
    for (auto& row : h) row = rng.vec(C, 0, 3);
    CHECK_SAME(call(h), oracle(h), h);
}

TEST("perf 500x500 flat map (flowing from every cell is too slow)") {
    vector<vector<int>> h(500, vector<int>(500, 7));
    size_t got = 0;
    double ms = judge::time_ms([&] { got = Solution().pacificAtlantic(h).size(); });
    EXPECT_UNDER_MS(ms, 500);
    EXPECT_EQ(got, size_t{250000});
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
