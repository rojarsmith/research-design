// 測試：lc0003-longest-substring-without-repeating-characters。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0003-longest-substring-without-repeating-characters（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(const string& s) { return Solution().lengthOfLongestSubstring(s); }

TEST("empty") { EXPECT_EQ(call(""), 0); }
TEST("single char") { EXPECT_EQ(call("a"), 1); }
TEST("all same") { EXPECT_EQ(call("aaaa"), 1); }
TEST("all distinct") { EXPECT_EQ(call("abcd"), 4); }
TEST("abba") { EXPECT_EQ(call("abba"), 2); }
TEST("dvdf") { EXPECT_EQ(call("dvdf"), 3); }
TEST("tmmzuxt") { EXPECT_EQ(call("tmmzuxt"), 5); }
TEST("spaces are characters") { EXPECT_EQ(call("a b"), 3); }
// 在這裡加你自己的邊界測資：

// ---- oracle：每個起點往右延伸到出現重複為止 ----
static int oracle(const string& s) {
    int best = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        set<char> seen;
        size_t j = i;
        while (j < s.size() && !seen.count(s[j])) seen.insert(s[j++]);
        best = max(best, static_cast<int>(j - i));
    }
    return best;
}

STRESS("random vs oracle", 600) {
    string s = rng.str(rng.range(0, size * 3), "abc ");
    CHECK_SAME(call(s), oracle(s), s);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
