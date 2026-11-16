[English](README.md) | **繁體中文**

# 高級軟體工程師求職：HackerRank 與 LeetCode 高效率刷題，不靠背答案

研究日期：2026-11-15。

- 兩個參考 repo 的數字取自當日的 shallow clone 與 GitHub API。
- 網路資料取自當日查詢。只來自二手或廠商來源、沒能用原始來源核對的，標「未核實」。

## 1. 結論摘要

**不背答案的做法是換掉練習的對象。** 練的不是「題目 → 程式碼」，而是「觸發條件 → 模式 → 不變量」。複習時從空白編輯器重寫，不重讀舊解答。

- 學習科學裡被評為高效益的只有兩項：練習測驗（檢索練習）和分散練習（Dunlosky 2013）。
- 交錯練習訓練的，正是「看到陌生題，知道該用哪一招」的能力。

**題量比想像中少。** 選一份按模式排序的題單做完：150 到 250 題，以 Medium 為主，Grind 169 或 NeetCode 150/250 都可以。之後再加 Hard 和模擬面試。這樣比散刷 500 題以上有效。

- interviewing.io 的資料：解題總數和面試表現的相關係數只有 0.27。
- 做過 5 次以上模擬面試的人，通過 Facebook 面試的機率約為 2 倍。

**兩個平台分工不同。** LeetCode 用來練內容。HackerRank 用來練雇主考試的格式：

- 計時
- 隱藏測資
- 自己解析 STDIN
- 監考模式
- AI 輔助模式

**高級職位的情況是兩頭都要準備。**

- Big Tech 仍然考 DSA，interviewing.io 的調查裡沒有任何一家取消演算法題。但題型轉向變體和開放式問題，輕度改寫的題目就足以讓背題的人失手。
- 另一邊，Meta、Canva、Shopify、Google 開始有允許或要求使用 AI 的回合，Google 也恢復了現場面試。
- 所以要同時做到「沒有輔助也能在白板上解題」和「會用 AI 在陌生的程式庫裡工作」。背答案在這兩種場合都沒用。
- 對高級工程師，DSA 大約只占準備時間的三成，系統設計才是槓桿最大的部分。

**兩個參考 repo 都適合當解完後的對照材料，不適合當主線。**

- Java-aid/Hackerrank-Solutions：292 個 Java 檔，重心在 30 Days of Code、Java 語言題和基礎程式題。Interview Preparation Kit 只收了 2 題。適合拿來看 HackerRank 的輸入處理和 Java 寫法。
- withaarzoo/LeetCode-Solutions：511 題，5 種語言，416 題有含 Intuition 的完整講解。它是每日一題的紀錄，Blind 75 只涵蓋 7 題，179 題是 3000 號以後的新題。

**執行方式。** 8 到 12 週，每天 60 到 90 分鐘做 DSA，每題走同一套流程（第 4.2 節）。錯題排入 SM-2 或 FSRS 間隔複習。後半段改成交錯、計時練習，並且做模擬面試。

**現成的練習環境。** [cpp-practice/](cpp-practice/README.zh-TW.md) 是可以直接用 VS Code 打開的 C++ 練習專案，把上述流程做成工具（第 10 節）。

## 2. 2025–2026 高級工程師的面試現況

| 觀察 | 內容 | 來源 |
| --- | --- | --- |
| DSA 沒有消失 | interviewing.io 調查 67 位面試官，其中 52 位在 FAANG，沒有人說公司取消了演算法題。58% 改變了題目形式：變體、開放式、較長的情境。只有 21% 刻意把題目變難。 | interviewing.io |
| 改寫題就能防背題 | 同一份調查發現，輕度改寫的 LeetCode 題和原題一樣容易作弊。只有完全自訂的題目擋得住。換句話說，背下的答案碰到變體就失效。 | interviewing.io |
| 門檻上升 | The Pragmatic Engineer（2025-04）指出：形式沒變但更難，Google 出 LeetCode Hard 越來越常見；面試官要求完整、乾淨、有驗證的解答；高級職位被期待具備過去屬於 staff 等級的分散式系統知識；降級錄取（downlevel）常見。 | The Pragmatic Engineer |
| AI 輔助回合：Meta | 2025 年第四季前後開始試行。CoderPad 內建 AI 助手，60 分鐘，在多檔案的程式庫裡修 bug、加功能、擴充。取代兩場現場程式回合中的一場，電話篩選的演算法題保留。「E7 以上只考 AI 回合」這點未核實。 | interviewing.io、Hello Interview |
| AI 輔助回合：Canva | 2025-06 起，後端和前端職位以「AI-Assisted Coding」取代 CS Fundamentals。可以使用 Copilot、Cursor 或 Claude，評估的是拆解模糊問題、判斷 AI 輸出、把 AI 產生的程式除錯到可上線的品質。 | Canva Engineering Blog |
| AI 輔助回合：Shopify、Google | Shopify 的結對程式回合允許並鼓勵使用 AI（未核實，二手來源）。Google 自 2026 下半年起，在美國部分地點的初中階職位試行可使用 Gemini 的程式理解回合（非高級職位）。 | Hello Interview、Yahoo Finance |
| 作弊與回歸現場 | Interview Coder 事件之後，Amazon 要求候選人聲明不使用未授權工具。Google 試行並計畫推廣至少一場現場面試。Meta 要求分享整個螢幕。 | TechCrunch、interviewing.io、Entrepreneur |
| HackerRank 的 AI 面試官 | Chakra 在 2026-10-05 正式上線，上線前已進行約 50 萬場面試。候選人在真實的程式庫裡搭配 AI 工作，Chakra 追問設計取捨和限制改變後的做法，評分對象是過程與「AI 熟練度」。最後仍由人做錄取決定。 | TechCrunch |

