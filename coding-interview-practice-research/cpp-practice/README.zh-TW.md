[English](README.md) | **繁體中文**

# C++ 刷題練習專案（VS Code、Windows 11）

這個專案把 [研究報告](../README.zh-TW.md) 的建議做成可以每天照著走的練習流程。

- **24 題**：觸發條件對照表的 22 種模式各 1 題，加上 2 題要自己讀 STDIN 的 HackerRank 題。
- **每題都附自己出的測資**：範例與邊界、暴力 oracle 對拍、上限大小的效能測試。
- **`pp` 練習工具**：用 C++ 寫成，負責計時、分層提示、強制寫卡片、間隔複習排程、交錯抽題和弱項統計。

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
| 7 | **效能測試告訴你複雜度錯了** | 逾時通常是演算法錯了，不是常數太大 | 每題都有一個題目上限大小的效能測試。O(n²) 的寫法會明確失敗，並提示「複雜度不對」。 |
| 8 | **先分塊，再交錯** | 交錯練習訓練的是「選哪一招」，也就是面試真正考的能力 | 題目依週排列。學完一個模式後，用 `pp mix` 隨機抽做過的題，並且隱藏模式（`--blind`）。 |
| 9 | **每天固定量，先複習再做新題** | 間隔複習只要積壓，就會整個崩掉 | `pp today` 給你 2 題到期複習加 1 題新題；到期超過 3 題時不給新題 |
| 10 | **自己先加測資** | 想出邊界測資本身就是解題的一部分，HackerRank 的隱藏測資考的正是這個 | 每個 `test.cpp` 都留了「在這裡加你自己的邊界測資」的位置，`pp test` 通過後也會提醒你 |
| 11 | **練習環境接近考場** | 監考模式沒有 AI 補全，也不能複製貼上；有些測驗要自己讀 STDIN | 這個工作區關掉了 AI 行內補全；2 題 HackerRank 題要自己解析輸入，時限 2 秒 |
| 12 | **讀解答之後一定要重寫** | 看懂和寫得出來是兩回事 | 記為 solution 之後，下次一定會排到隔天，並提醒你用 `pp start` 從空白檔重寫 |

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

每週再做兩件事：

- `pp: 交錯抽題`：抽做過的題，並且隱藏模式。
- `pp: 弱項統計`：依模式列出弱項，弱的模式用 `pp new` 多加幾題。

`<題>` 可以是題號（`739`）、完整的 id，或題目資料夾裡任一個檔案的路徑。

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
│  └─ history/              每次重做前封存的舊版
└─ build/                   建置產物（隨時可以整個刪掉）
```

## 5. 題目清單

依研究報告第 6.2 節的週計畫排列。`pp today` 會照這個順序給新題。

| 週 | 題目 | 模式 | 難度 |
| --- | --- | --- | --- |
| 1 | LC 11 Container With Most Water | 雙指針 | M |
| 1 | LC 3 Longest Substring Without Repeating Characters | 滑動視窗 | M |
| 1 | LC 560 Subarray Sum Equals K | 前綴和加雜湊 | M |
| 1 | LC 128 Longest Consecutive Sequence | 雜湊表 | M |
| 2 | LC 875 Koko Eating Bananas | 二分搜尋（對答案二分） | M |
| 2 | LC 739 Daily Temperatures | 單調堆疊 | M |
| 3 | LC 215 Kth Largest Element in an Array | 堆積 | M |
| 3 | LC 142 Linked List Cycle II | 快慢指針 | M |
| 3 | LC 124 Binary Tree Maximum Path Sum | 樹 DFS | H |
| 4 | LC 39 Combination Sum | 回溯 | M |
| 4 | LC 208 Implement Trie | Trie | M |
| 4 | LC 295 Find Median from Data Stream | 雙堆積 | H |
| 5 | LC 994 Rotting Oranges | BFS | M |
| 5 | LC 210 Course Schedule II | 拓撲排序 | M |
| 5 | LC 684 Redundant Connection | 併查集 | M |
| 6 | LC 743 Network Delay Time | Dijkstra | M |
| 6 | LC 322 Coin Change | 動態規劃 | M |
| 7 | LC 56 Merge Intervals | 區間排序掃描 | M |
| 7 | LC 134 Gas Station | 貪婪 | M |
| 7 | LC 239 Sliding Window Maximum | 單調佇列 | H |
| 7 | HackerRank Count Triplets | 雜湊計數（STDIN） | M |
| 7 | HackerRank Array Manipulation | 差分陣列（STDIN） | H |
| 8 | LC 338 Counting Bits | 位元運算 | E |
| 8 | LC 146 LRU Cache | 雜湊表加雙向鏈結串列 | M |

24 題只是骨架。做完後依研究報告的建議擴充到 150 到 250 題（Grind 169 或 NeetCode 150），用 `pp new` 加題。

## 6. 測試怎麼運作

**每個 `test.cpp` 有四段：**

1. 範例與邊界：手算的小例子。
2. oracle：刻意寫成最直接的暴力解，**動手之前不要讀它**。
3. `STRESS` 對拍：隨機產生由小到大的輸入，比對你的答案和 oracle，失敗時印出最小反例。
4. 效能測試：題目上限大小的輸入，O(n²) 的寫法會明確失敗。

**兩種建置模式：**

- 測試版：`-O2 -D_GLIBCXX_ASSERTIONS`。會檢查越界，也會檢查時限。
- 除錯版（F5）：`-O0 -g -D_GLIBCXX_DEBUG`。檢查更完整，不檢查時限。

**常用的測試參數：**

- 只跑部分測試：`pp test 739 random`。
- 重現一次對拍失敗：`pp test 739 random --seed=失敗訊息裡的數字`。

**HackerRank 題：**

- 逐一執行 `tests/*.in`，比對 `*.out`，每個測資時限 2 秒（含程式啟動時間）。
- `big.in` 由 `gen.cpp` 產生，同時檢查溢位和速度。
- 判定結果有 AC、WA、TLE、RE；只有輸入、沒有標準答案的測資只檢查時間。

## 7. 加新題

```bash
.\build\pp.exe new lc0076-minimum-window-substring fn 滑動視窗 https://leetcode.com/problems/minimum-window-substring/ H 2
```

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
| gdb 顯示 `No module named 'libstdcxx'`，容器印成一長串內部欄位 | 這是從 Git Bash 啟動 gdb 時的環境問題。改用 VS Code 預設的 PowerShell 終端機或按 F5，STL 容器就會正常顯示成 `{1, 1}` 這樣的格式。 |
| 改了 `tools/pp.cpp` | 執行工作 `pp: 重新建置工具` |
| `build/` 太大 | 預先編譯標頭每個約 130 MB，可以整個刪掉，下次會自動重建 |
| Linux 或 macOS | 一樣用 g++，測試版會多開 AddressSanitizer。macOS 除錯要把 `MIMode` 改成 `lldb`。 |

已在 Windows 11 上用 WinLibs g++ 16.2.0 驗證過兩件事：24 題的空白 stub 都不會通過測試；參考解答都通過全部測試，包含效能測試。參考解答沒有放進專案。
