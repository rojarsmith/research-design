// 測試：lc0072-edit-distance。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0072-edit-distance（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(const string& a, const string& b) { return Solution().minDistance(a, b); }

TEST("both empty") { EXPECT_EQ(call("", ""), 0); }
TEST("insert everything") { EXPECT_EQ(call("", "abc"), 3); }
TEST("delete everything") { EXPECT_EQ(call("abc", ""), 3); }
TEST("identical") { EXPECT_EQ(call("abc", "abc"), 0); }
TEST("swap needs two edits") { EXPECT_EQ(call("ab", "ba"), 2); }
TEST("kitten to sitting") { EXPECT_EQ(call("kitten", "sitting"), 3); }
TEST("sunday to saturday") { EXPECT_EQ(call("sunday", "saturday"), 3); }
TEST("horse to ros") { EXPECT_EQ(call("horse", "ros"), 3); }
// 在這裡加你自己的邊界測資：

// ---- oracle：不記憶的遞迴，三種操作都試（指數時間，只適合短字串） ----
static int oracle(const string& a, const string& b, size_t i = 0, size_t j = 0) {
    if (i == a.size()) return static_cast<int>(b.size() - j);
    if (j == b.size()) return static_cast<int>(a.size() - i);
    if (a[i] == b[j]) return oracle(a, b, i + 1, j + 1);
    return 1 + min({oracle(a, b, i + 1, j + 1), oracle(a, b, i + 1, j), oracle(a, b, i, j + 1)});
}

STRESS("random strings vs plain recursion", 500) {
    string a = rng.str(rng.range(0, min(size, 6)), "abc"), b = rng.str(rng.range(0, min(size, 6)), "abc");
    CHECK_SAME(call(a, b), oracle(a, b), a, b);
}

TEST("perf two strings of length 500 (plain recursion never finishes)") {
    string a(500, 'a'), b(500, 'b');
    int got = 0;
    double ms = judge::time_ms([&] { got = call(a, b); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, 500);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