**對準備方式的意義。** 純粹的 LeetCode 熟練度仍然是進門條件，但已經不夠。準備要分成三個面向：

1. 無輔助地解演算法題，在白板或純文字編輯器上。
2. 用 AI 在陌生的多檔案程式庫裡做事，同時講出自己怎麼驗證 AI 的輸出。
3. 系統設計與行為面試。

## 3. HackerRank 與 LeetCode 的差異

| 面向 | HackerRank | LeetCode |
| --- | --- | --- |
| 主要角色 | 雇主的評估平台：線上測驗、CodePair、Chakra。超過 3,000 家企業客戶。 | 個人練習平台，題庫和社群最大 |
| 測資 | 只公開範例測資，其餘隱藏，雇主可以連你自己的輸出都不顯示 | 送出後會顯示失敗的測資 |
| 輸入輸出 | 多數題目提供依函式簽章產生的 stub，部分題目或語言要自己解析 STDIN（官方沒有說明何時屬於哪一種） | 只寫函式，不用處理輸入 |
| 計分 | 依通過測資的比例給分（依雇主設定），所以先交一個正確的暴力解有價值 | 練習時只有通過與失敗 |
| 監考 | Proctor Mode（2025-07 之後建立的測驗）：AI 監控、攝影機與手機偵測、全程錄影、限制切換分頁、停用複製貼上、限用單一螢幕、AI 抄襲偵測和候選人之間的程式相似度比對 | 練習時沒有 |
| AI 使用 | 依雇主和測驗設定：關閉、Guarded（只解答語法和概念）、Unguarded（2026-04 起）。AI 對話紀錄預設附在報告裡。 | 練習時自行決定 |
| 練習資源 | Interview Preparation Kit（14 個主題，各標示考到該主題的公司比例）、技能認證 | Study Plan（LeetCode 75、Top Interview 150）、公司標籤（Premium）、競賽、模擬面試 |

**建議的分工。**

- 內容（模式與題型）在 LeetCode 練。
- 考前 1 到 2 週，到 HackerRank 練格式：自己解析 STDIN、隱藏測資、計時、沒有 IDE 補全。
- Interview Preparation Kit 各主題被考到的公司比例：陣列 70%；雜湊、排序、字串各 40%；貪婪 31%；搜尋 30%；DP 27%；堆疊與佇列 17%；圖 15%；樹 12%；鏈結串列 8%；遞迴與回溯 5%。這個比例可以拿來決定 HackerRank 線上測驗的準備重點。

## 4. 不背答案的方法

### 4.1 學習科學的根據

| 技巧 | 證據 | 套到刷題上 |
| --- | --- | --- |
| 檢索練習（測驗效應） | Roediger 和 Karpicke（2006）：考 5 分鐘後的測驗時，重讀組較好；考 2 天和 1 週後的測驗時，反過來是測驗組較好。Dunlosky 等人（2013）評為高效益。 | 複習時從空白編輯器重寫。重讀舊解答不算複習。 |
| 分散練習 | Dunlosky 2013 評為高效益 | 錯題依 1、3、7、14、30 天的間隔重做，或交給 SM-2 / FSRS 工具排程 |
| 交錯練習 | Taylor（2008）隔天考新題：交錯組 77%，分塊組 38%。Rohrer 等人（2014）72% 對 38%。機制是學會「選用哪個策略」。 | 一個主題學完後，改做不標主題的混合題組，先判斷模式再動手 |
| 先掙扎再學（productive failure） | Sinha 和 Kapur（2021）統合分析 53 篇研究，先嘗試解題再接受教學的效果較好（效果量取自摘要，未核對全文） | 先自己嘗試，有時間上限，再看提示或解答 |
| 流暢度錯覺 | 重讀會高估自己的掌握程度，自我測驗可以修正。自己產出的內容記得較牢。 | 看懂解答不等於會解。關掉解答能重寫出來，才算學會。 |
| 自我解釋 | Dunlosky 2013 評為中等效益 | 動手寫之前，先用一句話講出不變量；複盤時寫下「關鍵洞見」 |

### 4.2 每題固定流程

