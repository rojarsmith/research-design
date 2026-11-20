// 測試：lc0739-daily-temperatures。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0739-daily-temperatures（VS Code 工作：pp: 開始/重做這題）"
#endif

static vector<int> call(vector<int> t) { return Solution().dailyTemperatures(t); }

TEST("single day") { EXPECT_EQ(call({30}), {0}); }
TEST("strictly decreasing") { EXPECT_EQ(call({40, 39, 38}), {0, 0, 0}); }
TEST("strictly increasing") { EXPECT_EQ(call({30, 31, 32}), {1, 1, 0}); }
TEST("equal is not warmer") { EXPECT_EQ(call({50, 50, 50}), {0, 0, 0}); }
TEST("mixed") { EXPECT_EQ(call({55, 38, 53, 81, 61, 93, 97, 32, 43, 78}), {3, 1, 1, 2, 1, 1, 0, 1, 1, 0}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：每一天往後找第一個更暖的日子 ----
static vector<int> oracle(const vector<int>& t) {
    vector<int> ans(t.size(), 0);
    for (size_t i = 0; i < t.size(); ++i)
        for (size_t j = i + 1; j < t.size(); ++j)
            if (t[j] > t[i]) { ans[i] = static_cast<int>(j - i); break; }
    return ans;
}

STRESS("random vs oracle", 500) {
    auto t = rng.vec(rng.range(1, size * 3), 30, 40);
    CHECK_SAME(call(t), oracle(t), t);
}

TEST("perf n=100000 equal temperatures (O(n^2) is too slow)") {
    vector<int> t(100000, 50), got;
    double ms = judge::time_ms([&] { got = call(t); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, vector<int>(100000, 0));
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
