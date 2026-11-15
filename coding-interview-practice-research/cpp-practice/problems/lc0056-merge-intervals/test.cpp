// 測試：lc0056-merge-intervals。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0056-merge-intervals（VS Code 工作：pp: 開始/重做這題）"
#endif

// 輸出順序不限：比對前先排序
static vector<vector<int>> call(vector<vector<int>> in) { return normalized(Solution().merge(in)); }

TEST("single") { EXPECT_EQ(call({{1, 4}}), {{1, 4}}); }
TEST("touching intervals merge") { EXPECT_EQ(call({{1, 4}, {4, 5}}), {{1, 5}}); }
TEST("contained interval") { EXPECT_EQ(call({{1, 4}, {2, 3}}), {{1, 4}}); }
TEST("unsorted input") { EXPECT_EQ(call({{6, 8}, {1, 3}, {2, 4}, {9, 10}}), {{1, 4}, {6, 8}, {9, 10}}); }
TEST("adjacent points do not touch") { EXPECT_EQ(call({{0, 0}, {1, 1}}), {{0, 0}, {1, 1}}); }
TEST("identical intervals") { EXPECT_EQ(call({{2, 2}, {2, 2}}), {{2, 2}}); }
TEST("one wide interval swallows several") { EXPECT_EQ(call({{1, 10}, {2, 3}, {4, 5}, {11, 12}}), {{1, 10}, {11, 12}}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：反覆找任意一對重疊的區間合併，直到沒有可合併的 ----
static vector<vector<int>> oracle(vector<vector<int>> v) {
    for (bool changed = true; changed;) {
        changed = false;
        for (size_t i = 0; i < v.size() && !changed; ++i)
            for (size_t j = i + 1; j < v.size() && !changed; ++j)
                if (v[i][0] <= v[j][1] && v[j][0] <= v[i][1]) {
                    v[i] = {min(v[i][0], v[j][0]), max(v[i][1], v[j][1])};
                    v.erase(v.begin() + static_cast<long>(j));
                    changed = true;
                }
    }
    return normalized(v);
}

STRESS("random intervals vs oracle", 500) {
    vector<vector<int>> in;
    int n = rng.range(1, size * 2);
    for (int i = 0; i < n; ++i) {
        int s = rng.range(0, 12);
        in.push_back({s, s + rng.range(0, 4)});
    }
    CHECK_SAME(call(in), oracle(in), in);
}

TEST("perf 100000 disjoint intervals (pairwise merging is too slow)") {
    const int n = 100000;
    vector<vector<int>> in, expect;
    for (int i = 0; i < n; ++i) in.push_back({2 * i, 2 * i + 1});
    expect = in;
    shuffle(in.begin(), in.end(), mt19937(3));
    vector<vector<int>> got;
    double ms = judge::time_ms([&] { got = call(in); });
    EXPECT_UNDER_MS(ms, 400);
    EXPECT_EQ(got, expect);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
