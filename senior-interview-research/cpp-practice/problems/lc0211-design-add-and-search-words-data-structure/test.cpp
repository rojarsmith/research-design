// 測試：lc0211-design-add-and-search-words-data-structure。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0211-design-add-and-search-words-data-structure（VS Code 工作：pp: 開始/重做這題）"
#endif

TEST("example") {
    WordDictionary d;
    d.addWord("bad");
    d.addWord("dad");
    d.addWord("mad");
    EXPECT_EQ(d.search("pad"), false);
    EXPECT_EQ(d.search("bad"), true);
    EXPECT_EQ(d.search(".ad"), true);
    EXPECT_EQ(d.search("b.."), true);
}
TEST("a dot matches exactly one letter") {
    WordDictionary d;
    d.addWord("a");
    EXPECT_EQ(d.search("."), true);
    EXPECT_EQ(d.search(".."), false);
    EXPECT_EQ(d.search("a."), false);
}
TEST("a prefix of a word is not a word") {
    WordDictionary d;
    d.addWord("abc");
    EXPECT_EQ(d.search("ab"), false);
    EXPECT_EQ(d.search("ab."), true);
    EXPECT_EQ(d.search("abc."), false);
}
TEST("empty dictionary") {
    WordDictionary d;
    EXPECT_EQ(d.search("a"), false);
    EXPECT_EQ(d.search("."), false);
}
TEST("a dot must try every child, not just the first") {
    WordDictionary d;
    d.addWord("aab");
    d.addWord("acd");
    EXPECT_EQ(d.search("a.d"), true);
    EXPECT_EQ(d.search("a.b"), true);
    EXPECT_EQ(d.search("a.c"), false);
}
TEST("dots only match words of the same length") {
    WordDictionary d;
    d.addWord("ab");
    d.addWord("xyz");
    EXPECT_EQ(d.search(".."), true);
    EXPECT_EQ(d.search("."), false);
    EXPECT_EQ(d.search("x.."), true);
    EXPECT_EQ(d.search("a.."), false);
}
// 在這裡加你自己的邊界測資：

// ---- oracle：把所有單字存在 vector，每次查詢逐一比對 ----
static bool matches(const string& w, const string& pat) {
    if (w.size() != pat.size()) return false;
    for (size_t i = 0; i < w.size(); ++i)
        if (pat[i] != '.' && pat[i] != w[i]) return false;
    return true;
}

STRESS("random operations vs a list of words", 400) {
    WordDictionary d;
    vector<string> words;
    for (int step = 0; step < size * 5; ++step) {
        if (rng.coin()) {
            string w = rng.str(rng.range(1, 3), "ab");
            d.addWord(w);
            words.push_back(w);
        } else {
            string pat = rng.str(rng.range(1, 3), "ab.");
            if (count(pat.begin(), pat.end(), '.') > 2) pat[0] = 'a';  // 題目保證查詢最多 2 個點
            bool expect = any_of(words.begin(), words.end(), [&](const string& x) { return matches(x, pat); });
            CHECK_SAME(d.search(pat), expect, step, string("search"), pat, words);
        }
    }
}

// 所有單字共用 20 個 a 的前綴，逐一比對時每次都要比到第 21 個字元以後。
// 一半的查詢取自已加入的單字（必定為 true），另一半在後 5 碼放一個沒有單字會用到的 z（必定為 false）。
TEST("perf 10000 words with a shared 20-letter prefix, 10000 searches (above LeetCode's 10^4 calls; scanning every word is too slow)") {
    judge::Rng r(211);
    const string prefix(20, 'a');
    vector<string> words, queries;
    for (int i = 0; i < 10000; ++i) words.push_back(prefix + r.str(5, "abcdefghijklmnopqrstuvwxy"));
    int expect_true = 0;
    for (int i = 0; i < 10000; ++i) {
        string q = words[static_cast<size_t>(r.range(0, 9999))];
        int zpos = -1;
        if (i % 2 == 0) {
            ++expect_true;
        } else {
            zpos = r.range(20, 24);
            q[static_cast<size_t>(zpos)] = 'z';
        }
        for (int dots = r.range(0, 2); dots > 0; --dots) {
            int p = r.range(0, 24);
            if (p != zpos) q[static_cast<size_t>(p)] = '.';
        }
        queries.push_back(q);
    }
    int hits = 0;
    double ms = judge::time_ms([&] {
        WordDictionary d;
        for (auto& w : words) d.addWord(w);
        for (auto& q : queries) hits += d.search(q);
    });
    EXPECT_UNDER_MS(ms, 300);
    EXPECT_EQ(hits, expect_true);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
