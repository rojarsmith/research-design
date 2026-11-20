// 測試：lc1143-longest-common-subsequence。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc1143-longest-common-subsequence（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(const string& a, const string& b) { return Solution().longestCommonSubsequence(a, b); }

TEST("same single char") { EXPECT_EQ(call("a", "a"), 1); }
TEST("no common char") { EXPECT_EQ(call("a", "b"), 0); }
TEST("identical") { EXPECT_EQ(call("abc", "abc"), 3); }
TEST("skip in the second string") { EXPECT_EQ(call("abcde", "ace"), 3); }
TEST("one inside the other") { EXPECT_EQ(call("abcba", "abcbcba"), 5); }
TEST("scattered matches") { EXPECT_EQ(call("xmjyauz", "mzjawxu"), 4); }
// 在這裡加你自己的邊界測資：

// ---- oracle：列舉第一個字串的所有子序列，檢查是不是第二個字串的子序列 ----
static bool is_subseq(const string& sub, const string& s) {
    size_t i = 0;
    for (char c : s)
        if (i < sub.size() && sub[i] == c) ++i;
    return i == sub.size();
}
static int oracle(const string& a, const string& b) {
    int best = 0;
    for (unsigned mask = 0; mask < (1u << a.size()); ++mask) {
        string sub;
        for (size_t i = 0; i < a.size(); ++i)
            if (mask >> i & 1u) sub += a[i];
        if (is_subseq(sub, b)) best = max(best, static_cast<int>(sub.size()));
    }
    return best;
}

STRESS("random strings vs subsequence enumeration", 500) {
    string a = rng.str(rng.range(1, size), "abc"), b = rng.str(rng.range(1, size + 2), "abc");
    CHECK_SAME(call(a, b), oracle(a, b), a, b);
}

TEST("perf two strings of length 1000 (plain recursion never finishes)") {
    string a, b;
    for (int i = 0; i < 500; ++i) { a += "ab"; b += "ba"; }
    int got = 0;
    double ms = judge::time_ms([&] { got = call(a, b); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, 999);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
