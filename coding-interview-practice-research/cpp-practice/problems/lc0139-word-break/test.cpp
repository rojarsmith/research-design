// 測試：lc0139-word-break。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0139-word-break（VS Code 工作：pp: 開始/重做這題）"
#endif

static bool call(const string& s, vector<string> dict) { return Solution().wordBreak(s, dict); }

TEST("two words") { EXPECT_EQ(call("catsdog", {"cats", "dog", "cat"}), true); }
TEST("no full split") { EXPECT_EQ(call("catsandog", {"cats", "dog", "sand", "and", "cat"}), false); }
TEST("several short words") { EXPECT_EQ(call("abcd", {"a", "abc", "b", "cd"}), true); }
TEST("leftover character") { EXPECT_EQ(call("aaab", {"a", "aa"}), false); }
TEST("reuse a word") { EXPECT_EQ(call("aaaa", {"aa"}), true); }
TEST("nothing matches") { EXPECT_EQ(call("a", {"b"}), false); }
TEST("longest match first is wrong") { EXPECT_EQ(call("cars", {"car", "ca", "rs"}), true); }
// 在這裡加你自己的邊界測資：

// ---- oracle：不記憶的遞迴，試每一種切法（指數時間，只適合短字串） ----
static bool oracle(const string& s, const vector<string>& dict, size_t start = 0) {
    if (start == s.size()) return true;
    for (auto& w : dict)
        if (s.compare(start, w.size(), w) == 0 && oracle(s, dict, start + w.size())) return true;
    return false;
}

STRESS("random strings vs plain recursion", 500) {
    string s = rng.str(rng.range(1, size + 2), "ab");
    vector<string> dict;
    for (int i = rng.range(1, 4); i > 0; --i) dict.push_back(rng.str(rng.range(1, 3), "ab"));
    CHECK_SAME(call(s, dict), oracle(s, dict), s, dict);
}

TEST("perf 200 a's then b (recursion without memoization never finishes)") {
    string s(200, 'a');
    s += 'b';
    vector<string> dict;
    for (int len = 1; len <= 10; ++len) dict.push_back(string(static_cast<size_t>(len), 'a'));
    bool got = true;
    double ms = judge::time_ms([&] { got = call(s, dict); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, false);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
