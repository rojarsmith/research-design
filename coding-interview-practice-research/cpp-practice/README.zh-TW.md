[English](README.md) | **繁體中文**

# C++ 刷題練習專案（VS Code、Windows 11）

這個專案把 [研究報告](../README.zh-TW.md) 的建議做成可以每天照著走的練習流程。

- **74 題，分成 10 週**：研究報告觸發條件對照表的 30 種模式都有題目，其中 24 種有 2 題以上，常考的模式各有好幾題；其中 4 題是要自己讀 STDIN 的 HackerRank 題。
- **每題都附自己出的測資**：範例與邊界、暴力 oracle 對拍；74 題中有 60 題另有上限大小的效能測試。
- **`pp` 練習工具**：用 C++ 寫成，負責計時、分層提示、強制寫卡片、間隔複習排程、交錯抽題和弱項統計，也能做計時的模擬面試，並依研究報告的標準回報準備度。

**只需要裝三樣東西**：VS Code、C/C++ 擴充、WinLibs GCC。不需要 Python、CMake、make 或 PowerShell 腳本。

## 1. 先讀這段：大幅提升學習效率的要點

依效果大小排列。左欄是研究報告的建議，右欄是這個專案怎麼逼你做到。

| # | 要點 | 為什麼有效 | 專案怎麼幫你 |
| --- | --- | --- | --- |
| 1 | **從空白檔重寫才算複習** | 檢索練習（測驗效應）是學習科學裡效益最高的方法。重讀舊程式只會讓你**以為**自己會了。 | `pp start` 會封存上一版，還原成空白的 stub；VS Code 的檔案總管和搜尋都隱藏了 `progress/history` |
| 2 | **先想再寫** | 由限制推出目標複雜度、講出暴力解、找出瓶頸，比直接寫程式更快找到正解 | solution 檔頂端的卡片欄位：`[限制→複雜度]`、`[暴力解]`、`[瓶頸]`、`[觸發]` |
| 3 | **用最小反例除錯，不要猜** | 手算一個 3 個元素的反例，比盯著程式看有效得多 | `STRESS` 對拍時輸入由小到大，失敗時印出**最小**反例和 seed |
| 4 | **卡住有時間上限** | 先掙扎再學有效，但無止境地卡住沒有用 | 測試時超過 20 分鐘會提醒你；`pp hint` 一次給一層（模式、引導問題、你上次的卡片、關鍵洞見） |
| 5 | **誠實記錄，依結果排程** | 間隔重複要靠準確的回饋才排得準 | `pp done pass、hint 或 solution`。用過提示的這次會自動改記為 hint。間隔為 1 → 3 → 7 → 14 → 30 天；用了提示減半；看了解答就隔天重做。 |
| 6 | **卡片記洞見，不記程式碼** | 複習時要回想的是「看到什麼就想到這招」，不是程式碼 | `pp done` 會檢查 `[觸發]` 和 `[關鍵洞見]` 有沒有填，沒填不給記錄 |
| 7 | **效能測試告訴你複雜度錯了** | 逾時通常是演算法錯了，不是常數太大 | 74 題中有 60 題附效能測試，用題目上限大小的輸入。O(n²) 的寫法會明確失敗，並提示「複雜度不對」。 |
| 8 | **先分塊，再交錯** | 交錯練習訓練的是「選哪一招」，也就是面試真正考的能力 | 題目依週排列。學完一個模式後，用 `pp mix` 隨機抽做過的題，並且隱藏模式（`--blind`）。 |
| 9 | **每天固定量，先複習再做新題** | 間隔複習只要積壓，就會整個崩掉 | `pp today` 給你 2 題到期複習加 1 題新題；到期超過 3 題時不給新題 |
| 10 | **自己先加測資** | 想出邊界測資本身就是解題的一部分，HackerRank 的隱藏測資考的正是這個 | 每個 `test.cpp` 都留了「在這裡加你自己的邊界測資」的位置，`pp test` 通過後也會提醒你 |
| 11 | **練習環境接近考場** | 監考模式沒有 AI 補全，也不能複製貼上；有些測驗要自己讀 STDIN | 這個工作區關掉了 AI 行內補全；4 題 HackerRank 題要自己解析輸入，時限 2 秒，其中 1 題一次給多組測資 |
| 12 | **讀解答之後一定要重寫** | 看懂和寫得出來是兩回事 | 記為 solution 之後，下次一定會排到隔天，並提醒你用 `pp start` 從空白檔重寫 |
| 13 | **模擬面試要計時，看多場的穩定度** | 研究報告第 4.2 節的標準：沒見過的 Medium 大多能在 20 到 25 分鐘內解出，而且連續幾場模擬面試都通過。第 9 節建議至少 5 次，最好錄影檢討。 | `pp mock`：一個時鐘、一次給一題、模式和難度隱藏、沒有提示，填完評分表才記錄。`pp ready` 依報告的標準回報準備度。 |

