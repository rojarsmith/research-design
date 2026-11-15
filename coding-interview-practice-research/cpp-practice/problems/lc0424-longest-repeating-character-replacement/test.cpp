// 測試：lc0424-longest-repeating-character-replacement。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0424-longest-repeating-character-replacement（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(const string& s, int k) { return Solution().characterReplacement(s, k); }

TEST("example") { EXPECT_EQ(call("ABAB", 2), 4); }
TEST("example 2") { EXPECT_EQ(call("AABABBA", 1), 4); }
TEST("single char") { EXPECT_EQ(call("A", 0), 1); }
TEST("k = 0 means the longest run") { EXPECT_EQ(call("AABBBCC", 0), 3); }
TEST("all same") { EXPECT_EQ(call("AAAA", 2), 4); }
TEST("k covers the whole string") { EXPECT_EQ(call("ABC", 3), 3); }
TEST("replace at the front") { EXPECT_EQ(call("ABBB", 2), 4); }
TEST("run at the end") { EXPECT_EQ(call("ABAA", 0), 2); }
// 在這裡加你自己的邊界測資：

// ---- oracle：每個起點往右延伸，視窗長度減掉最多的字母數 <= k 就合法（O(n²)） ----
static int oracle(const string& s, int k) {
    int best = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        array<int, 26> cnt{};
        int top = 0;
        for (size_t j = i; j < s.size(); ++j) {
            top = max(top, ++cnt[static_cast<size_t>(s[j] - 'A')]);
            int len = static_cast<int>(j - i + 1);
            if (len - top <= k) best = max(best, len);
        }
    }
    return best;
}

STRESS("random strings vs oracle", 600) {
    string s = rng.str(rng.range(1, size * 3), "ABC");
    int k = rng.range(0, static_cast<int>(s.size()));
    CHECK_SAME(call(s, k), oracle(s, k), s, k);
}

TEST("perf n=100000 alternating AB, k=1000 (O(n^2) is too slow)") {
    string s;
    for (int i = 0; i < 50000; ++i) s += "AB";
    int got = 0;
    double ms = judge::time_ms([&] { got = call(s, 1000); });
    EXPECT_UNDER_MS(ms, 100);
    EXPECT_EQ(got, 2001);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
