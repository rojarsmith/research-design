// 測試：lc0134-gas-station。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0134-gas-station（VS Code 工作：pp: 開始/重做這題）"
#endif

static int call(vector<int> gas, vector<int> cost) { return Solution().canCompleteCircuit(gas, cost); }

TEST("one station, enough gas") { EXPECT_EQ(call({5}, {4}), 0); }
TEST("one station, not enough") { EXPECT_EQ(call({4}, {5}), -1); }
TEST("must start at the second station") { EXPECT_EQ(call({1, 2}, {2, 1}), 1); }
TEST("start at zero, tank hits zero exactly") { EXPECT_EQ(call({3, 1, 1}, {1, 2, 2}), 0); }
TEST("total gas too small") { EXPECT_EQ(call({2, 3, 4}, {3, 4, 3}), -1); }
TEST("only the last station works") { EXPECT_EQ(call({0, 0, 10}, {3, 3, 1}), 2); }
// 在這裡加你自己的邊界測資：

// ---- oracle：每個起點都模擬一圈 ----
static vector<int> valid_starts(const vector<int>& gas, const vector<int>& cost) {
    vector<int> out;
    int n = static_cast<int>(gas.size());
    for (int s = 0; s < n; ++s) {
        long long tank = 0;
        bool ok = true;
        for (int step = 0; step < n && ok; ++step) {
            int i = (s + step) % n;
            tank += gas[static_cast<size_t>(i)] - cost[static_cast<size_t>(i)];
            ok = tank >= 0;
        }
        if (ok) out.push_back(s);
    }
    return out;
}

STRESS("random stations vs simulation", 500) {
    vector<int> gas, cost, starts;
    do {  // 題目保證答案唯一，所以丟掉有兩個以上起點的輸入
        int n = rng.range(1, size * 2);
        gas = rng.vec(n, 0, 5);
        cost = rng.vec(n, 0, 5);
        starts = valid_starts(gas, cost);
    } while (starts.size() > 1);
    CHECK_SAME(call(gas, cost), starts.empty() ? -1 : starts[0], gas, cost);
}

TEST("perf n=100000, only the last station works (simulating every start is too slow)") {
    const int n = 100000;
    vector<int> gas(n, 1), cost(n, 1);
    gas[n - 1] = 2;  // the only surplus
    cost[n - 2] = 2; // the only deficit: every other start reaches it with an empty tank

    int got = 0;
    double ms = judge::time_ms([&] { got = call(gas, cost); });
    EXPECT_UNDER_MS(ms, 200);
    EXPECT_EQ(got, n - 1);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
