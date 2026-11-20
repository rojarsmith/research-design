// 測試：{{ID}}。oracle 寫成最直接的暴力解，只用來對拍。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start {{ID}}"
#endif

static int call(vector<int> nums) { return Solution().solve(nums); }

// 1. 自己手算的小例子與邊界：空、單一元素、全部相同、已排序、極值
TEST("example") { EXPECT_EQ(call({1, 2, 3}), 0); }

// 2. 暴力 oracle：寫成最直接、明顯正確的做法，不在意速度
static int oracle(const vector<int>& nums) {
    (void)nums;
    return 0;
}

// 3. 對拍：小的隨機輸入，找出最小反例
STRESS("random vs oracle", 500) {
    auto nums = rng.vec(rng.range(1, size * 2), -5, 5);
    CHECK_SAME(call(nums), oracle(nums), nums);
}

// 4. 效能：用題目上限大小的輸入，確認複雜度
TEST("perf") {
    vector<int> nums(100000, 1);
    int got = 0;
    double ms = judge::time_ms([&] { got = call(nums); });
    EXPECT_UNDER_MS(ms, 300);
    (void)got;
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
