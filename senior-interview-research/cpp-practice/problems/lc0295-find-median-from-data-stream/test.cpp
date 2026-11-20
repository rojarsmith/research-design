// 測試：lc0295-find-median-from-data-stream。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0295-find-median-from-data-stream（VS Code 工作：pp: 開始/重做這題）"
#endif

static double median_of(const vector<int>& xs) {
    MedianFinder m;
    for (int x : xs) m.addNum(x);
    return m.findMedian();
}

TEST("one number") { EXPECT_EQ(median_of({1}), 1.0); }
TEST("two numbers") { EXPECT_EQ(median_of({1, 2}), 1.5); }
TEST("three numbers") { EXPECT_EQ(median_of({1, 2, 3}), 2.0); }
TEST("unsorted with duplicates") { EXPECT_EQ(median_of({5, -3, 10, -3}), 1.0); }
TEST("two INT_MAX values") { EXPECT_EQ(median_of({INT_MAX, INT_MAX}), 2147483647.0); }
TEST("INT_MIN and INT_MAX") { EXPECT_EQ(median_of({INT_MIN, INT_MAX}), -0.5); }
TEST("median after every insert") {
    MedianFinder m;
    m.addNum(4);
    EXPECT_EQ(m.findMedian(), 4.0);
    m.addNum(8);
    EXPECT_EQ(m.findMedian(), 6.0);
    m.addNum(1);
    EXPECT_EQ(m.findMedian(), 4.0);
}
// 在這裡加你自己的邊界測資：

// ---- oracle：每次插入後排序好的陣列 ----
static double oracle_median(const vector<int>& sorted_xs) {
    size_t n = sorted_xs.size();
    return n % 2 ? sorted_xs[n / 2] : (static_cast<double>(sorted_xs[n / 2 - 1]) + sorted_xs[n / 2]) / 2.0;
}

STRESS("random streams vs sorted array", 400) {
    MedianFinder m;
    vector<int> xs;
    for (int step = 0; step < size * 3; ++step) {
        int x = rng.range(-5, 5);
        m.addNum(x);
        xs.insert(upper_bound(xs.begin(), xs.end(), x), x);
        CHECK_SAME(m.findMedian(), oracle_median(xs), step, xs);
    }
}

TEST("perf 100000 inserts with a query after each (sorting each time is too slow)") {
    const int n = 100000;
    vector<int> xs(n);
    unsigned s = 12345;
    for (int& x : xs) x = static_cast<int>((s = s * 1103515245u + 12345u) >> 1);
    double last = 0;
    double ms = judge::time_ms([&] {
        MedianFinder m;
        for (int x : xs) {
            m.addNum(x);
            last = m.findMedian();
        }
    });
    EXPECT_UNDER_MS(ms, 300);
    sort(xs.begin(), xs.end());
    EXPECT_EQ(last, oracle_median(xs));
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
