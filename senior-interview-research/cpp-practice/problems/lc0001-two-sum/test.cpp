// 測試：lc0001-two-sum。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0001-two-sum（VS Code 工作：pp: 開始/重做這題）"
#endif

// 兩個下標可以用任意順序回傳，隨機測資也可能有好幾組解，所以用檢查器驗證。
static vector<int> call(vector<int> nums, int target) { return Solution().twoSum(nums, target); }

static string check(const vector<int>& nums, int target, const vector<int>& got) {
    if (got.size() != 2) return "expected two indices, got " + judge::show(got);
    int n = static_cast<int>(nums.size()), i = got[0], j = got[1];
    if (i < 0 || i >= n || j < 0 || j >= n) return "index out of range: " + judge::show(got);
    if (i == j) return "used the same element twice: " + judge::show(got);
    if (1LL * nums[static_cast<size_t>(i)] + nums[static_cast<size_t>(j)] != target)
        return "nums[" + to_string(i) + "] + nums[" + to_string(j) + "] is not the target: " + judge::show(got);
    return "";
}

static void verify(const vector<int>& nums, int target) {
    CHECK_VALID(check(nums, target, call(nums, target)), nums, target);
}

TEST("example") { verify({2, 7, 11, 15}, 9); }
TEST("two equal values") { verify({3, 3}, 6); }
TEST("cannot use one element twice") { verify({3, 2, 4}, 6); }
TEST("negative values") { verify({-3, 4, 3, 90}, 0); }
TEST("zeros") { verify({0, 4, 3, 0}, 0); }
TEST("pair at both ends") { verify({1, 5, 6, 9}, 10); }
TEST("extreme values") { verify({1000000000, 7, -1000000000}, 0); }
// 在這裡加你自己的邊界測資：

// ---- 對拍：先放進一組保證有解的數對，再用檢查器驗證任何一組合法的答案 ----
STRESS("random arrays with a planted pair vs checker", 500) {
    int n = rng.range(2, size * 2 + 2);
    auto nums = rng.vec(n, -5, 5);
    int i = rng.range(0, n - 1), j = rng.range(0, n - 2);
    if (j >= i) ++j;
    verify(nums, nums[static_cast<size_t>(i)] + nums[static_cast<size_t>(j)]);
}

TEST("perf n=50000, the pair is the last two (above LeetCode's 10^4, so O(n^2) shows up)") {
    const int n = 50000;
    vector<int> nums(n);
    for (int i = 0; i < n - 2; ++i) nums[static_cast<size_t>(i)] = 2 * i;  // 偶數兩兩相加到不了目標
    nums[n - 2] = 1;
    nums[n - 1] = 200001;
    vector<int> got;
    double ms = judge::time_ms([&] { got = call(nums, 200002); });
    EXPECT_UNDER_MS(ms, 100);
    sort(got.begin(), got.end());
    EXPECT_EQ(got, {n - 2, n - 1});
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
