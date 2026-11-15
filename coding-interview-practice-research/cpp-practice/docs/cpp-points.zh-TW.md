# 刷題用的 C++ 學習要點

這份只收面試與線上測驗真的會用到、而且常出錯的部分。每題做完後 `pp done` 會顯示該題的 C++ 要點，這裡是彙整版。

## 1. 依模式選容器

| 模式 | 容器與寫法 | 要記住的事 |
| --- | --- | --- |
| 雜湊查找與計數 | `unordered_map<K, V>`、`unordered_set<K>` | 平均 O(1)。`m[key]` 在 key 不存在時會**插入**預設值，只想查詢時用 `find`、`count` 或 `contains`（C++20）。可以先 `reserve(n)` 減少 rehash。 |
| 需要排序或找最接近的值 | `map`、`set`、`multiset` | O(log n)。`lower_bound(x)` 找第一個 ≥ x，`upper_bound(x)` 找第一個 > x。`multiset::erase(x)` 會刪掉**全部**的 x，只刪一個要用 `erase(s.find(x))`。 |
| 堆疊 | `vector<int>` 的 `push_back`、`back`、`pop_back` | 比 `std::stack` 方便印出來除錯，也能看底部的元素 |
| 佇列與 BFS | `queue<T>` | 按層處理時，先記下這一層的 `q.size()` |
| 單調佇列、雙端 | `deque<int>` | 兩端的 push 和 pop 都是 O(1) |
| 堆積 | `priority_queue<T>` | 預設是**最大**堆積。最小堆積寫 `priority_queue<int, vector<int>, greater<int>>`。存 `pair` 時先比第一個欄位。 |
| LRU、O(1) 搬移 | `list<T>` 配 `splice` | `splice` 搬節點時 iterator 不會失效，可以把 iterator 存在雜湊表裡 |
| 小字元集計數 | `array<int, 26>`、`array<int, 128>` | 比雜湊表快，也不會碰撞。下標寫 `c - 'a'`；`char` 可能有號，當下標前轉成 `unsigned char`。 |
| 網格 | `vector<vector<int>>` 加方向陣列 | `const int dr[] = {1, -1, 0, 0}, dc[] = {0, 0, 1, -1};` |
| 位元 | `<bit>` 的 `std::popcount`、`std::countr_zero` | 參數要是無號數；`x & (x - 1)` 會去掉最低位的 1 |

## 2. 最常害你 WA 的陷阱

| 陷阱 | 例子 | 怎麼避免 |
| --- | --- | --- |
| 整數溢位 | `int` 只到約 2.1e9；`a * b`、加總、`(a + b) / 2` 都可能溢位 | 用 `long long`，乘之前先轉：`1LL * a * b`；中點寫 `lo + (hi - lo) / 2` |
| Windows 的 `long` 只有 4 bytes | MinGW 和 MSVC 都是 32 位元，Linux 評測機上是 64 位元 | 一律用 `long long` 或 `int64_t` |
| 無號數相減會繞回 | `v.size() - 1` 在 `v` 是空的時候變成很大的數 | 迴圈寫 `i + 1 < v.size()`，或先轉成 `int n = v.size();` |
| `accumulate` 的初值型別 | `accumulate(v.begin(), v.end(), 0)` 用 `int` 加總 | 初值寫 `0LL` |
| 比較函式不是嚴格弱序 | `sort` 的比較函式用 `<=` | 這是未定義行為，可能讓程式崩潰。一定要用 `<`。 |
| 迭代器失效 | 一邊走訪 `vector` 一邊 `push_back`；`erase` 之後還用舊的 iterator | 用 `it = v.erase(it)`；拿著 `auto& x = v.back()` 時不要再 `push_back` |
| 遞迴太深 | 1e5 層的 DFS 在 Windows 預設 1 MB 的堆疊會崩潰 | 本專案已經放大堆疊，但要知道原因；必要時改成迭代加自己的堆疊 |
| 負數的除法與取餘數 | `-7 / 2 == -3`，`-7 % 2 == -1`（往 0 截斷） | 需要數學上的 mod 時寫 `((a % m) + m) % m` |
| `abs(INT_MIN)` | 結果還是負數（溢位） | 先轉成 `long long` |
| 傳值造成大量複製 | 遞迴參數寫 `vector<int> path` | 傳參考 `vector<int>& path`，搭配 `push_back` 和 `pop_back` 回溯 |

## 3. HackerRank 讀寫範本

```cpp
ios::sync_with_stdio(false);  // 讀大量輸入前一定要寫
cin.tie(nullptr);
```

- 讀到檔案結尾：`while (cin >> x) { ... }`
- `cin >> n` 之後接著用 `getline`，會讀到剩下的換行。中間先寫 `cin >> ws`。
- 輸出用 `'\n'`，不要用 `endl`。`endl` 每一行都會沖洗緩衝區，輸出很多行時會慢很多。
- 輸出浮點數：`cout << fixed << setprecision(6) << x;`

## 4. 讓程式更短、更不容易錯的現代寫法

| 寫法 | 例子 |
| --- | --- |
| 結構化綁定 | `auto [r, c] = q.front();`、`for (auto& [key, cnt] : m)` |
| 遞迴 lambda | `auto dfs = [&](auto&& self, int u) -> void { ... self(self, v); };` |
| 初始化 0..n-1 | `iota(p.begin(), p.end(), 0);`（併查集） |
| 多值取最小或最大 | `min({a, b, c})` |
| 反向排序 | `sort(v.rbegin(), v.rend());` |
| 最大公因數 | `std::gcd(a, b)`、`std::lcm(a, b)`（`<numeric>`） |

## 5. 面試時要說出口的

- 為什麼選這個容器，以及每個操作的複雜度。例如：「`unordered_map` 平均 O(1)，最壞 O(n)；需要順序時改用 `map`，O(log n)。」
- 會不會溢位，用了什麼型別，以及理由。
- 遞迴的深度，還有在最壞情況下會不會爆堆疊。
