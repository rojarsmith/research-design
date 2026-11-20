// 測試：lc0136-single-number。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0136-single-number（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> nums) { return Solution().singleNumber(nums); }

TEST("example") { EXPECT_EQ(call({2, 2, 1}), 1); }
TEST("example 2") { EXPECT_EQ(call({4, 1, 2, 1, 2}), 4); }
TEST("single element") { EXPECT_EQ(call({1}), 1); }
TEST("negative values") { EXPECT_EQ(call({-1, -1, -2}), -2); }
TEST("the single number is 0") { EXPECT_EQ(call({0, 1, 1}), 0); }
TEST("extreme values (outside LeetCode's range; sum tricks overflow)") { EXPECT_EQ(call({INT_MAX, INT_MIN, INT_MAX}), INT_MIN); }
// 在這裡加你自己的邊界測資：

// ---- oracle：用 map 數每個值出現幾次 ----
static int oracle(const vector<int>& nums) {
    map<int, int> cnt;
    for (int x : nums) ++cnt[x];
    for (auto& [x, c] : cnt)
        if (c == 1) return x;
    return 0;
}

STRESS("random pairs plus one single vs oracle", 500) {
    int pairs = rng.range(0, size * 2);
    auto vals = rng.distinct(pairs + 1, -20, 20);
    vector<int> nums;
    for (int i = 0; i < pairs; ++i) nums.insert(nums.end(), 2, vals[static_cast<size_t>(i)]);
    nums.push_back(vals.back());
    rng.shuffle(nums);
    CHECK_SAME(call(nums), oracle(nums), nums);
}

TEST("perf n=29999 (counting each value with a full scan is O(n^2))") {
    judge::Rng r(136);
    auto vals = r.distinct(15000, -30000, 30000);
    vector<int> nums;
    for (size_t i = 0; i + 1 < vals.size(); ++i) nums.insert(nums.end(), 2, vals[i]);
    nums.push_back(vals.back());
    r.shuffle(nums);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(nums); });
    EXPECT_UNDER_MS(ms, 30);
    EXPECT_EQ(got, vals.back());
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