C++ 本身的要點，例如容器怎麼選、溢位、迭代器失效、HackerRank 的輸入輸出範本，整理在 [docs/cpp-points.zh-TW.md](docs/cpp-points.zh-TW.md)。每題 `pp done` 之後也會顯示該題的 C++ 要點。

## 2. 安裝

1. 安裝 [VS Code](https://code.visualstudio.com/)。
2. 用 Windows 11 內建的 winget 安裝 WinLibs GCC（g++ 16.2.0 加 gdb）：

   ```bash
   winget install --id BrechtSanders.WinLibs.POSIX.UCRT --exact
   ```

   為什麼選這個版本，以及下載頁上那麼多版本的差別，見 [docs/toolchain.zh-TW.md](docs/toolchain.zh-TW.md)。
3. **重新開啟 VS Code**，讓新的 PATH 生效。接著用「檔案 → 開啟資料夾」打開 `cpp-practice` 這個資料夾（不是整個 repo）。
4. VS Code 會提示安裝建議的擴充 **C/C++**（`ms-vscode.cpptools`），裝這一個就好。

第一次執行任何 `pp:` 工作時，會自動建置 `build/pp.exe`，約 5 秒。第一次測試時會預先編譯標頭，約 10 秒；之後每次修改再測試約 2 到 3 秒。

## 3. 每天的流程

在 VS Code 裡，工作是從「終端機 → 執行工作」選。終端機欄是同樣的指令，可以在 VS Code 的終端機輸入。

| 步驟 | VS Code | 終端機 |
| --- | --- | --- |
| 1. 看今天要做什麼 | 工作 `pp: 今日練習` | `.\build\pp.exe today` |
| 2. 開始一題 | 打開該題資料夾裡的任一個檔案（例如 `stub.hpp`），執行工作 `pp: 開始/重做這題` | `.\build\pp.exe start 739` |
| 3. 先填卡片的前四欄，再寫程式 | 編輯 `solution.hpp`。這個檔案可以原封不動貼到 LeetCode 送出。 | |
| 4. 測試 | **Ctrl+Shift+B** | `.\build\pp.exe test 739` |
| 5. 程式崩潰或要逐步追蹤 | **F5**，選「除錯這題」 | |
| 6. 卡住超過 15 到 20 分鐘 | 工作 `pp: 提示` | `.\build\pp.exe hint 739` |
| 7. 通過後，填 `[觸發]` 和 `[關鍵洞見]`，再記錄結果 | 工作 `pp: 記錄結果`，選 pass、hint 或 solution | `.\build\pp.exe done 739 pass` |

每週再做這幾件事：

- `pp: 交錯抽題`：抽做過的題，並且隱藏模式。
- `pp: 弱項統計`：依模式列出弱項，弱的模式用 `pp new` 多加幾題。
- 第 5 週起，`pp: 模擬面試`：每週一場，見下一小節。一週以上沒做，`pp today` 會提醒你。

`<題>` 可以是題號（`739`）、完整的 id，或題目資料夾裡任一個檔案的路徑。

### 模擬面試（第 5 週起每週一場，第 11 到 12 週加量）

研究報告第 4.2 節對「準備好了」的標準是：沒見過的 Medium 大多能在 20 到 25 分鐘內解出，而且連續幾場模擬面試都通過，不是只過一場。第 9 節建議至少做 5 次，最好錄影、事後檢討。`pp mock` 把一場模擬面試做成有時鐘的練習：

| 步驟 | VS Code | 終端機 |
| --- | --- | --- |
| 1. 開始，面板上倒數 | 工作 `pp: 模擬面試` | `.\build\pp.exe mock`；要倒數就另開一個終端機執行 `.\build\pp.exe mock clock` |
| 2. 解題、測試 | 編輯打開的 solution 檔，**Ctrl+Shift+B** | `.\build\pp.exe test <題>` |
| 3. 換下一題 | 工作 `pp: 模擬面試下一題` | `.\build\pp.exe mock next` |
| 4. 停止計時，打開檢討表 | 工作 `pp: 結束模擬面試` | `.\build\pp.exe mock end` |
| 5. 填好檢討表後記錄 | 再執行一次 `pp: 結束模擬面試` | `.\build\pp.exe mock end` |
| 6. 看準備度 | 工作 `pp: 準備度` | `.\build\pp.exe ready` |

進行中隨時可以用 `.\build\pp.exe mock status` 看狀態；再執行一次 `pp: 模擬面試` 也會顯示狀態並接著倒數。

**一場怎麼進行。**

- 預設 2 題 45 分鐘，整場一個時鐘。題數和時間可以改，例如 `pp mock 3 60`（1 到 4 題，5 到 180 分鐘）。
- 一次給一題，像面試官出下一題。每題的用時從出題那一刻算起。模式和難度都不顯示，和 `pp mix` 一樣要自己判斷。
- 抽題順序：先抽沒做過、但模式已經練過的題（`pp new` 加的第 11 週以後的題也算），其次抽一週以上沒碰過的題，越久沒碰越容易抽到。同一場的模式不重複。第一題一定是 Medium；其他題的難度看每題分到的時間：不到 15 分鐘只出 Easy，30 分鐘以上才可能出 Hard。出題順序由易到難。
- 每題都從空白檔開始，和 `pp start` 一樣先封存舊版。
- 沒有提示：`pp hint` 停用。進行中也不能 `pp start` 別的題，這幾題也不能用 `pp done` 記錄。
- 測試照常跑。時間內最後一次測試的結果，就是這題的結果。時間到之前按下的測試算數，編譯晚幾秒結束沒關係。換到下一題之後，前一題的結果就固定了。
- 練習時「20 分鐘就看提示」的規則，在這裡換成依這場時間的提醒。這些提醒出現在 `pp test`、`pp mock status` 和倒數面板上：
  - 一題用完平均每題的時間還沒通過（45 分鐘 2 題就是 22 分鐘）：先寫出正確的暴力解拿部分分數，講清楚卡在哪；也可以換下一題。
  - 剩 5 分鐘：不要再大改，跑一次極端測資，口頭講完還差什麼（研究報告第 8 節）。
  - 時間到：之後的通過不算，執行 `pp mock end`。

**檢討表。** 第一次 `pp mock end` 會停止計時，產生 `progress/mocks/<場次>.txt`。面試官評分看的是下面五項。每一項在標籤後面寫 1 到 4 分，再寫一句證據，例如 `[測試] 2 只跑了範例，沒有自己出空陣列的測資`：

| 標籤 | 看什麼 |
| --- | --- |
| `[澄清]` | 動手前有沒有用自己的話複述題目，問輸入範圍和邊界 |
| `[思路]` | 寫程式前有沒有先講暴力解和複雜度，再講怎麼優化、為什麼選這個模式 |
| `[正確]` | 程式是不是一次就寫得差不多對、結構清楚，而不是靠反覆跑測試改到通過 |
| `[測試]` | 有沒有自己出邊界測資、手動走一遍程式，在跑測試之前就找到 bug |
| `[溝通]` | 有沒有邊寫邊講，卡住時有沒有說出卡在哪 |

- 分數：4 是做得好，3 是合格，2 是有缺失，1 是沒做到。全形數字也可以。
- `[下次改進]` 必填，寫下一場最該改的一件事。下一場開始時，`pp mock` 會再顯示給你看。
- `[夥伴或錄影]` 選填，寫模擬面試夥伴，或錄影檔的位置。
- 和 `pp done` 一樣，沒填完就不記錄。
- 自己一個人練時也要出聲講。最好錄影或錄音，重看一次再打分數。

**記錄與複習排程。** 填完之後再執行一次 `pp mock end`：

- `progress/mocks.tsv` 每題記一行：結果、用時、評分表。
- 一場算**通過**的條件：時間內每題的測試都通過，而且評分表沒有任何一項是 1 分。
- 結果也排進 `reviews.tsv` 的複習，第 7 欄標 `mock`。通過的題目和 `pp done pass` 一樣往後排。
- 沒通過的題目記為 `fail`，**當天到期**，`pp today` 和 `pp due` 會列出來。先不計時把它做到通過（這時可以用 `pp hint`），再用 `pp done` 記錄。
- 被打斷的場次用 `pp mock abort` 中止，不記錄，也不影響排程。

**準備度。** `pp ready` 依研究報告的標準檢查三項；`pp stats` 的最後一行也會顯示摘要。

| 項目 | 門檻 | 依據 |
| --- | --- | --- |
| 場數 | 至少 5 場 | 第 1 節：做過 5 次以上模擬面試的人，通過 Facebook 面試的機率約為 2 倍（interviewing.io）。第 6.2 節第 9 到 12 週和第 9 節都建議至少 5 次。 |
| 穩定度 | 最近連續通過至少 3 場 | 第 4.2 節：連續幾場都通過，不是只過一場，只有約 20% 到 25% 的人每場表現穩定。第 9 節：看多場的穩定度。報告沒有說幾場，這裡取 3。 |
| Medium 速度 | 最近 5 場的 Medium，至少七成在 25 分鐘內通過 | 第 4.2 節：沒見過的 Medium 大多能在 20 到 25 分鐘內解出（NeetCode 的基準）。「大多」取七成，時間取上限。 |

`pp ready` 也會列出：

- 最近 5 場的結果。
- 評分表各項的平均，以及低於 3 分的最弱項目。
- 有夥伴或錄影的場次。
- Medium 速度裡「這場之前沒見過的題」占多少。做過的題會比較快，這一項要靠沒見過的題才準。所以第 11 到 12 週用 `pp new` 加目標公司的標籤題或新題單的題，再拿來做模擬面試。

**測試 `pp` 本身時**，可以設環境變數 `PP_MOCK_MINUTE`（1 到 60），讓模擬面試的一分鐘只有那麼多秒。例如 `PP_MOCK_MINUTE=2` 時，45 分鐘的一場 90 秒就結束。這個值在 `pp mock` 開始時記進這一場，之後的指令不必再設。練習時不要用。

## 4. 專案結構

```
cpp-practice/
├─ .vscode/                 工作、除錯、IntelliSense 設定；關閉 AI 補全，隱藏封存的舊版
├─ include/
│  ├─ leetcode.hpp          模擬 LeetCode 環境：標準標頭、using namespace std、ListNode、TreeNode
│  └─ judge.hpp             測試框架：TEST、STRESS 對拍、EXPECT_UNDER_MS 效能測試
├─ tools/pp.cpp             練習迴圈工具（第一次執行工作時自動編譯）
├─ problems/
│  ├─ catalog.tsv           題目清單：模式、週次、引導問題、關鍵洞見、C++ 要點
│  ├─ lc0739-daily-temperatures/
│  │  ├─ stub.hpp           空白框架（不要改；pp start 會從這裡還原）
│  │  ├─ solution.hpp       你的解答（pp start 產生，不進版本控制）
│  │  └─ test.cpp           範例與邊界、oracle 對拍、效能測試
│  └─ hr-array-manipulation/
│     ├─ stub.cpp、solution.cpp  完整程式，自己讀 STDIN
│     ├─ tests/*.in、*.out  自己出的測資
│     └─ gen.cpp            產生大型測資 tests/big.in（第一次測試時自動執行）
├─ templates/               pp new 用的新題範本
├─ docs/                    C++ 學習要點、工具鏈說明
├─ progress/                你的練習紀錄（自動建立，不進版本控制）
│  ├─ reviews.tsv           每次的結果與下次複習日期
│  ├─ mocks.tsv             模擬面試的紀錄，每題一行（pp ready 讀這個檔）
│  ├─ mocks/                每場模擬面試的檢討表；進行中的場次在 active.txt
│  └─ history/              每次重做前封存的舊版
└─ build/                   建置產物（隨時可以整個刪掉）
```

## 5. 題目清單

74 題分成 10 週，每週 6 到 9 題，平均每天 1 題新題。題目大致依研究報告第 6.2 節的主題順序排列，`pp today` 會照這個順序給新題。第 6.2 節的 8 週主題表在這裡拉長成 10 週：

- 第 1 到 3 週：陣列、雜湊、雙指針、滑動視窗、前綴和、矩陣、二分搜尋、堆疊。
- 第 3 到 5 週：鏈結串列、堆積、樹、Trie。
- 第 6 週：回溯和網格搜尋。第 7 週：圖。第 8 週：動態規劃。
- 第 9 週：區間、貪婪、單調佇列。第 10 週：HackerRank 計時練習、位元運算、設計題。

第 11 到 12 週留給到期複習、交錯練習、公司標籤題和模擬面試（`pp mock`，見第 3 節）。

| 週 | 題目 | 模式 | 難度 |
| --- | --- | --- | --- |
| 1 | LC 1 Two Sum | 雜湊表 | E |
| 1 | LC 11 Container With Most Water | 雙指針 | M |
| 1 | LC 167 Two Sum II - Input Array Is Sorted | 雙指針 | M |
| 1 | LC 15 3Sum | 雙指針（排序後去重） | M |
| 1 | LC 3 Longest Substring Without Repeating Characters | 滑動視窗 | M |
| 1 | LC 424 Longest Repeating Character Replacement | 滑動視窗 | M |
| 1 | LC 76 Minimum Window Substring | 滑動視窗（最短覆蓋） | H |
| 2 | LC 560 Subarray Sum Equals K | 前綴和加雜湊 | M |
| 2 | LC 974 Subarray Sums Divisible by K | 前綴和加雜湊（同餘） | M |
| 2 | LC 128 Longest Consecutive Sequence | 雜湊表 | M |
| 2 | LC 49 Group Anagrams | 雜湊表（分組鍵） | M |
| 2 | LC 48 Rotate Image | 矩陣操作 | M |
| 2 | LC 54 Spiral Matrix | 矩陣模擬 | M |
| 2 | LC 875 Koko Eating Bananas | 二分搜尋（對答案二分） | M |
| 3 | LC 33 Search in Rotated Sorted Array | 二分搜尋（旋轉陣列） | M |
| 3 | LC 153 Find Minimum in Rotated Sorted Array | 二分搜尋（旋轉陣列） | M |
| 3 | LC 20 Valid Parentheses | 堆疊（括號配對） | E |
| 3 | LC 739 Daily Temperatures | 單調堆疊 | M |
| 3 | LC 84 Largest Rectangle in Histogram | 單調堆疊 | H |
| 3 | LC 206 Reverse Linked List | 鏈結串列操作 | E |
| 3 | LC 21 Merge Two Sorted Lists | 鏈結串列操作 | E |
| 4 | LC 141 Linked List Cycle | 快慢指針 | E |
| 4 | LC 142 Linked List Cycle II | 快慢指針 | M |
| 4 | LC 876 Middle of the Linked List | 快慢指針 | E |
| 4 | LC 215 Kth Largest Element in an Array | 堆積 | M |
| 4 | LC 347 Top K Frequent Elements | 堆積（Top K） | M |
| 4 | LC 23 Merge k Sorted Lists | 堆積（多路合併） | H |
| 4 | LC 102 Binary Tree Level Order Traversal | 樹 BFS（層序） | M |
| 5 | LC 98 Validate Binary Search Tree | 樹 DFS（上下界） | M |
| 5 | LC 230 Kth Smallest Element in a BST | BST 中序走訪 | M |
| 5 | LC 236 Lowest Common Ancestor of a Binary Tree | 樹 DFS（後序回傳） | M |
| 5 | LC 124 Binary Tree Maximum Path Sum | 樹 DFS | H |
| 5 | LC 297 Serialize and Deserialize Binary Tree | 樹（序列化） | H |
| 5 | LC 208 Implement Trie (Prefix Tree) | Trie | M |
| 5 | LC 211 Design Add and Search Words Data Structure | Trie（萬用字元 DFS） | M |
| 5 | LC 295 Find Median from Data Stream | 雙堆積 | H |
| 6 | LC 78 Subsets | 回溯（子集） | M |
| 6 | LC 46 Permutations | 回溯（排列） | M |
| 6 | LC 39 Combination Sum | 回溯 | M |
| 6 | LC 79 Word Search | 回溯（網格 DFS） | M |
| 6 | LC 200 Number of Islands | 網格 BFS/DFS（連通塊） | M |
| 6 | LC 994 Rotting Oranges | BFS | M |
| 6 | LC 1091 Shortest Path in Binary Matrix | BFS（網格最短路徑） | M |
| 6 | LC 417 Pacific Atlantic Water Flow | 網格 BFS（從邊界反向搜尋） | M |
| 7 | LC 785 Is Graph Bipartite? | 圖著色（二分圖） | M |
| 7 | LC 207 Course Schedule | 拓撲排序（DFS 三色判環） | M |
| 7 | LC 210 Course Schedule II | 拓撲排序 | M |
| 7 | LC 547 Number of Provinces | 併查集（連通分量） | M |
| 7 | LC 684 Redundant Connection | 併查集（Union-Find） | M |
| 7 | LC 743 Network Delay Time | Dijkstra | M |
| 7 | LC 778 Swim in Rising Water | Dijkstra（最小化路徑上的最大值） | H |
| 7 | LC 787 Cheapest Flights Within K Stops | 限制步數的最短路（Bellman-Ford） | M |
| 7 | LC 1584 Min Cost to Connect All Points | 最小生成樹（Prim） | M |
| 8 | LC 198 House Robber | 動態規劃（1D） | M |
| 8 | LC 300 Longest Increasing Subsequence | 動態規劃加二分搜尋 | M |
| 8 | LC 322 Coin Change | 動態規劃 | M |
| 8 | LC 139 Word Break | 動態規劃（字串切分） | M |
| 8 | LC 62 Unique Paths | 動態規劃（2D 網格） | M |
| 8 | LC 1143 Longest Common Subsequence | 動態規劃（兩字串） | M |
| 8 | LC 72 Edit Distance | 動態規劃（兩字串） | M |
| 9 | LC 56 Merge Intervals | 區間排序掃描 | M |
| 9 | LC 435 Non-overlapping Intervals | 區間排序（依終點貪婪） | M |
| 9 | LC 134 Gas Station | 貪婪 | M |
| 9 | LC 55 Jump Game | 貪婪 | M |
| 9 | LC 239 Sliding Window Maximum | 單調佇列 | H |
| 9 | LC 862 Shortest Subarray with Sum at Least K | 單調佇列（前綴和） | H |
| 10 | HackerRank Count Triplets | 雜湊計數，左右兩側（STDIN） | M |
| 10 | HackerRank Array Manipulation | 差分陣列（STDIN） | H |
| 10 | HackerRank Sherlock and the Valid String | 雜湊計數，次數的分布（STDIN） | M |
| 10 | HackerRank Roads and Libraries | 連通分量，多組測資（STDIN） | M |
| 10 | LC 338 Counting Bits | 位元運算 | E |
| 10 | LC 136 Single Number | 位元運算（XOR） | E |
| 10 | LC 191 Number of 1 Bits | 位元運算 | E |
| 10 | LC 146 LRU Cache | 設計：雜湊表加雙向鏈結串列 | M |

**覆蓋範圍。** 這 74 題和 Blind 75 重疊 42 題（56%）。研究報告第 4.4 節觸發條件對照表的 30 種模式都至少有 1 題，其中 24 種有 2 題以上。只有 1 題的 6 種是比較專門的模式：從邊界反向的多源 BFS、二分圖著色、Bellman-Ford、最小生成樹、雙堆積和 LRU 設計題。常考的模式題數較多：雙指針 3 題、滑動視窗 3 題、二分搜尋 3 題、堆疊 3 題、鏈結串列 6 題、樹 6 題、回溯 4 題、圖 14 題、動態規劃 7 題。

**怎麼選的。** 第 4.4 節對照表列為代表題、但前 49 題還沒有的題目，這一批都補上了，只有 3 題例外：LC 253 Meeting Rooms II 是 Premium 題；LC 70 和 LC 104 所屬的模式（1D 動態規劃、樹）已經有 4 題以上。另外補了 Blind 75 的 LC 230（BST 原本只有 1 題），以及對照表 HackerRank 欄的 Sherlock and the Valid String 和 Roads and Libraries。

74 題仍然是骨架。研究報告建議的題量是 150 到 250 題，例如 Grind 169 或 NeetCode 150。做完這 74 題後，先用 `pp stats` 找出弱項，再用 `pp new` 補同一模式的題目。

## 6. 測試怎麼運作

**每個 `test.cpp` 有四段（不需要分辨複雜度的 14 題沒有第 4 段）：**

1. 範例與邊界：手算的小例子。
2. oracle：刻意寫成最直接的暴力解，**動手之前不要讀它**。
3. `STRESS` 對拍：隨機產生由小到大的輸入，比對你的答案和 oracle，失敗時印出最小反例。
4. 效能測試：題目上限大小的輸入，O(n²) 的寫法會明確失敗。題目上限太小、分不出複雜度時，輸入會刻意放大，測試名稱裡會註明（例如 above LeetCode's 10^4）。

**時間上限：** 單一測試跑超過 10 秒，程式會被終止並顯示 TIMEOUT，`pp` 會提示「多半是指數時間或無窮迴圈」。例如沒有記憶化的遞迴，就會停在這裡，而不是一直卡住。輸入是 10⁵ 的效能測試，O(n²) 的寫法也可能跑到 10 秒而停在這裡。

**兩種建置模式：**

- 測試版：`-O2 -D_GLIBCXX_ASSERTIONS`。會檢查越界，也會檢查時限和 10 秒的時間上限。
- 除錯版（F5）：`-O0 -g -D_GLIBCXX_DEBUG`。檢查更完整，但不檢查時限，也沒有時間上限，停在中斷點不會被終止。效能測試（名稱以 `perf` 開頭）在除錯版預設跳過，顯示為 `[ SKIP ]`：反正不檢查時限，而且 `_GLIBCXX_DEBUG` 每次 `priority_queue` push、pop 都會檢查整個 heap，大型輸入會變成 O(n²)，跑上好幾分鐘。真的要在除錯版追蹤效能測試，在 `.vscode/launch.json` 的 `args` 填 `["perf"]`。

**常用的測試參數：**

- 只跑部分測試：`pp test 739 random`。
- 重現一次對拍失敗：`pp test 739 random --seed=失敗訊息裡的數字`。
- 調整時間上限：`pp test 322 --timeout=3`。

**HackerRank 題：**

- 逐一執行 `tests/*.in`，比對 `*.out`，每個測資時限 2 秒（含程式啟動時間）。
- `big.in` 由 `gen.cpp` 產生，同時檢查溢位和速度。
- 判定結果有 AC、WA、TLE、RE；只有輸入、沒有標準答案的測資只檢查時間。
- Roads and Libraries 一次給多組測資，考的是每組之間有沒有重設狀態。
- 這 4 題是完整程式，沒有 10 秒的時間上限。程式陷入無窮迴圈時，在終端機按 Ctrl+C 中止。

## 7. 加新題

```bash
.\build\pp.exe new lc0438-find-all-anagrams-in-a-string fn 滑動視窗 https://leetcode.com/problems/find-all-anagrams-in-a-string/ M 1
```

最後兩個參數是難度和週次。沒有指定週次時，新題排在第 11 週，也就是 10 週題單之後。

建立之後要做三件事：

1. 把 LeetCode 的函式框架貼進 `stub.hpp`。
2. 在 `test.cpp` 寫自己的測資和暴力 oracle。寫 oracle 本身就是在練「先講出暴力解」。
3. 回 `problems/catalog.tsv` 補上引導問題、關鍵洞見和 C++ 要點。

HackerRank 類的題目把 `fn` 換成 `io`。

## 8. 疑難排解

| 症狀 | 處理 |
| --- | --- |
| 找不到 `g++` | 裝完 WinLibs 後要重開 VS Code。在終端機執行 `where g++` 確認。 |
| IntelliSense 滿是紅線 | 確認已安裝 C/C++ 擴充。如果還是不行，把 `.vscode/c_cpp_properties.json` 的 `compilerPath` 改成 `where g++` 顯示的完整路徑。 |
| 中文變亂碼 | `pp` 已經把主控台切換成 UTF-8。如果還是亂碼，先在終端機執行 `chcp 65001`。 |
| 結束碼 `-1073741819` 或 `-1073741571` | 分別是存取違規與堆疊溢位。按 F5 用除錯版重跑，會停在出錯的那一行。 |
| 顯示 TIMEOUT，結束碼 3 | 某個測試超過 10 秒還沒跑完，多半是指數時間或無窮迴圈。先想複雜度；要追蹤就按 F5，除錯版沒有時間上限。卡住的是效能測試時，在 `launch.json` 的 `args` 填 `["perf"]`，否則除錯版會跳過它。 |
| gdb 顯示 `No module named 'libstdcxx'`，容器印成一長串內部欄位 | 這是從 Git Bash 啟動 gdb 時的環境問題。改用 VS Code 預設的 PowerShell 終端機或按 F5，STL 容器就會正常顯示成 `{1, 1}` 這樣的格式。 |
| 改了 `tools/pp.cpp` | 執行工作 `pp: 重新建置工具` |
| 模擬面試中途被打斷，或關掉了倒數面板 | 倒數面板關掉不影響計時。`pp mock status` 看狀態；`pp mock end` 照常結束，超過時間就以時間上限計；不想記錄這場就 `pp mock abort`。 |
| 模擬面試中 `pp start`、`pp hint` 被拒絕 | 這是刻意的：面試中沒有提示，也不換做別的題。先 `pp mock end` 寫完檢討，或 `pp mock abort` 中止。 |
| `build/` 太大 | 預先編譯標頭每個約 130 MB，可以整個刪掉，下次會自動重建 |
| Linux 或 macOS | 一樣用 g++，測試版會多開 AddressSanitizer。macOS 除錯要把 `MIMode` 改成 `lldb`。 |

已在 Windows 11 上用 WinLibs g++ 16.2.0 驗證過兩件事：74 題的空白 stub 都不會通過測試；參考解答都通過全部測試，包含效能測試。參考解答沒有放進專案。除錯版（F5）下，70 題函式題用參考解答都通過，效能測試跳過，每題 1 秒內跑完。

`pp mock` 和 `pp ready` 也在同一個環境的終端機上跑過完整流程，用 `PP_MOCK_MINUTE` 縮短時鐘：

- 通過和沒通過的場次都跑過，也包含 HackerRank 題。
- 檢討表沒填完、分數超出 1 到 4 時拒絕記錄。
- 時間到之後的測試不算；在終端機執行的倒數計時（`pp mock clock`）會在該提醒的時間點提醒。
- 沒通過的題目當天到期。
- 連續通過 3 場、做滿 5 場後，`pp ready` 顯示達標。
- 在改版前產生的練習紀錄上，`today`、`start`、`test`、`hint`、`done`、`debug`、`due`、`list`、`stats` 的輸出都和改版前相同，只有 `pp today` 多了一行模擬面試的提醒。改版前的 `pp` 也讀得了加上模擬面試紀錄之後的檔案。

VS Code 工作和倒數面板沒有在 VS Code 裡實際操作過。
