// 測試：lc0049-group-anagrams。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0049-group-anagrams（VS Code 工作：pp: 開始/重做這題）"
#endif

// 組與組之間、組內字串的順序都不限，所以都排序之後再比對。
static vector<vector<string>> canon(vector<vector<string>> groups) {
    for (auto& g : groups) sort(g.begin(), g.end());
    sort(groups.begin(), groups.end());
    return groups;
}

static vector<vector<string>> call(vector<string> strs) { return canon(Solution().groupAnagrams(strs)); }

TEST("example") {
    EXPECT_EQ(call({"eat", "tea", "tan", "ate", "nat", "bat"}), {{"ate", "eat", "tea"}, {"bat"}, {"nat", "tan"}});
}
TEST("one empty string") { EXPECT_EQ(call({""}), {{""}}); }
TEST("single string") { EXPECT_EQ(call({"a"}), {{"a"}}); }
TEST("empty strings form one group") { EXPECT_EQ(call({"", ""}), {{"", ""}}); }
TEST("duplicate strings stay in the group") { EXPECT_EQ(call({"ab", "ba", "ab"}), {{"ab", "ab", "ba"}}); }
TEST("same letters, different counts") { EXPECT_EQ(call({"aab", "abb"}), {{"aab"}, {"abb"}}); }
TEST("different lengths") { EXPECT_EQ(call({"a", "aa"}), {{"a"}, {"aa"}}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：每個字串和每一組的第一個字串比較是不是同字母異序詞（O(n²)） ----
static vector<vector<string>> oracle(const vector<string>& strs) {
    vector<vector<string>> groups;
    for (const string& s : strs) {
        bool placed = false;
        for (auto& g : groups)
            if (g[0].size() == s.size() && is_permutation(g[0].begin(), g[0].end(), s.begin())) {
                g.push_back(s);
                placed = true;
                break;
            }
        if (!placed) groups.push_back({s});
    }
    return canon(groups);
}

STRESS("random short strings vs oracle", 500) {
    vector<string> strs;
    for (int i = rng.range(1, size * 2); i > 0; --i) strs.push_back(rng.str(rng.range(0, 3), "abc"));
    CHECK_SAME(call(strs), oracle(strs), strs);
}

TEST("perf 10000 strings of length 100 in 5000 groups (comparing with every group is too slow)") {
    judge::Rng r(49);
    vector<vector<string>> groups;
    vector<string> strs;
    for (int g = 0; g < 5000; ++g) {
        string a = r.str(100, "abcdefghijklmnopqrstuvwxyz"), b = a;
        shuffle(b.begin(), b.end(), r.engine());
        groups.push_back({a, b});
        strs.push_back(a);
        strs.push_back(b);
    }
    r.shuffle(strs);
    vector<vector<string>> got;
    double ms = judge::time_ms([&] { got = Solution().groupAnagrams(strs); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_TRUE(canon(got) == canon(groups));
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
