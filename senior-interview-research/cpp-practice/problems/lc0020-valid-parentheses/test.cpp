// 測試：lc0020-valid-parentheses。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0020-valid-parentheses（VS Code 工作：pp: 開始/重做這題）"
#endif

static bool call(const string& s) { return Solution().isValid(s); }

TEST("one pair") { EXPECT_EQ(call("()"), true); }
TEST("three kinds side by side") { EXPECT_EQ(call("()[]{}"), true); }
TEST("wrong kind") { EXPECT_EQ(call("(]"), false); }
TEST("crossed pairs") { EXPECT_EQ(call("([)]"), false); }
TEST("nested") { EXPECT_EQ(call("{[]}"), true); }
TEST("only an opening bracket") { EXPECT_EQ(call("("), false); }
TEST("closing bracket on an empty stack") { EXPECT_EQ(call("]"), false); }
TEST("left open at the end") { EXPECT_EQ(call("(()"), false); }
TEST("close before open") { EXPECT_EQ(call(")("), false); }
TEST("empty string is valid") { EXPECT_EQ(call(""), true); }
// 在這裡加你自己的邊界測資：

// ---- oracle：反覆刪掉相鄰的 ()、[]、{}，最後刪光就是合法（O(n²)） ----
static bool oracle(string s) {
    for (bool changed = true; changed;) {
        changed = false;
        for (const char* p : {"()", "[]", "{}"}) {
            size_t pos = s.find(p);
            if (pos != string::npos) {
                s.erase(pos, 2);
                changed = true;
            }
        }
    }
    return s.empty();
}

// 一半的測資是合法字串（有時改掉一個字元），否則隨機字串幾乎都不合法。
static string random_valid(judge::Rng& r, int pairs) {
    const string open = "([{", close = ")]}";
    string s, pending;
    int opened = 0;
    while (opened < pairs || !pending.empty()) {
        if (opened < pairs && (pending.empty() || r.coin())) {
            size_t kind = static_cast<size_t>(r.range(0, 2));
            s += open[kind];
            pending += close[kind];
            ++opened;
        } else {
            s += pending.back();
            pending.pop_back();
        }
    }
    return s;
}

STRESS("random strings vs repeated removal", 600) {
    string s;
    if (rng.coin()) {
        s = random_valid(rng, rng.range(1, size));
        if (rng.coin()) s[static_cast<size_t>(rng.range(0, static_cast<int>(s.size()) - 1))] = "()[]{}"[rng.range(0, 5)];
    } else {
        s = rng.str(rng.range(1, size * 2), "()[]{}");
    }
    CHECK_SAME(call(s), oracle(s), s);
}

TEST("perf 100000 nested brackets (above LeetCode's 10^4; repeated removal is O(n^2))") {
    const string open = "([{", close = ")]}";
    string s;
    for (int i = 0; i < 50000; ++i) s += open[static_cast<size_t>(i % 3)];
    for (int i = 49999; i >= 0; --i) s += close[static_cast<size_t>(i % 3)];
    bool got = false;
    double ms = judge::time_ms([&] { got = call(s); });
    EXPECT_UNDER_MS(ms, 50);
    EXPECT_EQ(got, true);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
