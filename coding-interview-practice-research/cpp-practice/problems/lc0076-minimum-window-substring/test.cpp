// 測試：lc0076-minimum-window-substring。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0076-minimum-window-substring（VS Code 工作：pp: 開始/重做這題）"
#endif

// 隨機測資的最短視窗可能不只一個，所以對拍時檢查：長度最短、真的是 s 的子字串、涵蓋 t 的每個字元。
static string call(const string& s, const string& t) { return Solution().minWindow(s, t); }

static bool covers(const string& w, const string& t) {
    array<int, 128> need{};
    for (char c : t) ++need[static_cast<unsigned char>(c)];
    for (char c : w) --need[static_cast<unsigned char>(c)];
    for (int x : need)
        if (x > 0) return false;
    return true;
}

static string check(const string& s, const string& t, const string& got, size_t best) {
    if (best == string::npos) return got.empty() ? "" : "no window covers t, so expected \"\", got \"" + got + "\"";
    if (got.size() != best) return "expected a window of length " + to_string(best) + ", got \"" + got + "\"";
    if (s.find(got) == string::npos) return "\"" + got + "\" is not a substring of s";
    if (!covers(got, t)) return "\"" + got + "\" does not contain every character of t";
    return "";
}

TEST("example") { EXPECT_EQ(call("ADOBECODEBANC", "ABC"), "BANC"); }
TEST("whole string") { EXPECT_EQ(call("a", "a"), "a"); }
TEST("t needs more copies than s has") { EXPECT_EQ(call("a", "aa"), ""); }
TEST("t longer than s") { EXPECT_EQ(call("ab", "abc"), ""); }
TEST("case sensitive") { EXPECT_EQ(call("aA", "A"), "A"); }
TEST("order inside the window does not matter") { EXPECT_EQ(call("bba", "ab"), "ba"); }
TEST("duplicates in t") { EXPECT_EQ(call("aaflslflsldkalskaaa", "aaa"), "aaa"); }
TEST("shrinking from the left finds the shorter window") { EXPECT_EQ(call("cabwefgewcwaefgcf", "cae"), "cwae"); }
// 在這裡加你自己的邊界測資：

// ---- oracle：每個起點往右延伸到涵蓋 t 為止，取最短的長度 ----
static size_t oracle_len(const string& s, const string& t) {
    size_t best = string::npos;
    for (size_t i = 0; i < s.size(); ++i)
        for (size_t j = i; j < s.size(); ++j)
            if (covers(s.substr(i, j - i + 1), t)) {
                best = min(best, j - i + 1);
                break;
            }
    return best;
}

STRESS("random strings vs oracle", 600) {
    string s = rng.str(rng.range(1, size * 3), "abc");
    string t = rng.str(rng.range(1, size), "abc");
    CHECK_VALID(check(s, t, call(s, t), oracle_len(s, t)), s, t);
}

TEST("perf |s|=100001, the only c is in the middle (O(n^2) is too slow)") {
    string half;
    for (int i = 0; i < 25000; ++i) half += "ab";
    string s = half + "c" + half, got;
    double ms = judge::time_ms([&] { got = call(s, "abc"); });
    EXPECT_UNDER_MS(ms, 100);
    CHECK_VALID(check(s, "abc", got, 3), string("(ab)^25000 c (ab)^25000"), string("abc"));
}

TEST("perf |s| = |t| = 100000") {
    string s(100000, 'a'), got;
    double ms = judge::time_ms([&] { got = call(s, s); });
    EXPECT_UNDER_MS(ms, 100);
    EXPECT_TRUE(got == s);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
