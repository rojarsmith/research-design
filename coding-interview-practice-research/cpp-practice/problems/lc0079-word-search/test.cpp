// 測試：lc0079-word-search。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0079-word-search（VS Code 工作：pp: 開始/重做這題）"
#endif

static bool call(vector<vector<char>> board, const string& word) { return Solution().exist(board, word); }

static vector<vector<char>> sample() { return {{'C', 'A', 'T'}, {'X', 'R', 'S'}, {'E', 'E', 'D'}}; }

TEST("straight line") { EXPECT_EQ(call(sample(), "CAT"), true); }
TEST("turns a corner") { EXPECT_EQ(call(sample(), "CAR"), true); }
TEST("goes down at the end") { EXPECT_EQ(call(sample(), "CATS"), true); }
TEST("starts in the middle") { EXPECT_EQ(call(sample(), "RED"), true); }
TEST("would need to reuse a cell") { EXPECT_EQ(call(sample(), "TAT"), false); }
TEST("dead end after a partial match") { EXPECT_EQ(call(sample(), "DEER"), false); }
TEST("single cell, reused") { EXPECT_EQ(call({{'a'}}, "aa"), false); }
TEST("single cell") { EXPECT_EQ(call({{'a'}}, "a"), true); }
TEST("board is restored after a search") {
    auto b = sample();
    Solution().exist(b, "CATS");
    EXPECT_EQ(b, sample());
}
// 在這裡加你自己的邊界測資：

// ---- oracle：狀態（目前格子、已用格子的 bitmask）的搜尋，只適合很小的棋盤 ----
static bool oracle(const vector<vector<char>>& b, const string& w) {
    int R = static_cast<int>(b.size()), C = static_cast<int>(b[0].size());
    set<tuple<int, int, unsigned>> frontier;  // (cell, matched length, used mask)
    for (int cell = 0; cell < R * C; ++cell)
        if (b[static_cast<size_t>(cell / C)][static_cast<size_t>(cell % C)] == w[0]) frontier.insert({cell, 1, 1u << cell});
    for (size_t len = 1; len < w.size(); ++len) {
        set<tuple<int, int, unsigned>> next;
        for (auto [cell, l, mask] : frontier) {
            int r = cell / C, c = cell % C;
            const int dr[] = {1, -1, 0, 0}, dc[] = {0, 0, 1, -1};
            for (int d = 0; d < 4; ++d) {
                int nr = r + dr[d], nc = c + dc[d], ncell = nr * C + nc;
                if (nr < 0 || nc < 0 || nr >= R || nc >= C || (mask >> ncell & 1u)) continue;
                if (b[static_cast<size_t>(nr)][static_cast<size_t>(nc)] == w[len]) next.insert({ncell, l + 1, mask | 1u << ncell});
            }
        }
        frontier = next;
    }
    return !frontier.empty();
}

STRESS("random small boards vs state search", 500) {
    int R = rng.range(1, 3), C = rng.range(1, 3);
    vector<vector<char>> b(static_cast<size_t>(R));
    for (auto& row : b) {
        string s = rng.str(C, "ab");
        row.assign(s.begin(), s.end());
    }
    string w = rng.str(rng.range(1, 1 + size / 2), "ab");
    CHECK_SAME(call(b, w), oracle(b, w), b, w);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
