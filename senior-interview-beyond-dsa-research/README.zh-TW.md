[English](README.md) | **繁體中文**

# 高級軟體工程師求職：演算法以外的面試回合，2026 年的高效準備，不靠背答案

研究日期：2026-11-15。

- 網路資料取自當日查詢。只來自二手或廠商來源、沒能用原始來源核對的，標「未核實」。
- GitHub repo 的星數取自當日的 GitHub API。
- 本報告接續 [HackerRank 與 LeetCode 刷題研究](../coding-interview-practice-research/README.zh-TW.md)。那份報告在[第 2 節](../coding-interview-practice-research/README.zh-TW.md#2-20252026-高級工程師的面試現況)（面試現況）、[第 6 節](../coding-interview-practice-research/README.zh-TW.md#6-時程計畫在職高級工程師8-到-12-週)（時間分配與 DSA 週計畫）、[第 9 節](../coding-interview-practice-research/README.zh-TW.md#9-高級職位刷題以外的準備)（刷題以外的準備）簡短提過本報告的主題，這裡展開成可以照著做的方法。

## 1. 結論摘要

**不背答案的原則在每一種回合都適用，只是練習的對象不同。**

| 回合 | 不要背的 | 要練的 |
| --- | --- | --- |
| 系統設計 | 某一題的標準架構圖 | 需求觸發 → 設計模式 → 取捨（第 3.4 節） |
| 行為面試 | 逐字稿 | 每個故事的事實、數字、決策點，以及它能證明哪些訊號 |
| LLD/OOD | 某一題的類別圖 | 需求 → 實體 → 職責 → 擴充點 |
| AI 輔助程式 | 提示詞範本 | 自己決策、AI 執行、每一步都驗證 |

面試官就是用改變條件和追問取捨來分辨背誦與理解。Hello Interview 的評分說明也寫明了這一點。

**系統設計是決定職級的主要訊號。**

- 用同一套框架：需求約 5 分、核心實體約 2 分、API 約 5 分、資料流約 5 分（可省略）、高階設計 10 到 15 分、深入探討約 10 分。
- 容量估算只在結果會改變設計時才做。
- 高級：主動找出系統的難點，在大約 2 個地方講到實務細節。
- Staff：幾乎主導整場，直接做決定而不是列選項給面試官挑，並主動砍掉不必要的複雜度。
- 不用把 30 多題全做完。挑 12 到 15 題、涵蓋所有設計模式就夠，剩下的時間用來做變體練習和模擬面試。

**行為面試：準備 6 到 8 個故事，來自 3 到 5 個專案。**

- 做一張矩陣，把故事對應到目標公司的訊號：Meta 的 8 個面向、Amazon 的 16 條領導力準則。每個訊號至少要有 2 個故事可以用。
- 對高級職位來說，關鍵是故事的 scope：影響的是一個團隊、整個組織，還是好幾個團隊。初階也能講的故事，放在高級面試裡可能變成負面訊號。
- 講「我」做了什麼，不講「我們」。

**LLD/OOD：不是每家都考，先問招募人員。** 約 35 分鐘、5 個步驟。評的是職責怎麼劃分、狀態放在哪裡、需求改變時好不好擴充，不是你背了多少設計模式。

**AI 輔助程式回合已經是主流公司的正式回合。**

| 公司 | 狀態 | 重點 |
| --- | --- | --- |
| Meta | 2025-10 起 | 60 分鐘，CoderPad，在多檔案程式庫裡依序修 bug、實作功能、優化 |
| Canva | 2025-06 起 | 後端與前端職位以它取代 CS Fundamentals 篩選 |
| Google | 2026 下半年試行 | 初中階、美國部分團隊，用 Gemini 閱讀、除錯、優化既有程式 |
| HackerRank Chakra | 2026-10-05 正式上線 | AI 面試官，在真實程式庫中觀察你與 AI 合作，給「AI 熟練度」評分 |

各家的評分重點一致：拆解問題、掌控 AI、驗證輸出、持續溝通。完全交給 AI 會失分，完全不用 AI 也會失分。

**時間怎麼排。** 沿用前一份報告的比例：系統設計 35–40%、DSA 25–35%、AI 輔助 10–15%、行為與 LLD 10–15%。第 7 節給出每週 18 到 20 小時的 12 週計畫，和既有的 DSA 週計畫並排。

## 2. 2026 年各回合的樣貌

| 回合 | 常見形式 | 主要評估 | 對高級職位的份量 | 本報告 |
| --- | --- | --- | --- | --- |
| 系統設計 | 45 到 60 分鐘，白板或 Excalidraw 類工具。Meta 依職務方向分成 System Design 和 Product Architecture。 | 問題拆解、方案設計、技術深度、溝通協作（Hello Interview 的四個評分主題） | 最高。Hello Interview 說它在高級職位占「不成比例的權重」。The Pragmatic Engineer 指出高級職位被期待具備過去屬於 staff 的分散式系統知識。 | 第 3 節 |
| 行為與領導力 | 約 45 分鐘，5 到 6 題，每題深入追問。Amazon 在每一輪都問領導力準則。 | 公司價值觀對應的行為，以及故事 scope 所顯示的職級 | 高。它也被用來判斷職級。 | 第 4 節 |
| LLD/OOD | 約 35 分鐘的類別設計加部分程式碼；部分公司是 90 分鐘、要能執行的 machine coding | 職責劃分、程式品質、可擴充性、溝通 | 中，依公司而定 | 第 5 節 |
| AI 輔助程式 | 60 分鐘，在多檔案程式庫中使用內建 AI 助手 | 問題拆解、AI 掌控、驗證習慣、溝通 | 上升中。據報導 Meta 到 E7 都採用（未核實）。 | 第 6 節 |

**對準備方式的意義。** 這四種回合都會在你講完第一版之後改條件、追問「為什麼」。所以練習時要刻意製造變體：改一個非功能需求、換一個限制、問一個追問，看自己能不能當場推出新答案。

## 3. 系統設計

### 3.1 各職級的期望

資料來源是 Hello Interview 的職級說明和 staff 面試文章。作者是前 Meta staff engineer 和資深主管，屬於業界經驗談，不是任何公司的官方評分標準。

| 面向 | 中階 | 高級 | Staff 以上 |
| --- | --- | --- | --- |
| 廣度 | 要有廣泛的基本知識，面試官可能問任何元件，例如 API gateway 是做什麼的 | 假設你懂基礎，除非露出破綻，否則不會考 | 假設你完全掌握基礎，重點是怎麼用基礎做出好的取捨 |
| 深度 | 用教科書的角度講清楚核心概念就夠 | 從「我會用 NoSQL」這種泛泛之談，講到實務細節，例如 DynamoDB 的 partition key 和 sort key 怎麼選。大約要在 2 個地方講深。 | 多個地方都講得深，最好能教面試官一些東西 |
| 主動性 | 主導需求、API、資料模型、高階設計；後段由面試官主導追問 | 及早指出這個系統難在哪裡，依此排序需求；主動指出自己設計的限制並討論替代方案 | 幾乎主導整場，以同儕身分對談 |

**Staff 面試的五個重點（Hello Interview）：**

1. 依面試官的程度調整說明。沒被問到就不要解釋負載平衡器怎麼運作，講太多基礎反而像是剛學會。
2. 找出關鍵難點（crux）。簡單的部分快速帶過，例如「這是很直接的 CRUD 服務」，時間留給最難、風險最高的部分。
3. 砍掉不必要的複雜度。文章的例子：在 1 萬個很少更新的地點中做位置搜尋，staff 候選人的答案是定期同步、在記憶體裡搜尋，而不是架一套地理索引。
4. 用過去的經驗展現深度，即使題目是不同的領域。
5. 做決定。最大的錯誤是列出 2 到 4 個選項，然後把選擇丟回給面試官。

**降級錄取的常見原因。** Hello Interview 的另一篇文章說，他們最近看到的降級變多了，過度設計是最常見的原因之一，而且是「較資淺候選人的典型特徵」。原因有四個：

- 用的技術越多，越容易漏掉東西。
- 維護成本高。在簡單的問題上加一堆新技術，本身就是警訊。
- 元件越多，出錯的地方越多。
- 細節講不完。面試官會注意你是不是只報名詞，追問時講不出來。

文章的建議：

- 加複雜度之前，先問有沒有更簡單的做法。
- 先做出滿足功能需求的最基本設計，再優化。
- 你帶進來的每一項技術都要知道缺點，例如 Redis 當機後怎麼恢復。

再加上前兩篇文章，降級的表現還包括：講太多基礎、只列選項不做決定、沒有主導討論。

### 3.2 可重複使用的框架

下表以 Hello Interview 的 Delivery Framework 為主，加上 Alex Xu《System Design Interview》四步驟裡的收尾步驟。幾個調整是本報告做的：

- Hello Interview 的「需求，約 5 分」拆成功能需求、非功能需求兩列。
- 容量估算原本是需求步驟裡的一部分，這裡獨立成一列。

時間以一場 45 分鐘來估。60 分鐘的場次，多出來的時間放在深入探討。

| 步驟 | 時間 | 產出 | 檢查點 |
| --- | --- | --- | --- |
| 1. 功能需求 | 約 3 分 | 3 個左右、排好優先順序的「使用者可以……」 | 像跟產品經理對談一樣提問；明確寫下不做的範圍 |
| 2. 非功能需求 | 約 2 分 | 3 到 5 條「系統應該……」，盡量量化 | 從清單挑：CAP 取捨、規模（讀寫比、突發流量）、延遲、耐久性、安全、容錯、法規 |
| 3. 容量估算（視情況） | 0 到 5 分 | 只算會影響設計的數字 | 先跟面試官說「需要時再算」；結果會決定要不要分片、要不要快取時才算 |
| 4. 核心實體 | 約 2 分 | 名詞清單，例如 User、Tweet、Follow | 還不用寫完整的資料表欄位 |
| 5. API | 約 5 分 | 端點或介面，例如 `POST /v1/tweets`、`GET /v1/feed` | 預設 REST；內部高效能呼叫用 gRPC；即時功能在核心 API 之後再加 WebSocket 或 SSE。使用者身分從 token 取得，不放在 request body。 |
| 6. 資料流（可省略） | 約 5 分 | 依序列出處理步驟 | 只用在資料處理型系統，例如爬蟲、ETL |
| 7. 高階設計 | 10 到 15 分 | 方框與箭頭，一次處理一個 API | 邊畫邊講資料怎麼流、每個請求改了什麼狀態；快取、佇列先標註，之後再展開 |
| 8. 深入探討 | 約 10 分 | 針對非功能需求強化設計、處理邊界情況和瓶頸 | 例如動態消息要用 fan-out on write 還是 fan-out on read。高級要自己找出弱點，不等面試官指出。 |
| 9. 收尾 | 2 到 3 分 | 總結取捨、已知限制、下一步 | 監控、故障處理、還沒解決的問題 |

Hello Interview 指出，中階候選人最常見的失敗是「沒有交出一個能運作的系統」，通常被歸因於時間管理。所以先完成一條端到端的路徑，再去優化。

### 3.3 容量估算

**原則。**

- 不追求精確。用整數，例如把 99987 / 9.1 當成 100,000 / 10。
- 寫下假設，數字要標單位。
- 讀和寫的 QPS 分開算，兩者通常差很多。
- 尖峰 QPS 約取平均的 2 到 3 倍。

**換算捷徑**（本報告依 86,400 秒／天推算）：

| 每天的量 | 平均 QPS |
| --- | --- |
| 100 萬 | 約 12 |
| 1 億 | 約 1,200 |
| 10 億 | 約 12,000 |

一天約 10⁵ 秒，所以「每天 X 次」除以 10⁵ 就是 QPS 的數量級。

**二的冪次。** 2¹⁰ ≈ 一千（KB）、2²⁰ ≈ 一百萬（MB）、2³⁰ ≈ 十億（GB）、2⁴⁰ ≈ 一兆（TB）、2⁵⁰ ≈ 一千兆（PB）。

**延遲數量級。** 取自 Jeff Dean 2010 年的列表，ByteByteGo 也註明部分數字已經過時，只用來比較數量級。

| 操作 | 時間 |
| --- | --- |
| L1 快取 | 0.5 ns |
| 主記憶體存取 | 100 ns |
| 從記憶體循序讀 1 MB | 250 µs |
| 同資料中心來回 | 500 µs |
| 磁碟 seek | 10 ms |
| 從磁碟循序讀 1 MB | 30 ms |
| 加州到荷蘭來回 | 150 ms |

結論：記憶體快、磁碟慢；避免隨機 seek；跨區域的來回很貴。

**可用性的幾個 9。**

| 可用性 | 每年停機 | 每月停機 |
| --- | --- | --- |
| 99% | 3.65 天 | 7.31 小時 |
| 99.9% | 8.77 小時 | 43.8 分 |
| 99.99% | 52.6 分 | 4.38 分 |
| 99.999% | 5.26 分 | 26.3 秒 |

99.9% 那一列是本報告依同一公式推算，其餘取自 ByteByteGo。

**範例：Twitter 的 QPS 與儲存量。** 數字是 ByteByteGo 的練習假設，不是 Twitter 的真實數據。

| 假設 | 推算 |
| --- | --- |
| 月活躍 3 億，其中 50% 每天使用 | 日活躍 1.5 億 |
| 每人每天發 2 則 | 寫入 QPS ≈ 1.5 億 × 2 / 86,400 ≈ 3,500；尖峰約 7,000 |
| 10% 的推文含 1 MB 媒體 | 媒體儲存每天 1.5 億 × 2 × 10% × 1 MB = 30 TB |
| 保存 5 年 | 30 TB × 365 × 5 ≈ 55 PB |

**估算要接到設計決策。** 每算出一個數字，就說出它代表什麼：

- 寫入 3,500 QPS：單一關聯式資料庫的主節點可能撐得住，也可能撐不住，這要看寫入的大小和索引數量。所以這時要講的是「這裡是要評估分片的地方」，而不是背一個「單機上限」。
- 55 PB 媒體：一定放 object storage 加 CDN，資料庫只存 metadata。

Hello Interview 用 Top-K 熱門話題當例子：不同話題的數量，決定了一個 min-heap 就夠，還是要分片。

### 3.4 需求觸發 → 設計模式對照表

這張表對應前一份報告的「觸發條件 → 模式」表，是本報告的整理。模式名稱主要參考 Hello Interview 的 Patterns 分類（Scaling Reads、Scaling Writes、Dealing with Contention、Real-time Updates、Multi-step Processes、Handling Large Blobs、Managing Long Running Tasks），加上常見的搜尋、地理、串流聚合等主題。

| 需求觸發 | 設計模式 | 要講出的取捨 | 代表題 |
| --- | --- | --- | --- |
| 讀遠多於寫、延遲要求低 | 快取（cache-aside）、讀取副本、CDN | 快取失效與一致性、熱鍵、快取穿透 | Bitly、News Feed |
| 寫入量大、單機撐不住 | 分片（依 hash 或範圍）、批次寫入、LSM 型資料庫 | 分片鍵怎麼選、熱點、跨分片查詢與交易 | Ad Click Aggregator、Metrics Monitoring |
| 多人搶同一份資源 | 悲觀鎖、樂觀鎖（版本號）、帶 TTL 的預留、分散式鎖 | 正確性與吞吐量、預留過期、公平性 | Ticketmaster、Flash Sale、Online Auction |
| 即時推送給使用者 | WebSocket 或 SSE，後端用 pub/sub | 連線狀態放在哪、伺服器怎麼水平擴充、離線訊息 | WhatsApp、FB Live Comments |
| 一個人的更新要給很多人看 | fan-out on write、fan-out on read，名人帳號用混合策略 | 寫入放大和讀取延遲 | News Feed、Instagram |
| 多步驟流程，中途可能失敗 | workflow 引擎（如 Temporal）、saga、outbox、冪等鍵 | 補償動作、重試、exactly-once 的「效果」 | Payment System、Job Scheduler |
| 大檔案上傳下載 | presigned URL 直接傳到 object storage、分段上傳、CDN | 斷點續傳、metadata 一致性 | Dropbox、YouTube |
| 耗時的工作 | 非同步佇列、worker、狀態查詢 API | 重試與毒訊息、進度回報 | YouTube 轉碼、Web Crawler |
| 全文或模糊搜尋 | 倒排索引（Elasticsearch），用 CDC 從主資料庫同步 | 索引延遲、主資料與索引的一致性 | FB Post Search |
| 依地理位置找附近 | geohash、quadtree、PostGIS 或 Redis GEO | 熱門區域的密度、位置更新頻率 | Yelp、Uber |
| 排行榜、Top-K、計數 | 串流聚合（如 Flink）、預先聚合、近似演算法（count-min sketch） | 精確與近似、時間視窗 | YouTube Top K、Game Leaderboard |
| 限制請求速率 | token bucket、滑動視窗，計數放在 Redis | 分散式計數的準確度、限流放在哪一層 | Rate Limiter |
| 多人同時編輯同一份文件 | OT 或 CRDT、每份文件一個協調者 | 衝突合併、離線編輯 | Google Docs |
| LLM 應用 | 串流回應、對話狀態儲存、請求排隊與 GPU 容量、RAG 檢索 | 延遲與成本、速率限制、評估 | ChatGPT |

用法跟 DSA 的表一樣：先從需求找出觸發條件，再挑候選模式，然後講出取捨。觸發條件只是線索。

### 3.5 常見題目與練習選題

Hello Interview 的練習清單有 32 題，依它標的難度：

- **Easy**：Bitly、Dropbox、Yelp、Local Delivery Service
- **Medium**：Ticketmaster、Instagram、FB News Feed、Tinder、LeetCode、WhatsApp、Strava、Distributed Cache、Rate Limiter、Online Auction、YouTube、Job Scheduler、FB Live Comments、News Aggregator、Price Tracking Service、Notification System
- **Hard**：YouTube Top K、Uber、Robinhood、Google Docs、Web Crawler、Ad Click Aggregator、FB Post Search、Payment System、Metrics Monitoring、Online Chess、ChatGPT、Flash Sale

**建議的 14 題。** 本報告的建議是依第 3.4 節的模式挑題，讓每個模式至少出現一次，而不是照順序全做：

| 順序 | 題目 | 主要涵蓋的模式 |
| --- | --- | --- |
| 1 | Bitly | 讀多寫少、ID 產生、快取 |
| 2 | Dropbox | 大檔案、同步、metadata |
| 3 | Ticketmaster | 資源競爭、預留與過期 |
| 4 | FB News Feed | fan-out、快取 |
| 5 | WhatsApp | 即時推送、離線訊息、連線狀態 |
| 6 | Rate Limiter | 分散式計數 |
| 7 | YouTube | 大檔案、轉碼、CDN、長時間工作 |
| 8 | Yelp 或 Uber | 地理搜尋；Uber 另外涵蓋即時位置和配對競爭 |
| 9 | Web Crawler | 資料流、佇列、去重、禮貌爬取 |
| 10 | YouTube Top K | 串流聚合、近似計數 |
| 11 | Ad Click Aggregator | 大量寫入、串流、冪等 |
| 12 | Payment System | 多步驟流程、冪等、對帳 |
| 13 | Google Docs | 協同編輯 |
| 14 | 目標公司的產品 | 例如面 Meta 就練 Instagram 或 FB Live Comments |

### 3.6 不背答案的練習方法

**每題的流程。**

1. **先自己做。** 計時 45 分鐘，用 Excalidraw 或面試會用的白板工具，從需求做到深入探討。
2. **再對照解答。** 自己做完才讀 answer key，記下差異。Hello Interview 的準備流程也是這個順序。
3. **寫一張設計複盤卡**，見下方範例。
4. **做變體練習。** 隔 1 到 2 週，從空白畫布重做，同時改一個條件。例如：
   - 寫入量變 10 倍
   - 要跨區域部署
   - 要求強一致性
   - 預算砍半
   - 加上新功能
5. **大聲講並錄下來。** 回放時檢查有沒有先講結論、有沒有做決定。

```
題目：Ticketmaster
關鍵難點：熱門場次開賣時，同一個座位被很多人搶
決定：選座時建立帶 TTL 的預留（Redis），付款完成才寫入資料庫並釋放預留
替代方案：資料庫悲觀鎖。放棄的原因是鎖持有時間跟付款流程一樣長
非功能需求的影響：開賣尖峰改用虛擬排隊，保護後端
我卡在哪：一開始想用分散式交易
變體：如果改成不劃位的站票？如果要支援轉售？
下次重做：+10 天
```

這張卡和 DSA 的複盤卡一樣，記的是洞見和決策，不記架構圖。

**模擬面試。**

- 至少做 3 到 5 次系統設計模擬面試。Hello Interview 建議找在目標公司工作的人。
- 也可以讓 LLM 扮演面試官，請它針對你的設計追問取捨、改變需求。Tech Interview Handbook 建議在行為面試這樣用，本報告建議延伸到系統設計。
- 模擬時刻意練兩件事：在前 10 分鐘內說出關鍵難點；被追問時直接做決定。

**用自己的經驗。** 高級和 staff 的深度，最可信的來源是你做過的系統。準備時把自己做過的 2 到 3 個系統各畫一次：當時的需求和規模、做過的決定、出過的事故、事後會怎麼改。這也能拿來回答 Google 行為回合裡新增的「過去作品技術設計討論」（第 4.1 節）。

### 3.7 Meta：Product Architecture 還是 System Design

Meta 依職務方向，把設計回合分成兩種：

- **Product Architecture**：產品方向的職位。偏使用者面的產品，重點是 API 設計、資料模型、client 與 server 的互動。
- **System Design**：基礎架構方向的職位。偏後端的規模、分散式系統。

有候選人回報，招募人員讓他自己選（Blind，未核實）。兩者準備內容高度重疊，題目甚至可能相同，例如「設計 YouTube」。資料來源都是第三方指南和論壇，沒有 Meta 官方說明。選擇方式：做產品功能、API 的人選 Product Architecture；做後端、基礎架構的人選 System Design。不確定就問招募人員。

### 3.8 資源

| 資源 | 類型 | 用途 | 備註 |
| --- | --- | --- | --- |
| Hello Interview「System Design in a Hurry」 | 網站，部分免費 | 框架、核心概念、關鍵技術、模式、32 題解答 | 部分內容和 Guided Practice 需付費 |
| Alex Xu《System Design Interview》第 1、2 冊 | 書，ByteByteGo 也有線上版 | 四步驟框架、估算、經典題的完整解答 | 適合讀完自己做題後對照 |
| Kleppmann 與 Riccomini《Designing Data-Intensive Applications》第 2 版 | 書 | 複製、分區、交易、一致性、串流的原理 | 2026 年出版（O'Reilly 目錄標 2026-02，作者部落格文章 2026-03-24）。Hello Interview 建議高級候選人當補充讀物。 |
| donnemartin/system-design-primer | GitHub | 主題索引、估算、Anki 卡 | 377,928 星；最後推送 2026-09-15 |
| ByteByteGoHq/system-design-101 | GitHub | 圖解概念 | 91,205 星；最後推送 2025-04-04 |
| karanpratapsingh/system-design | GitHub | 概念整理與題目 | 46,906 星；最後推送 2026-10-18 |
| Aminian 與 Sheng《Generative AI System Design Interview》 | 書 | LLM、RAG、評估、安全、成本 | 2024 年出版（二手來源）。目標是 AI 或 ML 職位時加讀。 |
| 目標公司的工程部落格 | 部落格 | 了解他們實際的架構和用語 | 面試前 1 到 2 週讀 |

前一份報告提到 Exponent 的模擬面試。研究當日 tryexponent.com 會轉址到 aced.io，品牌改為「Aced（formerly Exponent）」。

## 4. 行為與領導力面試

### 4.1 評估什麼

行為面試有兩個目的：一是判斷你的行為能不能預測你在這家公司的表現，二是判斷你的職級。第二點對高級候選人特別重要。

**Meta 的 8 個面向。** 來源是 interviewing.io 上一位前 Meta 工程主管的文章（他做過 150 場以上的行為面試），不是官方文件。

1. 動機
2. 主動性
3. 在缺乏結構的環境中工作
4. 毅力
5. 解決衝突
6. 同理心
7. 成長
8. 溝通

面試約 45 分鐘：開場 5 分、討論過去的工作 35 分、候選人提問 5 分。通常問 5 到 6 題，每題深入追問，一題可以同時提供好幾個面向的訊號。

**Meta 如何用 scope 判斷職級**（同一來源）：

| 面向 | 初階（IC4） | 高級（IC5） | Staff（IC6） |
| --- | --- | --- | --- |
| 主動性 | 影響自己的工作範圍 | 影響整個團隊，通常 3 人以上 | 影響整個組織，通常 2 個團隊以上 |
| 缺乏結構的環境 | 在團隊內幾位利害關係人之間取得共識 | 跨團隊或組織內，3 人以上 | 跨組織，2 個團隊以上 |
| 衝突 | 和一位同事對實作細節有不同意見 | 和幾位同事或負責人對專案方向有不同意見 | 2 個團隊以上對方向有不同意見 |
| 成長 | 學一項新技術 | 學一項影響整個團隊的技能 | 學一項影響 2 個團隊以上的技能 |

錄取高級的條件，是 8 個面向幾乎都有正面訊號，而且 scope 達到高級。只達到高級 scope 的 staff 候選人，可能拿到「staff 不錄取、高級錄取」的結果。

**Amazon 的 16 條領導力準則**（amazon.jobs 官方列表）：

1. Customer Obsession
2. Ownership
3. Invent and Simplify
4. Are Right, A Lot
5. Learn and Be Curious
6. Hire and Develop the Best
7. Insist on the Highest Standards
8. Think Big
9. Bias for Action
10. Frugality
11. Earn Trust
12. Dive Deep
13. Have Backbone; Disagree and Commit
14. Deliver Results
15. Strive to be Earth's Best Employer
16. Success and Scale Bring Broad Responsibility

Amazon 官方的建議：

- 用 STAR 回答。
- 講「我」不講「我們」，官方稱這是最關鍵的建議之一。
- 用可量化的結果。
- 每一條準則都準備多個例子。
- 官方說所有準則同等重要。

一位 Amazon 招募人員建議準備一份 8 到 9 個強例子的文件。

**Google 的 Googleyness & Leadership。** Business Insider 看過一份內部文件，報導 Google 會在這一輪加入「依候選人過去作品進行的技術設計討論」（未核實，二手來源）。如果目標是 Google，至少要有 1 到 2 個故事能講到架構層級的細節。

### 4.2 故事的結構

| 結構 | 組成 | 什麼時候用 |
| --- | --- | --- |
| STAR | Situation、Task、Action、Result | 基本款，Amazon 官方推薦 |
| STAR(R) | 在 STAR 之後加 Reflection | Tech Interview Handbook 的建議，用來展現職級和成長 |
| CARL | Context、Actions、Results、Learnings | Hello Interview 的版本，把 Situation 和 Task 合成 Context，縮短開場 |

**本報告建議的時間比例：**

| 段落 | 比例 | 內容 |
| --- | --- | --- |
| 背景 | 約 20% | 只講到聽得懂為止：規模、利害關係人、為什麼困難 |
| 行動 | 約 50% | 你做的決定、你考慮過的選項、你怎麼說服別人。Tech Interview Handbook 指出行動才是「可重複的行為」。 |
| 結果 | 約 20% | 數字、對誰有影響 |
| 反思 | 約 10% | 學到什麼、現在會怎麼做不同 |

第一次回答控制在 2 到 3 分鐘，把細節留給追問。這個長度是本報告的建議，不是來源給的數字。

### 4.3 準備 6 到 8 個故事的步驟

各來源給的數量不同：

| 來源 | 建議數量 |
| --- | --- |
| Tech Interview Handbook | 3 到 5 個高影響、高複雜度、你高度參與的專案 |
| Amazon 招募人員 | 8 到 9 個例子 |
| Hello Interview | 幾個來自大型專案的「關鍵故事」，就能回答大部分的題目 |

所以本報告的建議是：從 3 到 5 個專案裡，切出 6 到 8 個故事。

**步驟：**

1. **盤點（1 小時）。** 列出最近 3 到 5 年的 10 到 15 個候選經歷：專案、事故、衝突、帶人、失敗、跨團隊合作。翻自己的績效考核、設計文件、事故報告，把數字找回來。
2. **篩選。** 依 Hello Interview 的優先順序挑：scope 最大的 → 和這個職位最相關的 → 最獨特的 → 最近的。
3. **填矩陣。** 列是故事，欄是目標公司的訊號（見第 4.4 節）。每個訊號至少要有 2 個故事可以用，因為面試中同一個故事不能講兩次。哪一欄空著，就回到步驟 1 補。
4. **寫成重點，不寫成稿子。** 每個故事一張卡：背景一句話、2 到 3 個關鍵決策、結果的數字、反思一句話，加上可能的追問。記的是事實和決策，不是句子。
5. **大聲講、錄下來。** 每個故事練兩種長度：2 到 3 分鐘的完整版、30 秒的摘要版。
6. **校準職級。** 檢查每個故事的 scope 用語：影響了誰、多少人、幾個團隊、多大的業務。Tech Interview Handbook 的高級候選人頁面提醒，高級候選人要自己主動講出 scope，面試官不太會幫你挖。

**另外要準備的固定題：**

- 「介紹一下你自己」：90 秒，以和這個職位相關的主線組織。
- 「你最有成就感的專案」：通常就是故事 1。
- 「為什麼想來這家公司」。
- 「你想問我們什麼」：準備 3 到 5 個，依面試官的角色調整。

Hello Interview 估計：初次準備行為面試約需 1 到 2 週；如果故事已經選好，幾天就夠。

### 4.4 故事槽位與對應矩陣

**建議的 8 個故事槽位**（本報告整理）：

| # | 故事槽位 | 主要證明的訊號 | 高級版本的 scope 例子 |
| --- | --- | --- | --- |
| 1 | 最有影響力的專案 | 動機、交付成果、技術深度 | 主導一個跨團隊依賴的系統改版 |
| 2 | 沒有職權卻推動了改變 | 主動性、影響力 | 說服其他團隊採用新的部署流程 |
| 3 | 不同意，然後怎麼處理 | 衝突、Disagree and Commit | 和其他團隊的負責人對 API 方向有分歧 |
| 4 | 失敗或犯錯 | 成長、Ownership、Earn Trust | 自己的設計造成事故，以及事後改了什麼制度 |
| 5 | 需求模糊、從零開始 | 缺乏結構的環境、Think Big | 沒有規格的新產品線，自己定義第一版範圍 |
| 6 | 帶人或提升團隊 | Hire and Develop the Best、同理心 | 帶新人上手、建立團隊的 code review 標準 |
| 7 | 艱難的技術取捨 | Are Right A Lot、Dive Deep | 在一致性和延遲之間做選擇，並且說服反對者 |
| 8 | 壓力下交付或處理事故 | 毅力、Bias for Action、Deliver Results | 事故指揮、期限前砍範圍 |

**矩陣範例。** 欄位換成目標公司的訊號。面 Amazon 就用 16 條準則，面 Meta 就用 8 個面向。● 表示主要訊號，○ 表示次要訊號。

| 故事 | 主動性 | 缺乏結構 | 毅力 | 衝突 | 同理心 | 成長 | 動機 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1. 支付系統改版 | ● | ○ |  |  |  |  | ● |
| 2. 推動共用 CI 流程 | ● |  |  | ○ | ○ |  |  |
| 3. 和平台團隊的 API 爭議 |  |  |  | ● | ● |  |  |
| 4. 快取設計造成的事故 |  |  | ○ |  |  | ● |  |
| …… |  |  |  |  |  |  |  |

### 4.5 追問與常見錯誤

**面試官常見的追問。** 本報告整理，取自 interviewing.io 的範例題目，以及「深入挖細節」的描述：

- 為什麼是你負責？當時還有誰？
- 你考慮過哪些做法？為什麼沒選？
- 對方怎麼看？最後他同意了嗎？
- 怎麼量測結果？數字是多少？
- 如果重來，你會怎麼做不同？

**常見錯誤：**

| 錯誤 | 來源 |
| --- | --- |
| 講「我們」，聽不出你個人的貢獻 | Amazon、Hello Interview |
| 背景講太久，行動講太少 | Hello Interview |
| 選錯故事：scope 太小，或跟問題不相關 | Hello Interview |
| 初階也能講的故事，在高級面試中變成負面訊號 | Tech Interview Handbook |
| 以為不用準備，或以為要背題庫 | Hello Interview |
| 以為 STAR 就夠了 | Hello Interview |
| 結果沒有數字、沒有反思 | Amazon、Tech Interview Handbook |
| 被打斷或被帶去細節後就亂了 | Tech Interview Handbook |

### 4.6 不背稿的練習方式

- **隨機抽題。** 準備一份 30 題左右的題庫，例如 Tech Interview Handbook 的題目清單。每次隨機抽 3 題，當場決定用哪個故事。這等於行為面試的交錯練習。
- **請 LLM 扮演面試官。** 給它職位描述和目標公司的價值觀，要求它每個回答至少追問 2 次，最後依 scope 和「我」的比例給回饋。Tech Interview Handbook 建議用 LLM 練習並取得回饋。
- **找人模擬至少 2 次。** 最好找做過面試官的人。
- **先講結論。** 第一句話就說出結果，例如「我把部署失敗率從 X 降到 Y」，再回頭補背景。

## 5. LLD/OOD

### 5.1 誰考、怎麼考

Hello Interview 說 LLD 和 OOD「是同一種面試，只是名稱不同」。

| 項目 | 內容 |
| --- | --- |
| 長度 | 約 35 分鐘 |
| 形式 | 在白板上設計類別、介面、關係，再實作關鍵方法 |
| 地區差異 | 美國 Big Tech 偏向用真實語言（Java、Python、C++）寫部分程式碼；印度與亞洲偏向結構化的虛擬碼 |
| 語言 | 用你每天在寫的語言，不要為了一場面試換語言 |
| UML | 不必畫。面試官要求時，先確認要畫到什麼程度 |

**哪些公司考。** 以下全是二手來源或候選人的心得，未核實。

| 公司 | 回報的形式 |
| --- | --- |
| Flipkart | machine coding 約 90 分鐘，要寫出能執行的程式；另有一輪 LLD |
| Microsoft | SDE II 的回合中有 LLD，題目是多樓層停車場（2026-03 的 LeetCode 心得） |
| Amazon | 白板 OOD |
| Uber | 有 machine coding 和並行程式的回合（資料偏舊） |

Hello Interview 的建議是直接問招募人員或面試官，他們期待什麼形式。

### 5.2 框架

依 Hello Interview 的 LLD Delivery Framework：

| 步驟 | 時間 | 做什麼 |
| --- | --- | --- |
| 1. 需求 | 約 5 分 | 問清楚主要功能、規則和結束條件、錯誤處理、範圍。把需求和不做的範圍寫在白板上。 |
| 2. 實體與關係 | 約 3 分 | 從需求裡挑名詞。會保存變動狀態或執行規則的，才是實體。畫出擁有和使用的關係。 |
| 3. 類別設計 | 10 到 15 分 | 從協調者（orchestrator）開始，定義每個類別的狀態和行為，每一項都對應到某條需求 |
| 4. 實作 | 約 10 分 | 先寫正常路徑，再處理邊界情況，最後追一個情境驗證（1 到 2 分鐘）。先問面試官要寫到多細。 |
| 5. 擴充性 | 約 5 分 | 時間和職級允許時才做，通常由面試官帶 |

面試官把你拉離這個框架時，跟著他走。

### 5.3 評估什麼、各職級的差異

**評估面向（Hello Interview）：**

1. 問題分析：先理解問題，不急著寫程式。
2. 類別設計：職責、方法簽章、擁有關係、清楚的邊界。
3. 程式品質：封裝、狀態管理、合理使用組合和繼承、關注點分離、命名、依賴方向。
4. 擴充性與可維護性：設計容易調整就好，不是預先應付所有可能的未來。
5. 溝通：有條理的說明，被追問時能調整。Hello Interview 認為這常比完美的程式碼更能顯示成熟度。

**幾條設計原則：**

- 規則放在擁有那份狀態的實體裡，也就是 Tell, Don't Ask。
- 流程規則放在協調者；某筆資料自己的規則放在那筆資料的實體。
- 組合優於繼承；簡單的狀態優於很深的繼承階層。

**職級差異**（擴充性討論）：

| 職級 | 擴充性討論 |
| --- | --- |
| 初階 | 很少或沒有 |
| 中階 | 1 到 2 個小的追問 |
| 高級 | 連續好幾個「如果我們要……」 |

### 5.4 題目 → 考點對照表

題目取自 Hello Interview 和 awesome-low-level-design 的清單。依「主要考點」分組是本報告的整理。

| 考點 | 代表題 | 常用的設計 |
| --- | --- | --- |
| 狀態機 | 販賣機、咖啡機、電梯、紅綠燈、ATM | State 模式或明確的狀態列舉加轉移表 |
| 可替換的演算法或計費規則 | 停車場（計費、車位分配）、速率限制器（不同演算法）、電梯排程 | Strategy |
| 事件通知 | Pub/Sub、通知系統、拍賣出價 | Observer |
| 遊戲規則與回合 | 井字遊戲、Connect Four、西洋棋、蛇梯棋 | 把棋盤、規則、玩家、回合分開；勝負判斷放在規則或棋盤 |
| 資料結構設計 | LRU 快取、檔案系統、記錄框架 | 雜湊表加雙向鏈結串列；Composite（檔案系統）；Chain of Responsibility（記錄等級） |
| 預約與並行 | 電影訂票、飯店、租車、Amazon Locker | 座位或資源的鎖定與過期；並行控制 |
| 帳本與分帳 | Splitwise、數位錢包 | 交易紀錄不可變；餘額由紀錄推導 |
| 物件建立 | 有很多選配的訂單或設定 | Builder、Factory |

**高級職位的加分題：並行。** 例如執行緒安全的 LRU、帶 TTL 的快取、阻塞佇列、多執行緒的速率限制器。awesome-low-level-design 另外列了 9 題並行程式題。

### 5.5 設計模式與原則

- **最常用到的模式：** Strategy、State、Observer、Factory、Builder，其次是 Decorator、Command、Composite。Hello Interview 的例子提到 Factory、Strategy、Singleton、Builder。
- **Singleton 要小心用。** 它是全域狀態，會讓測試變困難。用之前要能說出為什麼需要。
- **原則：** SOLID、DRY、KISS、YAGNI、組合優於繼承。
- **不要硬套模式。** Hello Interview 說美國 Big Tech 的面試官在意的是理由，不是術語，並且警告不要「為了用模式而用模式」，它的說法是「務實勝過崇拜模式」。印度和亞洲的中小型公司比較會直接問模式。

### 5.6 練習方式

- **先問清楚要不要準備。** 目標公司不考 LLD 的話，只要讀一次框架、做 1 到 2 題，當作系統設計和 OOP 的複習。
- **每題計時 35 分鐘**，照第 5.2 節的 5 個步驟做。做完再對照解答。
- **變體練習：** 做完一題後抽一個「如果……」，例如停車場要支援電動車充電、要支援預約、要支援多種計費方式。檢查要改幾個類別。改動範圍小，代表職責劃分得好。
- **machine coding 型的公司**（例如 Flipkart）：要練到 90 分鐘內寫出能執行、有測試的程式。可以把題目加進自己慣用的練習專案，例如前一份報告的 [cpp-practice](../coding-interview-practice-research/cpp-practice/README.zh-TW.md)。
- **題庫：** ashishps1/awesome-low-level-design（28,071 星，GPL-3.0）列了 33 題（7 題簡單、15 題中等、11 題困難）加 9 題並行題。多數講解連到 algomaster.io，那是販售付費課程的網站。

## 6. AI 輔助程式回合

### 6.1 各公司比較

| 項目 | Meta | Canva | Google | HackerRank Chakra |
| --- | --- | --- | --- | --- |
| 開始 | 2025-10 開始推出 | 2025-06-11 公告 | 2026 下半年試行 | 2026-10-05 正式上線，之前約 6 個月 beta |
| 適用 | SWE 與 EM，到 E7、M2（未核實） | 後端、前端、ML 工程師 | 初中階職位、美國部分團隊（未提到高級職位） | 採用它的雇主，例如 Snowflake、Snorkel、Capgemini 參與了 beta |
| 取代什麼 | onsite 兩場程式回合的其中一場 | 後端與前端的 CS Fundamentals 篩選 | 不是取代，而是在「code comprehension」回合開放使用 AI | 廠商說可以把招募電話篩選、帶回家作業、工程師追問合成一場 |
| 環境 | CoderPad，多檔案程式庫，內建 AI 聊天 | 候選人自己的 AI IDE，例如 Copilot、Cursor、Claude | Gemini | 預先建好的程式庫或行內編輯器，內建 AI 助手 |
| 任務 | 修 bug → 實作功能 → 依更大的測資優化 | 模糊、貼近產品的題目，一個 prompt 解不完 | 閱讀、除錯、優化既有程式 | 在程式庫中處理類似客服工單的任務：規劃、實作、說明 |
| 評估 | 問題解決、程式品質、驗證、溝通（未核實） | 拆解問題、技術判斷、找出並修正 AI 程式的問題、達到上線品質 | AI 熟練度：提示、驗證輸出、除錯 | 問題解決、判斷、溝通、AI 熟練度 |
| 來源品質 | 候選人回報和第三方整理 | 公司工程部落格（一手） | Business Insider 依內部文件報導；Google 確認使用 Gemini | TechCrunch 報導、廠商文件 |

**其他公司。** Hello Interview 的 AI 輔助回合指南把 Shopify、LinkedIn、Uber、Rippling 也列為有這類回合的公司（未核實）。

### 6.2 Meta

以下多數來自 Hello Interview 整理的候選人回報。Meta 沒有公開官方說明，所以全部視為未核實。

**流程與環境：**

- 60 分鐘。onsite 原本有兩場程式回合，其中一場改成 AI 形式，另一場仍是不能用 AI 的傳統題。招募人員會事先告訴你哪一場是哪一種。
- interviewing.io 在 2025-10 推出前報導，電話篩選的演算法題保留不變。
- 環境是 CoderPad，分成三欄：左邊是檔案總覽，中間是編輯器，右邊是 AI 聊天和題目說明。
- AI 看得到專案檔案，但只能在聊天裡回答，不能直接改檔案，所以程式碼要自己打。
- 語言：Java、C++、C#、Python、Kotlin、TypeScript。
- 可以切換的模型：GPT-4o mini、GPT-5、Claude Haiku 3.5/4.5、Claude Sonnet 4/4.5、Claude Opus 4、Gemini 2.5 Pro、Llama 4 Maverick。這是 Hello Interview 文章當時的清單，可能已經改變。
- 招募人員會在面試前提供練習環境，裡面有一題叫「the puzzle」的範例，據說比多數正式題目難。

**三個階段：**

| 階段 | 內容 | 注意 |
| --- | --- | --- |
| 開場 | 約 5 到 6 分鐘，面試官介紹檔案和怎麼跑測試 | 扣掉開場，實作和優化大約剩 30 到 40 分鐘 |
| 1. 修 bug | 多半不是演算法錯誤，而是型別轉換、off-by-one、條件寫錯。也可能要你解釋測試、說出現有程式用的演算法和複雜度。 | 有些面試官這階段不准用 AI。Hello Interview 建議這階段不用 AI。 |
| 2. 核心實作 | 明確允許並預期使用 AI。難度據說高於 LeetCode Medium，約 120 行以上。 | 先自己想好演算法、講出來，再請 AI 寫 |
| 3. 優化 | 用更大的分級測資打掛第 2 階段的解法。最佳做法常取決於資料的形狀，有時要換演算法，例如從貪婪改成 trie、從 DFS 改成 bitmask。 | 有候選人沒做完第 3 階段仍拿到 offer |

候選人回報過的題目有：迷宮求解並印出路徑、從單字清單中最大化不重複字元、找出總和為 15 的三張牌、好友推薦。據說題庫約 9 題（未核實）。

**評估：**

- 據說和傳統程式回合相同的四項：問題解決、程式品質（包括你懂不懂 AI 產生的程式）、驗證、溝通。
- 一份被轉述的 Meta 內部訊息寫著：應該用 AI，但要證明你懂這些程式。
- Hello Interview 整理的節奏：提示 → 檢查 → 執行 → 確認 → 前進。

**常見的失敗方式：**

- 太依賴 AI。有候選人收到「看起來非常依賴 AI」的負面回饋。
- 直接問 AI「這題怎麼解」。
- 跳過讀程式的階段，直接動手修。
- 複製 AI 的輸出卻不懂它在做什麼。
- 檢查 AI 程式花的時間，比自己寫還久。
- 下 prompt 時不出聲，事後才補說明。

**AI 被削弱的回報。** 有候選人說正式面試時 AI 一直給錯答案，練習時卻很正常。Hello Interview 推測是系統提示被修改過，但這只是推測，也有人沒遇到。所以要練到 AI 不可靠時也能自己完成。

### 6.3 Canva

來源是兩篇 Canva 工程部落格文章：2025-06-11 Simon Newton 的公告，以及 2025-10-20 Karl Hörnlund 從面試官角度寫的指南。

- **為什麼改：** 舊的形式要求候選人在沒有工作中會用的工具下寫程式。當時 Canva 將近一半的前端和後端工程師，每天都在用 AI 輔助寫程式的工具。
- **題目：** 刻意設計成模糊、貼近真實、一個 prompt 解不完。例如用「為繁忙機場設計起降管制系統」取代「實作康威生命遊戲」。
- **CS 基礎仍然會評**，而且工程師要為自己產出的程式負完全的責任。

**強的面試分成三段：**

| 階段 | 做什麼 |
| --- | --- |
| 規劃 | 釐清需求和範圍、講出假設、先畫架構再寫程式 |
| 開發 | 用 AI 產生骨架、查不熟的 API、處理例行工作；遇到複雜或不熟的建議時停下來評估；目標是能直接送 PR 的程式 |
| 討論 | 解釋每個技術決定、還能怎麼改進、上線前還缺什麼、怎麼擴充 |

**三種失分的模式：**

1. **AI 秀場**：重點放在展示提示技巧或最新的 AI 功能，而不是解題。
2. **功能馬拉松**：想做越多功能越好，快速接受 AI 的輸出。
3. **完全放手**：把 AI 當成完整解答的產生器，而不是加速工具。

**準備建議：**

- 事先準備好自己的 AI IDE，建好基本的專案結構，不要在面試中花時間設定環境。
- 平常就用 AI 助手，了解它的能力和限制。Canva 發現 AI 經驗很少的候選人常常表現不好，原因不是不會寫程式，而是缺乏引導 AI、看出爛建議的判斷力。
- 面試官問問題時，先給出你自己的答案，再去問 AI。

### 6.4 Google

| 項目 | 內容 | 可信度 |
| --- | --- | --- |
| 試行存在、使用 Gemini | Google 發言人向 Business Insider 確認；招募副總裁 Brian Ong 發表聲明 | 公司確認 |
| 回合名稱和內容 | 「code comprehension」：在既有的程式庫裡閱讀、除錯、優化 | Business Insider 依內部文件報導 |
| 評估 | AI 熟練度，包括提示工程、驗證輸出、除錯 | 同上 |
| 範圍 | 初中階職位、美國部分團隊、2026 下半年開始；成功的話擴大到更多團隊和地區 | 同上 |
| 其他改變 | Googleyness & Leadership 加入依候選人過去作品的技術設計討論；初階職位以一場開放式工程問題取代一場傳統技術面試 | 同上 |
| 高級職位 | 報導沒有提到 | — |
| 擴大範圍 | 一篇二手報導說 Business Insider 在 2026-10-20 報導這個形式已擴及公司更多部門 | 未核實，只找到單一二手來源 |

前一份報告第 2 節提到，Google 也在恢復至少一場現場面試。對高級候選人的意義是：短期內仍以傳統回合和現場白板為主，但閱讀陌生程式、用 AI 除錯的能力，已經被 Google 明確列為評分項目。

### 6.5 HackerRank Chakra

| 項目 | 內容 | 來源 |
| --- | --- | --- |
| 上線 | 約 6 個月 beta 後，2026-10-05 正式上線；beta 期間超過 50 萬場面試 | TechCrunch（數字是公司說法） |
| 候選人的體驗 | 在預先建好的程式庫中處理類似客服工單的任務，用 AI 規劃修法、實作、在 review 階段說明做法。程式庫涵蓋 MERN、Spring Boot、Django、Go、.NET。也有行內編輯器，邊寫程式邊和 Chakra 對話。 | 2026-07 版本說明 |
| 追問 | 依你的實作追問：為什麼選這個做法、多一個限制會怎麼改 | TechCrunch |
| AI 熟練度 | 定義是你怎麼把問題交代給 AI、怎麼判斷它的輸出、怎麼引導它走向解答。評分依據是 IDE 裡的實際操作，不只看聊天。**AI 互動太少，就不給 AI 熟練度分數。** | TechCrunch、2026-07 版本說明 |
| 監考 | 接第二個螢幕、分享螢幕、離開全螢幕時，Chakra 會暫停回應；桌面 App 會鎖定畫面、限制其他程式；另有視線偵測（有限開放） | 2026-07 版本說明 |
| 最終決定 | 執行長說最後的錄取決定仍由人做 | 公司說法 |
| 作弊率 | 執行長說 Chakra 面試中的可疑行為標記比傳統測驗少 70% 到 80% | 公司說法，未核實，沒有公布方法 |
| 爭議 | TechCrunch 指出演算法可能繼承或放大偏見；紐約市要求某些自動化招募工具做獨立的偏見稽核並通知候選人 | TechCrunch |

**對候選人的意義：**

- 不能整場都不用 AI，否則拿不到 AI 熟練度分數。
- 也不能整段請 AI 生成。它看的是規劃、驗證、說明的過程。
- 練習時要習慣邊做邊對著麥克風解釋，因為面試官是 AI，會依你剛才的操作追問。

### 6.6 共通的作答流程

本報告依 Meta、Canva、Hello Interview 的建議整合：

| 步驟 | 動作 | 時間（60 分鐘的場次） |
| --- | --- | --- |
| 1. 讀程式庫 | 找出進入點、資料模型、測試。先跑一次測試，看哪些會失敗。 | 5 到 10 分 |
| 2. 先講方案 | 用自己的話說出問題和打算的做法、複雜度。用 AI 執行決策，不讓 AI 做決策。 | 2 到 3 分 |
| 3. 給具體的 prompt | 寫出檔名、函式簽章、限制、目標複雜度。例如「實作一個支援插入和前綴搜尋的 trie 類別……」，而不是「修好這個」。 | 每次 1 分 |
| 4. 每次產出都驗證 | 逐行讀、執行、測邊界情況，確認後才前進 | 持續 |
| 5. 除錯先靠自己 | 先自己讀錯誤訊息、加 print 或設中斷點；真的需要才問 AI | 依需要 |
| 6. 持續講話 | 下 prompt 之前先講你要做什麼，讀 AI 的輸出時唸出重點 | 持續 |
| 7. 收尾 | 說明取捨、還沒做完的部分、上線前還需要什麼（測試、監控、錯誤處理） | 3 到 5 分 |

**反模式對照表：**

| 反模式 | 來源 | 改成 |
| --- | --- | --- |
| 問 AI「這題怎麼解」 | Meta 候選人回報 | 自己決定演算法，再請 AI 寫 |
| AI 秀場、功能馬拉松、完全放手 | Canva | 一次一個功能，每次驗證，保持主導 |
| 複製貼上卻不懂 | Meta | 逐行讀，說得出每一段在做什麼 |
| 下 prompt 時沉默 | Hello Interview | 先講再做 |
| 完全不用 AI | Canva、Chakra | 例行程式碼、樣板、查 API 交給 AI |
| 被問問題就先問 AI | Canva | 先給自己的答案 |

### 6.7 練習方式

**每週一次 60 分鐘的三階段練習**（本報告設計，模仿 Meta 的結構）：

1. **選程式庫。** 找一個沒碰過、有測試的開源專案，或是 Hello Interview 的 AI coding 練習題（例如 Maze Solver、Spell Checker、Task Scheduler、Inventory Packer）。
2. **第 1 階段，修 bug（15 分）。** 請別人或 LLM 在程式裡埋一個型別或邊界的 bug。這一段不用 AI 修。
3. **第 2 階段，加功能（25 分）。** 先寫下演算法和複雜度，再用 AI 實作。
4. **第 3 階段，優化（15 分）。** 把測資放大 100 倍，找出瓶頸，必要時換演算法。
5. **錄影回放（5 分）。** 檢查三件事：有沒有在 prompt 之前先講方案、有沒有每次都執行驗證、AI 寫錯時多久發現。

**刻意製造的條件：**

- **只能用聊天、不能讓 AI 改檔案。** 這和 Meta 的環境一樣，可以關掉 agent 模式或行內補全來模擬。
- **換比較弱的模型。** 練習 AI 不可靠時的應對。
- **限制語言。** 用面試時會選的語言。

**不要放掉無輔助的能力。** 傳統程式回合和現場白板仍然存在。前一份報告的 cpp-practice 專案在工作區關閉了 AI 行內補全，適合練無輔助的部分；AI 練習請另外開一個工作區。

**正式面試前：**

- 如果目標公司提供練習環境（例如 Meta 的「the puzzle」），一定要用，熟悉版面、模型切換、怎麼跑測試。
- 收到 HackerRank 或 Chakra 的邀請時，先確認是否允許 AI、是哪一種模式、需不需要裝桌面 App。

## 7. 時間分配與週計畫

### 7.1 時間分配

比例沿用前一份報告第 6.1 節，這裡換算成每週時數。假設每週 18 到 20 小時：平日每天約 2 小時，週末每天 4 到 5 小時。

| 項目 | 比例 | 每週時數 | 怎麼排 |
| --- | --- | --- | --- |
| 系統設計 | 35–40% | 約 7 小時 | 平日 2 次 × 1 小時讀概念；週末 1 次 45 分鐘的完整練習加複盤，或 1 次模擬面試 |
| DSA | 25–35% | 約 6 小時 | 沿用前一份報告第 6.2 節：每天 60 到 90 分鐘 |
| AI 輔助程式 | 10–15% | 約 2.5 小時 | 第 3 週起，每週 1 次 60 分鐘的三階段練習加回放 |
| 行為與 LLD/OOD | 10–15% | 約 2.5 小時 | 行為：前 4 週寫故事，之後每週講 3 個；LLD：目標公司有考時每週 1 題 |

每週少於 18 小時時，等比例縮減，但對高級職位，系統設計的時數不要低於 DSA。Hello Interview 估計：系統設計沒基礎的人要 3 到 4 週，有經驗的人不到 1 週可以複習完。Tech Interview Handbook 估計：DSA 最低限度約 30 小時，充分準備約 100 小時。

### 7.2 12 週計畫：和 DSA 並排

DSA 欄抄自前一份報告第 6.2 節，方便對照。其餘欄位是本報告的計畫。

| 週 | DSA（前一份報告） | 系統設計 | 行為 | LLD/OOD | AI 輔助 |
| --- | --- | --- | --- | --- | --- |
| 1–2 | 陣列、雜湊、雙指針、滑動視窗、前綴和、堆疊、二分搜尋 | 讀框架（第 3.2 節）、估算（第 3.3 節）、核心概念：網路、API、資料模型、快取、分片、一致性雜湊、CAP、索引。做 Bitly、Dropbox。 | 盤點 10 到 15 個經歷，找回數字 | 問招募人員要不要考 | 選定工具，建好練習用的工作區 |
| 3–4 | 鏈結串列、樹、Trie、堆積、回溯 | 關鍵技術：Redis、Kafka、Postgres、Cassandra 或 DynamoDB、Elasticsearch。做 Ticketmaster、News Feed、WhatsApp、Rate Limiter。 | 選出 6 到 8 個故事，填矩陣（第 4.4 節），寫成重點卡 | 讀框架；做停車場、LRU 快取 | 第 1 次三階段練習 |
| 5–6 | 圖、1D 與 2D DP | 做 YouTube、Uber 或 Yelp、Web Crawler；第 1 次系統設計模擬面試 | 大聲講、錄音；第 1 次行為模擬面試 | 電梯、電影訂票（含並行） | 每週 1 次；開始只用聊天、不讓 AI 改檔 |
| 7–8 | 貪婪、區間、位元；改為交錯練習，加入 Hard | 做 Top K、Ad Click Aggregator、Payment System、Google Docs；重做第 1 到 4 週的題目並改一個條件；第 2、3 次模擬面試 | 隨機抽題練習；強化失敗和衝突故事 | 1 到 2 題，加上擴充追問 | 換弱模型；計時 |
| 9–12 | 公司標籤題、補弱點 | 目標公司的產品題；Meta 決定 Product Architecture 或 System Design；每週 1 到 2 次模擬面試 | 依目標公司的價值觀重填矩陣（Amazon 每條準則 2 個例子）；準備自我介紹和「為什麼是這家公司」 | 有考才維持每週 1 題 | 使用目標公司的練習環境；確認 HackerRank 或 Chakra 的模式 |

### 7.3 時間只剩 3 到 4 週時

- **系統設計：** 框架，加第 3.5 節的前 8 題，加 2 次模擬面試。Hello Interview 也建議時間很少時，先學 Delivery Framework、快速看過 Key Technologies，再讀 Core Concepts；有需要時向招募人員要求延後。
- **行為：** 直接寫 6 個故事，每個訊號 1 個故事，至少做 1 次模擬。
- **AI 輔助：** 目標公司有這個回合時，至少做 2 次三階段練習，並使用官方的練習環境。
- **DSA：** 改成只做前一份報告的公司標籤題和到期的複習。

### 7.4 準備好了的標準

本報告的建議：

| 回合 | 標準 |
| --- | --- |
| 系統設計 | 第 3.5 節的題目沒看解答也能在 45 分鐘內交出完整設計；前 10 分鐘內說得出關鍵難點；連續 2 次模擬面試都過 |
| 行為 | 隨機抽 3 題都能在 10 秒內選好故事；每個故事都講得出數字和反思；每個訊號至少有 2 個故事可用 |
| LLD | 35 分鐘內完成類別設計和關鍵方法；面試官連問 3 個「如果……」時，修改範圍都不大 |
| AI 輔助 | 三階段練習能在 60 分鐘內完成前兩個階段；回放時每段 AI 產出之後都有執行驗證 |

## 8. 未核實與限制

- **Meta AI 回合：** 細節多來自候選人回報和 Hello Interview 的整理。包括適用職級（到 E7、M2）、「E7 以上只有 AI 回合」、模型清單、三階段結構、約 9 題的題庫、AI 被削弱的推測。Meta 沒有公開官方說明。
- **Google：** 試行內容來自 Business Insider 對內部文件的報導，Google 只確認了試行存在和使用 Gemini。2026-10 擴大的說法只找到單一二手來源。報導沒有提到高級職位。
- **Chakra：** 50 萬場、作弊標記少 70% 到 80%、人做最終決定，都是公司說法。
- **Hello Interview：** 職級期望、框架時間、題目難度，是業界經驗談。Hello Interview、interviewing.io、ByteByteGo 都販售課程或模擬面試，有利益關係。
- **Meta 的 8 個行為面向和 scope 對照**，來自一位前員工的文章，不是官方文件。
- **LLD：** 哪些公司考、什麼形式，全部來自備考網站和候選人心得。
- **其他公司的 AI 回合：** Shopify、LinkedIn、Uber、Rippling 的說法來自 Hello Interview 的指南。
- **本報告自己的建議：** 第 3.4 節的對照表、第 3.5 節的選題、第 4.2 節的時間比例、第 4.4 節的故事槽位、第 6.7 節的練習設計、第 7 節的時數與週計畫、第 7.4 節的標準，都是綜合各來源的建議，沒有原始資料直接支持這些數字。
- **延遲數字**是 2010 年的數量級，不能拿來推算現代硬體的實際效能。
- **DDIA 第 2 版的出版日期**，各零售商標示不一致，從 2026-02 到 2026-03 都有。
- **其他未讀到的內容：** 部分網站的付費內容沒有讀到，例如 Hello Interview 的 Numbers to Know 與 Patterns 詳細頁。

## 9. 參考資料

**系統設計**

- Hello Interview，Delivery Framework：https://www.hellointerview.com/learn/system-design/in-a-hurry/delivery
- Hello Interview，How to prepare：https://www.hellointerview.com/learn/system-design/in-a-hurry/how-to-prepare
- Hello Interview，Introduction：https://www.hellointerview.com/learn/system-design/in-a-hurry/introduction
- Hello Interview，各職級的期望：https://www.hellointerview.com/blog/the-system-design-interview-what-is-expected-at-each-level
- Hello Interview，Staff 系統設計：https://www.hellointerview.com/blog/staff-level-system-design
- ByteByteGo，Back-of-the-envelope estimation：https://bytebytego.com/courses/system-design-interview/back-of-the-envelope-estimation
- Martin Kleppmann，DDIA 第 2 版：https://martin.kleppmann.com/2026/03/24/designing-data-intensive-applications-2e.html
- O'Reilly，DDIA 第 2 版：https://www.oreilly.com/library/view/-/9781098119058/
- ByteByteGo，GenAI System Design Interview：https://bytebytego.com/courses/genai-system-design-interview/introduction-and-overview
- donnemartin/system-design-primer：https://github.com/donnemartin/system-design-primer
- ByteByteGoHq/system-design-101：https://github.com/ByteByteGoHq/system-design-101
- karanpratapsingh/system-design：https://github.com/karanpratapsingh/system-design
- Meta Product Architecture 與 System Design 的比較（第三方）：https://igotanoffer.com/en/advice/meta-product-architecture-interview 、https://www.designgurus.io/blog/meta-system-design-vs-product-design
- Hello Interview，Over-engineering is getting you down-levelled：https://www.hellointerview.com/blog/over-engineering-is-getting-you-down-levelled
- The Pragmatic Engineer，The reality of tech interviews：https://newsletter.pragmaticengineer.com/p/the-reality-of-tech-interviews

**行為面試**

- Amazon，Leadership Principles：https://www.amazon.jobs/content/en/our-workplace/leadership-principles
- About Amazon，Leadership Principles 面試建議：https://www.aboutamazon.com/news/workplace/amazon-leadership-principles-interview
- Amazon 招募人員的面試建議：https://advertising.amazon.com/en-ca/careers/re-exploring-careers/amazon-interview-preparation
- Tech Interview Handbook，Behavioral interview：https://www.techinterviewhandbook.org/behavioral-interview/
- Tech Interview Handbook，高級候選人：https://www.techinterviewhandbook.org/behavioral-interview-senior-candidates/
- Hello Interview，Behavioral：https://www.hellointerview.com/learn/behavioral/overview/introduction
- interviewing.io，Meta 的行為面試如何評估：https://interviewing.io/blog/how-software-engineering-behavioral-interviews-are-evaluated-meta

**LLD/OOD**

- Hello Interview，LLD Introduction：https://www.hellointerview.com/learn/low-level-design/in-a-hurry/introduction
- Hello Interview，LLD Delivery：https://www.hellointerview.com/learn/low-level-design/in-a-hurry/delivery
- ashishps1/awesome-low-level-design：https://github.com/ashishps1/awesome-low-level-design
- Flipkart 面試流程（第三方）：https://www.techprep.app/companies/flipkart
- Microsoft SDE II 面試心得（LeetCode）：https://leetcode.com/discuss/post/7620569/microsoft-sde-ii-interview-experience-re-hqua/

**AI 輔助程式回合**

- Canva，Yes, you can use AI in our interviews：https://canva.dev/blog/engineering/yes-you-can-use-ai-in-our-interviews
- Canva，AI Interview Success：https://www.canva.dev/blog/engineering/ai-interview-success/
- Hello Interview，Meta AI-enabled coding：https://www.hellointerview.com/blog/meta-ai-enabled-coding
- Hello Interview，AI coding 指南：https://www.hellointerview.com/learn/ai-coding/overview/introduction
- interviewing.io，AI 如何改變面試流程：https://interviewing.io/blog/how-is-ai-changing-interview-processes-not-much-and-a-whole-lot
- Entrepreneur，Google 試行 AI 面試：https://www.entrepreneur.com/business-news/google-is-testing-a-new-rule-transform-job-interviews
- Yahoo Finance（Moneywise），Google 允許面試使用 AI：https://finance.yahoo.com/sectors/technology/articles/asking-kid-math-test-without-151500986.html
- Aced（原 Exponent），Google AI coding interview：https://www.aced.io/blog/google-ai-coding-interview
- Let's Data Science，Google 測試 AI 熟練度（2026-10 擴大的單一二手來源）：https://letsdatascience.com/news/google-tests-ai-fluency-in-engineering-interviews-ddacf941
- TechCrunch，HackerRank Chakra：https://techcrunch.com/2026/10/05/hackerranks-ai-interviewer-offers-a-glimpse-into-what-job-interviews-could-become/
- HackerRank，2026-07 版本說明：https://hackerrank-knowledge-base.help.usepylon.com/articles/8142080826-july-2026-release-notes

**前一份報告**

- HackerRank 與 LeetCode 刷題研究：[../coding-interview-practice-research/README.zh-TW.md](../coding-interview-practice-research/README.zh-TW.md)
