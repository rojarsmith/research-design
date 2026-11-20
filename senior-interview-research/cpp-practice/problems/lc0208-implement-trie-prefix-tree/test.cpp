// 測試：lc0208-implement-trie-prefix-tree。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0208-implement-trie-prefix-tree（VS Code 工作：pp: 開始/重做這題）"
#endif

TEST("word vs prefix") {
    Trie t;
    t.insert("apple");
    EXPECT_EQ(t.search("apple"), true);
    EXPECT_EQ(t.search("app"), false);
    EXPECT_EQ(t.startsWith("app"), true);
    t.insert("app");
    EXPECT_EQ(t.search("app"), true);
}
TEST("prefix longer than any word") {
    Trie t;
    t.insert("ab");
    EXPECT_EQ(t.startsWith("abc"), false);
    EXPECT_EQ(t.search("abc"), false);
}
TEST("branches share a prefix") {
    Trie t;
    t.insert("car");
    t.insert("cat");
    EXPECT_EQ(t.search("ca"), false);
    EXPECT_EQ(t.startsWith("ca"), true);
    EXPECT_EQ(t.search("cab"), false);
    EXPECT_EQ(t.search("cat"), true);
}
TEST("nothing inserted") {
    Trie t;
    EXPECT_EQ(t.search("a"), false);
    EXPECT_EQ(t.startsWith("a"), false);
}
// 在這裡加你自己的邊界測資：

// ---- oracle：std::set<string> ----
STRESS("random operations vs std::set", 300) {
    Trie t;
    set<string> words;
    for (int step = 0; step < size * 5; ++step) {
        int op = rng.range(0, 2);
        string w = rng.str(rng.range(1, 4), "ab");
        if (op == 0) {
            t.insert(w);
            words.insert(w);
        } else if (op == 1) {
            CHECK_SAME(t.search(w), words.count(w) > 0, step, string("search"), w, words);
        } else {
            auto it = words.lower_bound(w);
            bool expect = it != words.end() && it->compare(0, w.size(), w) == 0;
            CHECK_SAME(t.startsWith(w), expect, step, string("startsWith"), w, words);
        }
    }
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