這套流程融合了 BCtCI 的 MIKE 模板、NeetCode 的時間上限，以及第 4.1 節的研究。

| 步驟 | 動作 | 時間 |
| --- | --- | --- |
| 0. 讀題 | 用自己的話複述題目，列出輸入範圍和邊界，記下要問面試官的澄清問題 | 2–3 分 |
| 1. 由限制推複雜度 | 從 n 的上限推出目標複雜度（見第 4.3 節），先排除不可能的做法 | 1 分 |
| 2. 手算小例子 | 手算 2 到 3 個例子，包含一個邊界例子 | 2–3 分 |
| 3. 暴力解 | 講出暴力解和它的複雜度，不一定要寫出來（MIKE 的 M） | 2 分 |
| 4. 找瓶頸 | 迴圈裡哪個操作重複做了？換資料結構、預先計算、排序、找單調性（BCtCI 的 booster） | 3–5 分 |
| 5. 對照觸發條件 | 用第 4.4 節的表選出候選模式（MIKE 的 K） | 1–2 分 |
| 6. 講出不變量 | 寫程式之前，先用一句話講出這個模式靠什麼成立 | 1 分 |
| 7. 寫程式、自己出測資 | 自己出測資：空輸入、單一元素、重複值、極值、溢位、全部相同、已經排序 | 10–15 分 |
| 8. 複盤 | 寫一張卡（第 4.5 節），排定下次複習 | 3 分 |

**卡住時的時間上限。**

1. 15 到 20 分鐘沒有進展，只看提示：題目的主題標籤，或 editorial 的第一段。
2. 再試 10 分鐘。
3. 還是不行，就讀完整解答，理解它。
4. 關掉解答，從零重寫到通過。
5. 隔天重做一次。

讀解答 45 到 60 分鐘還看不懂，表示這題超出目前的程度，先退回同一模式較簡單的題目。

**準備好了的標準。** 沒見過的 Medium 大多能在 20 到 25 分鐘內解出（NeetCode 的基準）。而且連續幾場模擬面試都通過，不是只過一場。interviewing.io 的資料顯示，只有約 20% 到 25% 的人每場表現穩定。第 10 節練習專案的 `pp ready` 依這個標準計算準備度。

### 4.3 由限制推目標複雜度

這是經驗法則，假設每秒約可執行 10⁸ 次簡單運算。

| n 的上限 | 目標複雜度 | 常見做法 |
| --- | --- | --- |
| ≤ 10 | O(n!) | 全排列回溯 |
| ≤ 20 | O(2ⁿ)、O(2ⁿ·n) | 子集枚舉、bitmask DP |
| ≤ 500 | O(n³) | 區間 DP、Floyd–Warshall |
| ≤ 5,000 | O(n²) | 二維 DP、雙層迴圈 |
| ≤ 10⁵ 至 10⁶ | O(n log n)、O(n) | 排序、堆積、二分搜尋、雙指針、滑動視窗、雜湊 |
| ≥ 10⁹（數值範圍） | O(log n)、O(1) | 對答案二分、數學公式 |

### 4.4 觸發條件 → 模式對照表

這張表是本報告的核心，可以印出來，每題對照。觸發條件只是線索，BCtCI 和 AlgoMaster 都提醒它可能誤導，所以要再用第 4.2 節的步驟 4 確認。代表題標 ★ 的，在 withaarzoo repo 裡有講解，可以解完後對照。

