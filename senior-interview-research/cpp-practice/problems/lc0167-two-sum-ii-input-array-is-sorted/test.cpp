// 測試：lc0167-two-sum-ii-input-array-is-sorted。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0167-two-sum-ii-input-array-is-sorted（VS Code 工作：pp: 開始/重做這題）"
#endif

// 手算的例子答案唯一，直接比對；隨機測資可能有好幾組解，所以對拍時用檢查器。
static vector<int> call(vector<int> numbers, int target) { return Solution().twoSum(numbers, target); }

static string check(const vector<int>& a, int target, const vector<int>& got) {
    if (got.size() != 2) return "expected [index1, index2], got " + judge::show(got);
    int n = static_cast<int>(a.size()), i = got[0], j = got[1];
    if (!(1 <= i && i < j && j <= n)) return "need 1 <= index1 < index2 <= n (1-indexed): " + judge::show(got);
    if (a[static_cast<size_t>(i - 1)] + a[static_cast<size_t>(j - 1)] != target)
        return "the two values do not add up to target: " + judge::show(got);
    return "";
}

TEST("example") { EXPECT_EQ(call({2, 7, 11, 15}, 9), {1, 2}); }
TEST("indices are 1-based") { EXPECT_EQ(call({2, 3, 4}, 6), {1, 3}); }
TEST("negative and zero") { EXPECT_EQ(call({-1, 0}, -1), {1, 2}); }
TEST("equal values") { EXPECT_EQ(call({0, 0, 3, 4}, 0), {1, 2}); }
TEST("duplicates in the middle") { EXPECT_EQ(call({1, 2, 3, 4, 4, 9, 56, 90}, 8), {4, 5}); }
TEST("pair at both ends") { EXPECT_EQ(call({-5, 1, 2, 10}, 5), {1, 4}); }
// 在這裡加你自己的邊界測資：

// ---- 對拍：已排序的隨機陣列，先放進一組保證有解的數對，再用檢查器驗證 ----
STRESS("random sorted arrays with a planted pair vs checker", 500) {
    int n = rng.range(2, size * 2 + 2);
    auto a = rng.vec(n, -6, 6);
    sort(a.begin(), a.end());
    int i = rng.range(0, n - 2), j = rng.range(i + 1, n - 1);
    int target = a[static_cast<size_t>(i)] + a[static_cast<size_t>(j)];
    CHECK_VALID(check(a, target, call(a, target)), a, target);
}

TEST("perf n=50000, the answer is the last two (above LeetCode's 3*10^4, so O(n^2) shows up)") {
    const int n = 50000;
    vector<int> a(n);
    iota(a.begin(), a.end(), 0);
    vector<int> got;
    double ms = judge::time_ms([&] { got = call(a, 2 * n - 3); });
    EXPECT_UNDER_MS(ms, 50);
    EXPECT_EQ(got, {n - 1, n});
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
