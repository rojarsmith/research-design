// 測試：lc0015-3sum。先別讀下方的 oracle：它刻意寫成暴力解，只用來對拍，不是答案。
#include "leetcode.hpp"
#if __has_include("solution.hpp")
#include "solution.hpp"
#else
#error "還沒開始這題：先執行 pp start lc0015-3sum（VS Code 工作：pp: 開始/重做這題）"
#endif

// 三元組的順序、組內的順序都不限，所以先 normalized（每組排序，再整體排序）再比對。
static vector<vector<int>> call(vector<int> nums) { return normalized(Solution().threeSum(nums)); }

TEST("example") { EXPECT_EQ(call({-1, 0, 1, 2, -1, -4}), {{-1, -1, 2}, {-1, 0, 1}}); }
TEST("no triplet") { EXPECT_EQ(call({0, 1, 1}), {}); }
TEST("three zeros") { EXPECT_EQ(call({0, 0, 0}), {{0, 0, 0}}); }
TEST("four zeros give one triplet") { EXPECT_EQ(call({0, 0, 0, 0}), {{0, 0, 0}}); }
TEST("a repeated value used twice") { EXPECT_EQ(call({-2, 0, 1, 1, 2}), {{-2, 0, 2}, {-2, 1, 1}}); }
TEST("duplicates must not repeat a triplet") { EXPECT_EQ(call({-1, -1, -1, 2, 2}), {{-1, -1, 2}}); }
TEST("many duplicates") {
    EXPECT_EQ(call({-4, -2, -2, -2, 0, 1, 2, 2, 2, 3, 3, 4, 4, 6, 6}),
              {{-4, -2, 6}, {-4, 0, 4}, {-4, 1, 3}, {-4, 2, 2}, {-2, -2, 4}, {-2, 0, 2}});
}
// 在這裡加你自己的邊界測資：

// ---- oracle：三層迴圈列出每一組，用 set 去重（O(n³)） ----
static vector<vector<int>> oracle(const vector<int>& nums) {
    set<vector<int>> found;
    size_t n = nums.size();
    for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j)
            for (size_t k = j + 1; k < n; ++k)
                if (nums[i] + nums[j] + nums[k] == 0) {
                    vector<int> t{nums[i], nums[j], nums[k]};
                    sort(t.begin(), t.end());
                    found.insert(t);
                }
    return vector<vector<int>>(found.begin(), found.end());
}

STRESS("random arrays vs oracle", 500) {
    auto nums = rng.vec(rng.range(3, size * 2 + 3), -4, 4);
    CHECK_SAME(call(nums), oracle(nums), nums);
}

// 效能測試的答案太多，不能手算：用「每種值有幾個」直接數出不重複的三元組有幾組，再逐組檢查。
static size_t count_triplets(const vector<int>& nums) {
    map<int, int> cnt;
    for (int x : nums) ++cnt[x];
    vector<int> u;
    for (auto& [x, c] : cnt) u.push_back(x);
    size_t total = 0;
    for (size_t i = 0; i < u.size(); ++i)
        for (size_t j = i; j < u.size(); ++j) {
            int a = u[i], b = u[j], c = -a - b;
            if (c < b) break;
            if (!cnt.count(c)) continue;
            if (a == b && b == c) total += cnt[a] >= 3;
            else if (a == b) total += cnt[a] >= 2;
            else if (b == c) total += cnt[b] >= 2;
            else total += 1;
        }
    return total;
}

static string check(const vector<int>& nums, const vector<vector<int>>& got) {
    map<int, int> cnt;
    for (int x : nums) ++cnt[x];
    for (size_t i = 0; i < got.size(); ++i) {
        const auto& t = got[i];
        if (t.size() != 3) return "not a triplet: " + judge::show(t);
        if (1LL * t[0] + t[1] + t[2] != 0) return "does not sum to 0: " + judge::show(t);
        if (i > 0 && got[i - 1] == t) return "triplet listed twice: " + judge::show(t);
        map<int, int> need;
        for (int x : t) ++need[x];
        for (auto& [x, c] : need)
            if (cnt[x] < c) return "uses values that are not in nums: " + judge::show(t);
    }
    size_t want = count_triplets(nums);
    if (got.size() != want) return "expected " + to_string(want) + " triplets, got " + to_string(got.size());
    return "";
}

TEST("perf n=3000 random values (O(n^3) is too slow)") {
    judge::Rng r(15);
    auto nums = r.vec(3000, -100000, 100000);
    vector<vector<int>> got;
    double ms = judge::time_ms([&] { got = call(nums); });
    EXPECT_UNDER_MS(ms, 500);
    CHECK_VALID(check(nums, got), nums.size());
}

int main(int argc, char** argv) { return judge::run_all(argc, argv); }