| 觸發條件 | 模式 | 核心不變量 | LeetCode 代表題 | HackerRank Kit 對應 |
| --- | --- | --- | --- | --- |
| 已排序陣列、找數對或三數組合 | 雙指針（左右夾逼） | 每移動一次指針，就排除一整排不可能的答案 | 167、15、11★ | Pairs |
| 連續子陣列或子字串的最長、最短、至多 K 個 | 滑動視窗 | 視窗內永遠滿足條件，右邊擴張、左邊收縮 | 3、424、76 | Sherlock and the Valid String（計數） |
| 子陣列和等於 K、區間和 | 前綴和加雜湊 | sum(i..j) = P[j] − P[i−1] | 560、974 | Array Manipulation（差分陣列） |
| 查找、計數、去重、配對 | 雜湊表 | 用空間換掉一層迴圈 | 1、49、128 | Count Triplets（左右兩個雜湊表） |
| 矩陣旋轉、螺旋走訪 | 矩陣操作與模擬 | 旋轉等於轉置加左右反轉；螺旋是四個邊界逐步內縮 | 48★、54 | — |
| 已排序，或「最小的可行值」 | 二分搜尋（含對答案二分） | 判斷條件對答案單調 | 33★、153★、875 | Triple Sum、Ice Cream Parlor |
| 下一個更大或更小的值、括號配對 | 單調堆疊 | 堆疊內保持單調，元素被彈出的那一刻就是它找到答案的時刻 | 20★、739、84 | Balanced Brackets、Largest Rectangle |
| 視窗內的最大或最小值 | 單調佇列 | 佇列頭永遠是目前的最值 | 239、862★ | — |
| Top K、第 K 大、合併 K 個 | 堆積 | 大小為 K 的堆積，只保留候選 | 215、347、23 | Find the Running Median（雙堆） |
| 區間重疊、會議室 | 排序後掃描 | 按起點或終點排序後，只要跟上一個比較 | 56、435、253 | — |
| 鏈結串列的環、中點 | 快慢指針 | 快指針一次兩步，有環必定相遇 | 141、142、876 | Detect a Cycle |
| 反轉、合併、重新串接鏈結串列 | 鏈結串列指標操作 | 改 next 之前先存下一個節點；用 dummy 頭節點消除「第一個節點」的特例 | 206、21、23 | — |
| 樹的深度、路徑、子樹性質 | DFS 後序回傳值、BFS 層序 | 子問題回傳「以此節點為根」的答案 | 104、102、124、236、297 | Height of a Binary Tree |
| BST 合法性、有序的樹 | DFS 帶上下界、中序走訪 | 每個節點都有從祖先一路繼承下來的合法範圍 (low, high) | 98、230 | Is This a BST |
| 網格、無權圖的最少步數 | BFS | 第一次到達某點時就是最短距離 | 200、994、1091 | BFS: Shortest Reach |
| 能從邊界抵達、能流到兩側的格子 | 從目標反向的多源 BFS | 從邊界往回推，每一格只走一次 | 417★ | — |
| 依賴關係、先後順序 | 拓撲排序 | 入度為 0 的節點才能處理 | 207、210 | — |
| 連通分量、動態合併、找多餘的邊 | Union-Find | 路徑壓縮加按秩合併 | 684★、547 | Roads and Libraries |
| 分成兩組、相鄰的點不能同組 | 二分圖著色 | 相鄰的點顏色相反；每個連通塊都要檢查 | 785 | — |
| 帶權最短路徑、最小化路徑上的最大值 | Dijkstra | 從堆積中取出的節點，距離已經確定 | 743、778★ | — |
| 最多 k 次轉機、限制段數的最短路 | Bellman-Ford（限制輪數） | 第 i 輪只用上一輪的距離，保證最多 i 段 | 787 | — |
| 用最小總長把所有點連起來 | 最小生成樹（Prim、Kruskal） | 每次加入離目前這棵樹最近的點（切割性質） | 1584 | — |
| 列出所有組合、排列、子集，網格上找路徑 | 回溯 | 選擇、遞迴、撤銷；用剪枝減少分支 | 78、46、39、79 | Recursive Digit Sum（遞迴） |
| 最優值或方案數，且有重疊子問題 | DP（1D、背包） | 先定義狀態，轉移只依賴更小的狀態 | 70、198、322、300、139 | Max Array Sum、Abbreviation |
| 兩個字串的比對與編輯、網格路徑數 | 2D DP | dp[i][j] 只依賴左、上、左上三格 | 1143、72、62 | Common Child |
| 局部最優能證明會導向全域最優 | 貪婪 | 交換論證 | 55、134 | Max Min、Minimum Absolute Difference |
| 字首查詢、字典 | Trie | 共用前綴的節點 | 208、211 | Contacts |
| 只出現一次的數、位元計數 | 位元運算、XOR | x ^ x = 0 | 136、191、338 | Lonely Integer |
| 資料流的中位數 | 雙堆積 | 兩個堆積的大小差不超過 1 | 295 | Find the Running Median |
| 設計題，要求 O(1) 存取並維持順序 | 雜湊表加雙向鏈結串列 | 雜湊表找節點，鏈結串列維持順序 | 146 | — |

### 4.5 複盤卡片：記洞見，不記程式碼

```
題目：LC 739 Daily Temperatures
觸發：「下一個更暖的日子」= 下一個更大的值
模式：單調遞減堆疊（存索引）
關鍵洞見：某天被彈出堆疊的那一刻，就是它等到答案的時刻
不變量：堆疊內的溫度由底到頂遞減
複雜度：時間 O(n)，空間 O(n)
卡在哪：先想到 O(n²) 的雙迴圈，沒想到「讓還在等的日子排隊」
變體：901 Online Stock Span、84 Largest Rectangle in Histogram
下次複習：+3 天
```

- 複習時只看題目，從零寫出來，不先看卡片。
- 寫不出來，才翻開卡片，而且先只看「觸發」和「關鍵洞見」兩欄。

### 4.6 間隔與交錯的排程

**依複習結果調整間隔。**

- 一次就通過：間隔延長，1 → 3 → 7 → 14 → 30 天。
- 需要提示：間隔縮短到上一次的一半。
- 必須看解答：隔天重做，重新從 1 天開始。

**排程工具。** 小型專案，使用前先確認還有人維護。

