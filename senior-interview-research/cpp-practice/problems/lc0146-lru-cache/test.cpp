// 測試：lc0146-lru-cache。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0146-lru-cache（VS Code 工作：pp: 開始/重做這題）"
#endif

TEST("eviction order follows use, not insertion") {
    LRUCache c(2);
    c.put(1, 10);
    c.put(2, 20);
    EXPECT_EQ(c.get(1), 10);
    c.put(3, 30);
    EXPECT_EQ(c.get(2), -1);
    EXPECT_EQ(c.get(3), 30);
    c.put(1, 11);
    c.put(4, 40);
    EXPECT_EQ(c.get(3), -1);
    EXPECT_EQ(c.get(1), 11);
    EXPECT_EQ(c.get(4), 40);
}
TEST("updating a key does not grow the cache") {
    LRUCache c(1);
    c.put(1, 1);
    c.put(1, 2);
    EXPECT_EQ(c.get(1), 2);
    c.put(2, 3);
    EXPECT_EQ(c.get(1), -1);
    EXPECT_EQ(c.get(2), 3);
}
TEST("missing key") {
    LRUCache c(3);
    EXPECT_EQ(c.get(7), -1);
}
// 在這裡加你自己的邊界測資：

// ---- oracle：vector 依使用順序排列（前面最舊），每次操作 O(n) ----
struct OracleLRU {
    int cap;
    vector<pair<int, int>> items;
    int get(int key) {
        for (size_t i = 0; i < items.size(); ++i)
            if (items[i].first == key) {
                auto kv = items[i];
                items.erase(items.begin() + static_cast<long>(i));
                items.push_back(kv);
                return kv.second;
            }
        return -1;
    }
    void put(int key, int value) {
        if (get(key) != -1) { items.back().second = value; return; }
        if (static_cast<int>(items.size()) == cap) items.erase(items.begin());
        items.push_back({key, value});
    }
};

STRESS("random operations vs oracle", 400) {
    int cap = rng.range(1, 3);
    LRUCache c(cap);
    OracleLRU o{cap, {}};
    for (int step = 0; step < size * 6; ++step) {
        int key = rng.range(0, 4);
        if (rng.coin()) {
            CHECK_SAME(c.get(key), o.get(key), step, string("get"), key, cap);
        } else {
            int value = rng.range(0, 99);
            c.put(key, value);
            o.put(key, value);
        }
    }
}

TEST("perf capacity 50000, 200000 operations (O(n) per operation is too slow)") {
    const int cap = 50000;
    long long hits = 0, misses = 0;
    double ms = judge::time_ms([&] {
        LRUCache c(cap);
        for (int i = 0; i < cap; ++i) c.put(i, i);
        for (int i = 0; i < cap; ++i) hits += c.get(i);
        for (int i = cap; i < 2 * cap; ++i) c.put(i, i);
        for (int i = 0; i < cap; ++i) misses += c.get(i) == -1;
    });
    EXPECT_UNDER_MS(ms, 400);
    EXPECT_EQ(hits, 1LL * cap * (cap - 1) / 2);
    EXPECT_EQ(misses, 1LL * cap);
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
