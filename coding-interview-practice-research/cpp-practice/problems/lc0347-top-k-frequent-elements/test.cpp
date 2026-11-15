// 測試：lc0347-top-k-frequent-elements。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0347-top-k-frequent-elements（VS Code 工作：pp: 開始/重做這題）"
#endif

// 答案可以任意順序回傳，所以排序後再比對。題目保證答案唯一。
static vector<int> call(vector<int> nums, int k) {
    auto got = Solution().topKFrequent(nums, k);
    sort(got.begin(), got.end());
    return got;
}

TEST("example") { EXPECT_EQ(call({1, 1, 1, 2, 2, 3}, 2), {1, 2}); }
TEST("single element") { EXPECT_EQ(call({1}, 1), {1}); }
TEST("negative values") { EXPECT_EQ(call({4, 4, -1, -1, -1, 2}, 1), {-1}); }
TEST("k equals the number of distinct values") { EXPECT_EQ(call({4, 5, 6}, 3), {4, 5, 6}); }
TEST("first appearance does not matter") { EXPECT_EQ(call({5, 3, 1, 1, 1, 3, 73, 1}, 2), {1, 3}); }
TEST("every value, different counts") { EXPECT_EQ(call({3, 2, 3, 1, 2, 3}, 3), {1, 2, 3}); }
// 在這裡加你自己的邊界測資：

// ---- oracle：用 map 計數，再依次數整個排序 ----
static vector<int> oracle(const vector<int>& nums, int k) {
    map<int, int> cnt;
    for (int x : nums) ++cnt[x];
    vector<pair<int, int>> by_count;
    for (auto& [x, c] : cnt) by_count.push_back({-c, x});
    sort(by_count.begin(), by_count.end());
    vector<int> top;
    for (int i = 0; i < k; ++i) top.push_back(by_count[static_cast<size_t>(i)].second);
    sort(top.begin(), top.end());
    return top;
}

// 每個值的次數都不同，答案才會唯一。
STRESS("random arrays with distinct counts vs oracle", 500) {
    int d = rng.range(1, size + 1);
    auto vals = rng.distinct(d, -20, 20);
    vector<int> times(static_cast<size_t>(d));
    iota(times.begin(), times.end(), 1);
    rng.shuffle(times);
    vector<int> nums;
    for (size_t i = 0; i < vals.size(); ++i) nums.insert(nums.end(), static_cast<size_t>(times[i]), vals[i]);
    rng.shuffle(nums);
    int k = rng.range(1, d);
    CHECK_SAME(call(nums, k), oracle(nums, k), nums, k);
}

TEST("perf n=100000 with 60000 distinct values (a full scan per value is too slow)") {
    vector<int> nums;
    for (int v = 0; v < 60000; ++v) nums.push_back(v);
    for (int v = 0; v < 10; ++v) nums.insert(nums.end(), 4000, v);
    judge::Rng r(347);
    r.shuffle(nums);
    vector<int> got;
    double ms = judge::time_ms([&] { got = call(nums, 10); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