- Firefox 外掛 Leetcode Anki：SM-2，通過後評 Again、Hard、Good 或 Easy。
- CLI 工具 grind：SM-2。
- Leetcode Mastery Scheduler：FSRS。
- 也可以用試算表自己記錄，欄位是題號、模式、結果、下次日期。

**新題和複習題的比例。** 前四週約 2 比 1，之後 1 比 1。

**交錯的做法。**

- 一個主題先集中練習 5 到 8 題，學會這個模式。
- 之後改從已學過的主題隨機抽題，練習時不看題目的主題標籤。
- 適合用來交錯的來源：LeetCode 每日一題、模擬面試、HackerRank Kit 跨主題混做。

## 5. 題單與題量

| 題單 | 題數 | 特色 | 適合誰 |
| --- | --- | --- | --- |
| Blind 75 | 75 | 原始版，沒有順序，不能客製 | 時間極少，只求覆蓋 |
| Grind 75 / 169 | 75 至 169 | Blind 75 的作者重新設計。可以依週數和每週時數產生排程，題目按優先順序排列，每題有建議用時，前 75 題沒有 Premium 題 | **在職的高級工程師首選** |
| NeetCode 150 / 250 | 150 / 250 | 按依賴順序排列，每題都建立在前面的題目上，每題有影片講解；150 據稱包含 Blind 75（二手） | 喜歡看影片、需要講解 |
| LeetCode 75 | 75 | 官方 study plan，沒有 Hard（二手） | 暖身 |
| Top Interview 150 | 150 | 官方 study plan，約 39 Easy、93 Medium、18 Hard（二手） | 習慣在 LeetCode 站內追蹤進度 |
| Sean Prashad 的 LeetCode Patterns | 約 170 至 180 | 按模式分組，解答放在另一個分支，避免看到 | 想按模式整組練 |
| Grokking / AlgoMonster | 課程 | 模式數 16 到 28 種，依版本而不同；AlgoMonster 有「關鍵字 → 演算法」對照和決策流程圖 | 願意付費，想要結構化教材 |

**題量建議。**

- 用一份題單做主線，做完再擴充，不要同時開好幾份。這幾份題單高度重疊。
- 題數目標 150 到 250 題，以 Medium 為主。Grind 的 FAQ 也說超過 169 題之後收益遞減。
- 練完約 100 題 Medium 之後，開始加入熱門、通過率高的 Hard。interviewing.io 的資料：每多 50 題 Medium 約加 3 個百分位，每多 50 題 Hard 約加 7 個；超過約 500 題後趨於平緩。
- 公司標籤（LeetCode Premium）只在面試前 1 到 3 週、已經知道目標公司時使用。標籤是群眾回報的，不是外流的考題。
- 競賽積分和面試表現沒有明顯相關（interviewing.io，樣本小），可以當作練速度，但不能當作準備好的指標。

## 6. 時程計畫：在職高級工程師，8 到 12 週

### 6.1 準備時間分配

下列比例是本報告依各來源綜合出的建議，沒有原始資料直接給出這些數字。

| 項目 | 比例 | 說明 |
| --- | --- | --- |
| 系統設計 | 35–40% | 高級職位槓桿最大的部分 |
| DSA | 25–35% | 以模式為主，目標是 Medium 流暢 |
| AI 輔助與實務程式 | 10–15% | 在陌生的多檔案程式庫中修 bug、加功能，審查 AI 的輸出 |
| 行為面試與 LLD/OOD | 10–15% | 領導、衝突、跨團隊的故事；目標公司若考物件導向設計就加練 |

Tech Interview Handbook 估計，DSA 最低限度的準備約需 30 小時，充分準備約需 100 小時。

### 6.2 DSA 週計畫

每天 60 到 90 分鐘：1 題新題，加 1 到 2 題到期的複習。週末做一次 90 分鐘計時，2 到 3 題，再做一次模擬面試。

| 週 | 主題（先分塊學） | 同時進行 |
| --- | --- | --- |
| 1–2 | 陣列、雜湊、雙指針、滑動視窗、前綴和、堆疊、二分搜尋 | 系統設計基礎 |
| 3–4 | 鏈結串列、樹、Trie、堆積、回溯 | 開始間隔複習 |
| 5–6 | 圖（BFS、DFS、拓撲排序、Union-Find、Dijkstra）、1D 與 2D DP | 開始第一次模擬面試 |
| 7–8 | 貪婪、區間、位元運算；改成交錯練習，加入 Hard | HackerRank 計時練習，自己解析 STDIN |
| 9–12 | 公司標籤（面試前 1 到 3 週）、弱項補強 | 至少 5 次模擬面試、AI 輔助回合練習 |

## 7. 兩個參考 repo 的評估與用法

### 7.1 Java-aid/Hackerrank-Solutions

| 項目 | 內容 |
| --- | --- |
| 規模 | 1,844 星、858 fork、MIT 授權。2017-11-30 建立，最後推送 2025-12-31。 |
| 環境 | Java 8、Eclipse、Maven，共 374 個檔案，其中 292 個是 `.java` |
| 分布 | 基礎程式 67、Java 語言 64、30 Days of Code 約 30、字串 22、排序 18、CtCI 教學 17、鏈結串列 15、樹 7、搜尋 6、貪婪 6、位元 5、堆疊 5、堆積 3、圖 3、DP 2、Trie 2、併查集 2。**Interview Preparation Kit 只有 2 題**（Count Triplets、Triple Sum）。 |
| 解答形式 | 檔頭附題目連結和 YouTube 講解連結（README 裡有 57 個影片連結），程式本身幾乎沒有複雜度或思路註解，用 `Scanner` 讀 STDIN |
| 其他 | README 有 Amazon 聯盟連結和一個推廣工具的連結。在 Windows 上 clone 時，因路徑過長（repo 內最長的路徑 167 字元）會 checkout 失敗，要先設定 `git config --global core.longpaths true`，或直接在網頁上瀏覽。 |

**適合的用法。**

- 參考 HackerRank 自己解析 STDIN 的 `main` 怎麼寫。但 `Scanner` 在大量輸入時很慢，正式考試請改用第 8 節的快速輸入樣板。
- 拿 Count Triplets 學「以中間項為軸，左右各一個雜湊表」的計數技巧。題目要求 `long`，也提醒了整數溢位這個陷阱。
- 對照 Java 的集合 API 寫法。

**不適合的用法。** 不適合當題單。題目以 Easy、基礎題和語法題為主，DP、圖這些高級職位的重點只有個位數。

### 7.2 withaarzoo/LeetCode-Solutions

| 項目 | 內容 |
| --- | --- |
| 規模 | 111 星、23 fork，**沒有授權條款**（GitHub API 回傳 `license: null`）。2024-07-01 建立，研究當日仍在每日更新。 |
| 題數 | 511 題：Easy 127、Medium 278、Hard 106 |
| 語言 | 每題都有 C++、Java、JavaScript、Python、Go，其中 65 題另有 TypeScript |
| 講解 | 每題一份 `explanation.md`。416 題用完整模板：Problem Summary、Constraints、Intuition、Approach、Data Structures、Complexity、逐行說明。較早的題目只有逐行說明。 |
| 選題 | 依 LeetCode 每日一題，不按模式排序。Blind 75 只涵蓋 7 題（11、20、33、48、153、190、417），179 題是 3000 號以後的新題。 |
| 正確性 | 單一作者的個人 repo，本研究沒有逐題驗證 |

**適合的用法。**

- 自己解完後讀 Intuition 段，比對思路。它的寫法是「暴力解 → 瓶頸 → 觀察 → 模式」，本身就是很好的複盤範本。
- 多語言對照，學另一種語言的慣用寫法。例如主力用 Java 的人，可以看 Python 版本的寫法。
- 當交錯練習的題庫：從裡面抽自己沒做過的題，不看標題分類，先判斷模式再動手。

**不適合的用法。**

- 不適合當主線。題目依每日一題而定，和面試高頻題重疊很少。
- 沒有授權條款，引用或轉載它的內容前要先注意。

### 7.3 使用解答 repo 的規則

| 有效 | 反效果 |
| --- | --- |
| 自己認真嘗試之後，再比較 2 到 3 種解法的取捨 | 動手之前先看解答：產生流暢度錯覺，也錯失先掙扎的學習效果 |
| 學習面試用語言的標準函式庫寫法 | 複製貼上，只為了累積 Accepted |
| 把按模式分類的 repo 當成「要練什麼」的索引 | 背下 Kit 或認證題的答案：面試官改用變體就失效 |
| 看完後關掉，從零重寫，並排入間隔複習 | 使用外流的線上測驗或認證解答：違反誠信規範，HackerRank 的相似度比對和 AI 抄襲偵測就是為此設計的 |

## 8. HackerRank 線上測驗的專項準備

**收到邀請時先確認：**

- 時限和題數
- 可用的語言
- 是否開啟監考
- 是否允許使用 AI，以及是 Guarded 還是 Unguarded 模式

不確定的，問招募人員。

**輸入樣板要先練熟。** Java：

```java
import java.io.*;
import java.util.*;

public class Solution {
    public static void main(String[] args) throws IOException {
        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
        StringTokenizer st = new StringTokenizer(br.readLine());
        int n = Integer.parseInt(st.nextToken());
        long[] a = new long[n];
        st = new StringTokenizer(br.readLine());
        for (int i = 0; i < n; i++) a[i] = Long.parseLong(st.nextToken());
        StringBuilder out = new StringBuilder();
        // ... 解題 ...
        System.out.print(out);
    }
}
```

Python：

```python
import sys
data = sys.stdin.buffer.read().split()
n = int(data[0])
a = list(map(int, data[1:1 + n]))
```

**針對隱藏測資的檢查清單：**

- 空輸入、n = 1
- 全部相同、已經排序、反向排序
- 最大的 n（會不會逾時）
- 最大的值（`int` 會不會溢位）
- 負數、0
- 重複值
- 多組測資時，狀態有沒有重設

**作答策略：**

- 計分依通過測資的比例，所以先交一個正確的暴力解拿基本分，再優化。
- 留 5 分鐘自己跑一次極端測資。

**監考環境：**

- 不能複製貼上、只能用單一螢幕、切換分頁會被記錄。
- 平常就在沒有 AI 補全的純編輯器裡練習。
- 不要在考試中切出去查資料，除非測驗明確允許。

**AI 模式：**

- 開放使用 AI 時，對話紀錄會附在報告裡。
- 要讓紀錄呈現你的判斷：拆解問題、要求 AI 解釋、自己驗證，不是整段要答案。

## 9. 高級職位：刷題以外的準備

- **AI 輔助程式回合。** 找一個沒碰過的開源專案，限時 60 分鐘，用 Claude 或 Copilot 修一個 issue。過程中講出幾件事：先讀哪些檔案、怎麼驗證 AI 的修改、補了什麼測試。Canva 評的就是拆解問題、判斷 AI 輸出、除錯到可上線的品質。
- **系統設計。** 這是決定職級（L5、L6 或降級）的主要訊號。要準備容量估算、資料模型、一致性取捨、故障模式。
- **行為面試。** 準備 6 到 8 個 STAR 故事，涵蓋影響力、衝突、失敗、跨團隊合作、指導他人、技術決策。
- **LLD/OOD。** Amazon、Uber 等公司有物件導向設計回合，練習停車場、LRU、速率限制器這類題目。
- **模擬面試。** 至少 5 次，最好錄影事後檢討（BCtCI 的建議）。判斷有沒有準備好，看的是多場的穩定度，不是單場。第 10 節練習專案的 `pp mock` 可以做計時的演算法模擬面試，並記錄自我檢討。

## 10. VS Code C++ 練習專案

[cpp-practice/](cpp-practice/README.zh-TW.md) 把本報告的方法做成可以每天照著走的練習流程。

- **安裝需求**：只要 VS Code、C/C++ 擴充、WinLibs GCC（`winget install --id BrechtSanders.WinLibs.POSIX.UCRT --exact`）。
- **題目**：74 題，第 4.4 節觸發條件對照表的 30 種模式都有題目，其中 24 種有 2 題以上，常考的模式有 3 到 14 題；其中 4 題要自己讀 STDIN 的 HackerRank 題。題目大致依第 6.2 節的主題順序分成 10 週，第 11 到 12 週留給複習與模擬面試；和 Blind 75 重疊 42 題（56%）。
- **測資**：每題附自己出的範例與邊界測資、暴力 oracle 對拍（失敗時印出最小反例）；需要分辨複雜度的 60 題另有題目上限大小的效能測試（O(n²) 寫法會失敗）。
- **`pp` 練習工具**：用 C++ 寫成，把第 4 節的規則做成強制流程。
  - 重做時封存舊版、還原空白檔（檢索練習）。
  - 15 到 20 分鐘的時間盒與分層提示。用過提示，這次的 pass 會自動改記為 hint。
  - 沒填 `[觸發]`、`[關鍵洞見]` 卡片就不能記錄結果。
  - 依 1、3、7、14、30 天排複習。
  - 交錯抽題時隱藏模式。
  - 依模式統計弱項。
  - 到期複習積壓時，不給新題。
  - 計時模擬面試（`pp mock`）：一次給一題，隱藏模式和難度，沒有提示。要先依面試官的評分項目自評，才會記錄結果。沒通過的題目當天排進複習。
  - 準備度（`pp ready`）：依第 4.2 節和第 9 節的標準，檢查模擬面試場數、連續通過的場數和 Medium 的用時。
- **考場模擬**：工作區關閉 AI 行內補全。
- **已驗證**：在 Windows 11 上用 g++ 16.2.0 驗證過：74 題的空白 stub 都不會通過；參考解答全部通過；14 個刻意寫成錯誤複雜度的解法，都只在效能測試失敗。

WinLibs 下載頁上同一個 GCC 16.2.0 有多種建置，選擇方式整理在 [cpp-practice/docs/toolchain.zh-TW.md](cpp-practice/docs/toolchain.zh-TW.md)。

## 11. 未核實與限制

- BCtCI 的五種 booster，公開節錄中只確認到三種：暴力解優化（含資料結構 booster）、找出題目性質、降低難度或拆解問題。
- LeetCode 75、Top Interview 150 的題數組成，以及 Sean Prashad 題單的題數（170 到 179 不等），來自二手來源。
- HackerRank 沒有對候選人公開說明什麼情況是 stub、什麼情況要自己解析 STDIN。
- Shopify 允許 AI 的說法、Meta「E7 以上只考 AI 回合」的說法，都是二手來源。
- Roediger 和 Karpicke 常被引用的 61% 對 42%，以及 Sinha 和 Kapur 的效果量，沒有核對原文。
- 「掙扎 20 到 45 分鐘」沒有權威來源，本報告採用 NeetCode 的 15 到 20 分鐘。
- 第 6.1 節的時間分配比例和第 4.6 節的間隔數字，是本報告的綜合建議。
- HackerRank 抄襲偵測 93% 的準確率是廠商自己的說法。interviewing.io 販售模擬面試，它的資料屬於觀察性資料，也有利益關係。
- 兩個 repo 的解答正確性沒有逐題驗證。

## 12. 參考資料

**題單與方法**

- Tech Interview Handbook，Coding interview prep：https://www.techinterviewhandbook.org/coding-interview-prep/
- Grind 75，About 與 FAQ：https://www.techinterviewhandbook.org/grind75/about 、https://techinterviewhandbook.org/grind75/faq
- NeetCode，How to use NeetCode effectively：https://neetcode.io/courses/lessons/how-to-use-neetcode-effectively
- Sean Prashad，LeetCode Patterns：https://seanprashad.com/leetcode-patterns
- AlgoMonster，keyword to algorithm：https://algo.monster/problems/keyword_to_algo
- Nil Mamano，Problem solving BCtCI style：https://nilmamano.com/blog/problem-solving-bctci-style
- The Pragmatic Engineer，How to get unstuck during coding interviews：https://newsletter.pragmaticengineer.com/p/how-to-get-unstuck-during-coding-interviews
- AlgoMaster，How to identify which pattern to use：https://algomaster.io/learn/dsa/how-to-identify-which-pattern-to-use

**學習科學**

- Roediger & Karpicke（2006）：https://psychology.ecu.edu/wp-content/pv-uploads/sites/216/2019/03/Roediger-Karpicke-2006.pdf
- Dunlosky 等人（2013）摘要：https://www.psychologicalscience.org/news/releases/which-study-strategies-make-the-grade.html
- Taylor（2008）交錯練習：https://digitalcommons.usf.edu/etd/529
- 交錯練習研究整理：https://landmark.edu/research-training/blog/making-learning-stick-research-on-interleaving
- Sinha & Kapur（2021），DOI 10.3102/00346543211019105
- 流暢度錯覺（Frontiers in Psychology 2019）：https://pmc.ncbi.nlm.nih.gov/articles/PMC6713021/

**資料與調查**

- interviewing.io，AI 如何改變面試流程：https://interviewing.io/blog/how-is-ai-changing-interview-processes-not-much-and-a-whole-lot
- interviewing.io，LeetCode 積分與面試表現：https://interviewing.io/blog/how-well-do-leetcode-ratings-predict-interview-performance
- interviewing.io，模擬面試與通過率：https://interviewing.io/blog/technical-interview-practice-gap 、https://interviewing.io/blog/how-know-ready-interview-faang
- The Pragmatic Engineer，The reality of tech interviews：https://newsletter.pragmaticengineer.com/p/the-reality-of-tech-interviews

**AI 面試與作弊**

- Canva Engineering：https://canva.dev/blog/engineering/yes-you-can-use-ai-in-our-interviews
- Hello Interview，Meta AI-enabled coding：https://www.hellointerview.com/blog/meta-ai-enabled-coding
- Hello Interview，Shopify：https://www.hellointerview.com/blog/shopify-ai-enabled-coding
- TechCrunch，HackerRank Chakra：https://techcrunch.com/2026/10/05/hackerranks-ai-interviewer-offers-a-glimpse-into-what-job-interviews-could-become/
- TechCrunch，Interview Coder 與 Cluely：https://techcrunch.com/2025/04/21/columbia-student-suspended-over-interview-cheating-tool-raises-5-3m-to-cheat-on-everything
- Entrepreneur，Google 恢復現場面試：https://www.entrepreneur.com/business-news/google-mckinsey-reintroduce-in-person-interviews-due-to-ai/496041

**HackerRank**

- Interview Preparation Kit：https://www.hackerrank.com/interview/interview-preparation-kit
- Proctor Mode：https://hackerrank-knowledge-base.help.usepylon.com/articles/5663779659-proctor-mode
- 隱藏測資：https://hackerrank-knowledge-base.help.usepylon.com/articles/1626384486
- 自訂輸入格式：https://hackerrank-knowledge-base.help.usepylon.com/articles/2461683826-test-case-input-format
- AI 助手模式：https://hackerrank-knowledge-base.help.usepylon.com/articles/1152916770 、https://hackerrank-knowledge-base.help.usepylon.com/articles/4368819843-april-2026-release-notes

**間隔複習工具**

- Leetcode Anki：https://addons.mozilla.org/en-US/firefox/addon/leetcode-anki/
- grind：https://github.com/brandon-gong/grind

**參考 repo**

- Java-aid/Hackerrank-Solutions：https://github.com/Java-aid/Hackerrank-Solutions
- withaarzoo/LeetCode-Solutions：https://github.com/withaarzoo/LeetCode-Solutions
