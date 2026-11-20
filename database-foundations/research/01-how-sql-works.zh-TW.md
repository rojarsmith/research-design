[主報告](../README.zh-TW.md) | [02 實測紀錄](02-measurements.zh-TW.md) | [03 來源](03-sources.zh-TW.md)

# SQL 是怎麼運作的：為什麼寫一段語法就能查到資料

> **研究日期：2026-11-20。** 史實與機制對照[來源清單](03-sources.zh-TW.md)查核，各來源的讀取狀態也列在那裡。文中的數字來自本主題的 [minisql 實驗](../labs/minisql/README.zh-TW.md)與本機的 SQLite 3.49.1（Python 3.13.3 內建），原始輸出放在 [`labs/minisql/example-output/`](../labs/minisql/example-output/)。標為「推論」的段落是依來源做的判斷，不是查到的事實。

## 結論

1. **資料先被規定成一種數學形狀：關係，也就是表。** Codd 在 1970 年提出關聯模型，目的是讓使用者不必知道資料在機器裡怎麼排列、怎麼索引（資料獨立性）。所有資料都是「欄位固定、列沒有順序」的表，查詢就變成對表做運算，而且運算的結果還是表。
2. **每一段 SQL 都對應到一個關聯代數式，所以它的意思是精確的。** `FROM` 是笛卡兒積（×），`WHERE` 是選擇（σ），`GROUP BY` 是分組聚合（γ），`SELECT` 清單是投影（π），`ORDER BY` 是排序（τ）。語法之所以能「指定」資料，是因為語法先被翻譯成一個代數式，答案就是這個代數式算出來的那張表。
3. **SQL 只說「要什麼」，不說「怎麼拿」。** 關聯代數有等價規則（篩選可以往下推、內連接可以交換順序），同一個答案有許多種算法。資料庫的最佳化器依照統計資料估計每種算法的成本，挑估計最便宜的一種。minisql 實驗中，同一句三表查詢依定義直接算要產生約 362 萬列中間結果，最佳化後只要 4,114 列，答案完全相同。
4. **資料庫自己掌握資料的實際存放方式。** 目錄（catalog）記錄有哪些表、欄位、型別與索引，統計資料記錄大概有多少列、多少個不同值；表與索引在磁碟上通常是 B-tree。因此資料庫能把「找 customer_id = 42 的訂單」落實成「在索引 B-tree 裡二分搜尋，再回表取列」，應用程式完全不必知道。
5. **最後由執行器一列一列地把結果拉上來。** 計畫是一棵運算子樹，上層向下層要下一列（PostgreSQL 的說法是「需求拉動的管線」），或像 SQLite 那樣先編成 bytecode，再由虛擬機器執行。

一句話總結：**SQL 是一種被編譯的語言。它編譯的對象不是 CPU 指令，而是對表的運算；最佳化器扮演編譯器的最佳化階段，執行器扮演 CPU。**這和本 repo [C 語言與編譯器](../../c-compiler-foundations/README.zh-TW.md)、[動態語言](../../dynamic-language-design-research/README.zh-TW.md)兩個主題是同一套想法，SQLite 甚至真的把 SQL 編成 bytecode。

## 目錄

1. [關聯模型：資料就是表，查詢就是對表的運算](#一關聯模型資料就是表查詢就是對表的運算)
2. [SQL：宣告式的關聯代數](#二sql宣告式的關聯代數)
3. [從文字到結果：查詢處理管線](#三從文字到結果查詢處理管線)
4. [最佳化器：同一個答案，很多種算法](#四最佳化器同一個答案很多種算法)
5. [執行器：一列一列往上拉](#五執行器一列一列往上拉)
6. [儲存層：表與索引放在哪裡](#六儲存層表與索引放在哪裡)
7. [一條查詢的完整旅程](#七一條查詢的完整旅程)
8. [常見誤解](#八常見誤解)
9. [這篇沒有談的部分](#九這篇沒有談的部分)

## 一、關聯模型：資料就是表，查詢就是對表的運算

### 1970 年以前的問題

在關聯模型之前，主流資料庫是階層式或網狀式的：資料以記錄和指標串起來，程式要「沿著指標走」才拿得到資料。資料的存放結構一改（多一層索引、換一種排列），走訪資料的程式就得跟著改。Codd 1970 年的論文開頭就指出這個問題：使用者應該不必知道資料在機器裡是怎麼組織的。他把這稱為資料獨立性。

### 關係：一張沒有順序的表

Codd 的方案是把所有資料都表示成**關係**（relation）。一個關係有固定的一組屬性（欄位），內容是一個由元組（列）組成的**集合**：

```
customers
  id | name        | city
  ---+-------------+---------
   1 | customer001 | Taipei
   2 | customer002 | Hsinchu
  ...
```

有兩個性質特別重要：

- **列沒有順序，也沒有指標。** 要找「客戶 2 的訂單」，不是從客戶記錄沿指標走到訂單，而是用「訂單的 customer_id 等於 2」這個條件去比對值。關係之間的連結只靠值，不靠位址。
- **運算的輸入與輸出都是關係（封閉性）。** 對表做篩選，得到一張表；兩張表連接，還是一張表。所以運算可以任意組合、層層巢狀，就像算術式可以一直套括號。

### 關聯代數：六個基本運算

對關係的運算組成**關聯代數**。教科書通常列出六個基本運算，其他運算都可以由它們組合出來：

| 運算 | 符號 | 意思 | 例子 |
| --- | --- | --- | --- |
| 選擇（selection） | σ | 留下滿足條件的列 | `σ[status = 'paid'](orders)` |
| 投影（projection） | π | 只留某些欄 | `π[id, amount](orders)` |
| 笛卡兒積（product） | × | 兩表的列兩兩配對 | `customers × orders` |
| 聯集（union） | ∪ | 兩表的列合起來 | |
| 差集（difference） | − | 在甲不在乙的列 | |
| 更名（rename） | ρ | 替表或欄改名 | `ρ[c](customers)` |

最常用的**連接**（join）不是基本運算，而是「先做笛卡兒積再選擇」的簡寫：

```
customers ⋈[o.customer_id = c.id] orders  ≡  σ[o.customer_id = c.id](customers × orders)
```

這條等式是後面所有最佳化的起點：**定義上**，連接是先產生全部配對再篩選；**實作上**，沒有資料庫會真的這樣算（第四節）。

Codd 在 1972 年又證明，關聯代數與以謂詞邏輯描述的「關聯演算」表達能力相同，並把「至少能表達關聯演算（也就是關聯代數）所能表達的一切查詢」定義為**關聯完備**（relational completeness）。這給了查詢語言一個明確的標準：一種語言只要關聯完備，任何能用代數寫出來的查詢，它都寫得出來。

## 二、SQL：宣告式的關聯代數

### 由來

IBM 聖荷西研究室在 System R 計畫中開發了 SEQUEL（Chamberlin 與 Boyce，1974），目標是讓不熟數學符號的人也能寫關聯查詢，所以語法刻意接近英文。SEQUEL 這個名字已經是英國 Hawker Siddeley 公司的商標，因此改名為 SQL。第一個商用 SQL 產品是 Relational Software（後來的 Oracle 公司）1979 年推出的 Oracle V2，IBM 則在 1981 年推出 SQL/DS、1983 年推出 DB2。標準化從 ANSI 1986 年的 SQL-86 開始，目前最新的完整版是 ISO/IEC 9075:2023（SQL:2023），2026 年 8 月另發布了勘誤。

| 版本 | 代表性新增 |
| --- | --- |
| SQL-86／SQL-87 | 第一版標準（ANSI X3.135-1986，ISO 9075:1987） |
| SQL-92 | 大幅擴充，長期是各家實作的共同基礎 |
| SQL:1999 | 遞迴查詢（`WITH RECURSIVE`）、觸發程序 |
| SQL:2003 | 視窗函式、SQL/XML |
| SQL:2016 | JSON 操作、列模式比對 |
| SQL:2023 | 屬性圖查詢 SQL/PGQ（第 16 部）、JSON 資料型別 |

### 每個子句都是一個代數運算

下面這句是本主題的 demo 查詢：

```sql
SELECT c.city, COUNT(*) AS n, SUM(o.amount) AS total
FROM customers c JOIN orders o ON o.customer_id = c.id
WHERE o.status = 'paid' AND c.city <> 'Tainan'
GROUP BY c.city
HAVING COUNT(*) > 200
ORDER BY total DESC
LIMIT 3
```

它的意思可以逐子句翻成代數。minisql 的 `demo` 指令會印出這個翻譯：

```
limit[3](π[c.city, count(*) AS n, sum(o.amount) AS total](
  τ[sum(o.amount) DESC](
    σ[count(*) > 200](
      γ[c.city; count(*), sum(o.amount)](
        σ[o.status = 'paid' AND c.city <> 'Tainan'](
          σ[o.customer_id = c.id](customers AS c × orders AS o)))))))
```

| SQL 子句 | 代數運算 | 定義上的計算順序 |
| --- | --- | --- |
| `FROM a, b`、`a JOIN b` | a × b | 1 |
| `ON p` | σ[p]（作用在 × 上） | 1 |
| `WHERE p` | σ[p] | 2 |
| `GROUP BY` 與聚合函式 | γ（分組並對每組計算 COUNT、SUM……） | 3 |
| `HAVING p` | σ[p]（作用在 γ 的結果上） | 4 |
| `SELECT` 清單 | π（可以含運算式的廣義投影） | 5 |
| `DISTINCT` | δ（去除重複） | 6 |
| `ORDER BY` | τ（排序） | 7 |
| `LIMIT` | 不在古典代數裡，取前 n 列 | 8 |

注意第三欄：**SQL 書寫的順序（SELECT 寫在最前面）不是它定義上的計算順序。**這也解釋了幾個常見規則：`WHERE` 不能用 `SELECT` 裡取的別名（別名到第 5 步才出現），`ORDER BY` 卻可以（第 7 步時別名已經存在）；聚合函式不能出現在 `WHERE`（第 2 步時還沒分組），要寫在 `HAVING`。

而這個「定義上的順序」也還不是實際的執行順序，實際順序由最佳化器決定（第四節）。三者的區別是理解 SQL 的關鍵：

- **書寫順序**：給人讀的。
- **定義順序**：決定答案是什麼。
- **執行順序**：決定怎麼算得快，只要答案和定義相同，怎麼算都可以。

### 宣告式：只描述答案

C 或 Python 是命令式語言，程式描述的是步驟：開檔、迴圈、比較、累加。SQL 是宣告式語言，只描述答案要滿足的條件。這帶來兩個結果：

1. **同一句 SQL 可以在資料量差一百萬倍的資料庫上執行，不用改寫。** 資料量、索引、硬體改變時，改變的是資料庫選的算法，不是查詢本身。
2. **資料庫要承擔「選算法」這件事。** 這就是最佳化器存在的理由，也是 SQL 引擎裡最複雜的部分。

### SQL 與純關聯模型的三個差別

SQL 為了實用，在幾個地方偏離了 Codd 的模型，這些差別每天都會遇到：

- **重複列會保留。** 關係是集合，SQL 的表與查詢結果卻是多重集合：兩筆一樣的列都會留著，除非寫 `DISTINCT`。原因很實際：去除重複要排序或雜湊，成本不低。minisql 的測試 `test_duplicates_kept_unless_distinct` 示範了這點。
- **NULL 與三值邏輯。** NULL 表示「未知」或「不適用」。任何值和 NULL 比較，結果是 UNKNOWN，連 `NULL = NULL` 也是；`WHERE` 與 `HAVING` 只留下條件為 TRUE 的列，FALSE 和 UNKNOWN 都被丟掉。所以判斷 NULL 要用 `IS NULL`，而 `NOT (city = 'Taipei')` 不會選出 city 是 NULL 的列（`NOT UNKNOWN` 還是 UNKNOWN）。demo 查詢的 `c.city <> 'Tainan'` 也因此順便把 8 位城市是 NULL 的客戶濾掉了：200 位客戶只剩 163 位。
- **順序只在 `ORDER BY` 時才有保證。** 關係沒有順序，沒寫 `ORDER BY` 時，結果的順序取決於當時選中的計畫。計畫一換，順序就可能改變。

## 三、從文字到結果：查詢處理管線

不論是 PostgreSQL、SQLite 還是 minisql，一句 SQL 都走過大致相同的幾站：

```
SQL 文字
  │ ① 詞法分析：字元 → 記號（SELECT、c、.、city、,……）
  ▼
記號序列
  │ ② 語法分析：依文法組成語法樹（AST）
  ▼
語法樹
  │ ③ 語意分析：查目錄，確認表與欄存在、欄屬於哪張表、型別是否合理
  │    （PostgreSQL 在這之後還有改寫系統，例如把檢視展開）
  ▼
邏輯計畫 ＝ 關聯代數樹 ──── 這一步決定「答案是什麼」
  │ ④ 最佳化：套用等價規則、估計成本、選出實體計畫
  ▼
實體計畫 ──────────────── 這一步決定「怎麼算」：用哪個索引、哪種連接演算法、什麼順序
  │ ⑤ 執行：運算子迭代器，或 bytecode 虛擬機器
  ▼
儲存引擎：頁、B-tree、緩衝區
  │
  ▼
結果列
```

各家的對應：

| 階段 | PostgreSQL（官方內部架構章節） | SQLite（官方架構頁） | minisql |
| --- | --- | --- | --- |
| ①② | parser（含詞法與語法） | tokenizer、parser（由 Lemon 產生） | `tokenize()`、`Parser` |
| ③ | parser 的轉換階段、rewrite system | 程式碼產生器的一部分 | `Planner.owner()` 等 |
| ④ | planner/optimizer | 程式碼產生器內的查詢規劃器（NGQP） | `Planner.join_optimized()` |
| ⑤ | executor | bytecode 引擎（VDBE） | `Operator` 子類別 |
| 儲存 | 存取方法、緩衝區管理 | B-tree、pager（頁快取）、VFS | Python list 與排序索引 |

第 ①、② 站和任何程式語言的編譯器前端一樣：詞法分析把 `'paid'` 認成一個字串記號，語法分析依遞迴下降把 `a AND b OR c` 組成 `OR(AND(a, b), c)`，運算子優先順序由「哪個規則呼叫哪個規則」決定。minisql 的 demo 會印出完整的記號序列（65 個）與語法樹。

第 ③ 站是 SQL 特有的重點：**語法樹本身不知道 `c.city` 是什麼。**要查目錄才知道 `c` 是 `customers` 的別名、`customers` 有 `city` 欄、型別是 TEXT。沒寫表名的欄位（例如 `city`）若在兩張表都有，就是「ambiguous column」錯誤；這類錯誤都在這一站報出來。

## 四、最佳化器：同一個答案，很多種算法

### 等價規則給了自由

最佳化器能動手，靠的是關聯代數的等價規則。最常用的幾條：

| 規則 | 等式 | 效果 |
| --- | --- | --- |
| 選擇拆分 | `σ[p AND q](R) ≡ σ[p](σ[q](R))` | 把 WHERE 拆成一個個條件分別處理 |
| 選擇下推 | `σ[p](R × S) ≡ σ[p](R) × S`（p 只用到 R） | 先篩掉 R 的列，再去配對 |
| 積轉連接 | `σ[R.a = S.b](R × S) ≡ R ⋈[a = b] S` | 等值條件可以用雜湊或索引來配對，不必兩兩比較 |
| 交換律、結合律 | `R ⋈ S ≡ S ⋈ R`；`(R ⋈ S) ⋈ T ≡ R ⋈ (S ⋈ T)` | 內連接的順序可以任意調換 |
| 投影下推 | 只保留上層需要的欄 | 中間結果變窄 |

但這些規則有前提。例如外連接（`LEFT JOIN`）不能任意交換順序，篩選條件也不能隨便推過外連接；SQLite 文件明白寫著它不會重排外連接的順序。這也是 minisql 只支援內連接的原因。

### demo 查詢被改寫成什麼

minisql 對 demo 查詢做了這些事（`python minisql.py demo` 的第 5 段會印出來）：

```
未最佳化（依定義翻譯）                           最佳化後
Filter o.status = 'paid' AND c.city <> ...       HashJoin o.customer_id = c.id
└─ Filter o.customer_id = c.id                   ├─ Filter o.status = 'paid'
   └─ NestedLoopJoin ×   [600,000 列]            │  └─ SeqScan orders AS o
      ├─ SeqScan customers AS c                  └─ Filter c.city <> 'Tainan'
      └─ SeqScan orders AS o                        └─ SeqScan customers AS c
```

- **下推**：`o.status = 'paid'` 只用到 orders，移到 orders 的掃描正上方；`c.city <> 'Tainan'` 移到 customers 上方。
- **積轉連接**：`o.customer_id = c.id` 是等值條件，笛卡兒積加篩選改成 hash join。
- **選擇建表的一側**：hash join 要把一側整個放進雜湊表，估計 customers 篩完約 160 列、orders 篩完約 750 列，所以雜湊表建在 customers 這一側。

結果：所有運算子合計產生的列數從 607,530 降到 6,326，答案相同。三表查詢的差距更大，見 [02 實測紀錄](02-measurements.zh-TW.md)。

### 連接順序：搜尋空間會爆炸

n 張表做內連接，只算「左深樹」（每次把一張新表接到目前結果上）就有 n! 種順序：10 張表是 3,628,800 種，允許任意形狀的樹還更多。最佳化器不可能逐一試，於是各家用不同的搜尋策略：

- **System R（Selinger 等人，1979）**：奠定了今天主流做法的論文。用動態規劃由下往上搜尋：先找每張表的最佳存取方式，再找每兩張表的最佳連接，再找三張……每個子集合只保留最便宜的計畫。它還有兩個影響深遠的想法：一是盡量不考慮沒有連接條件的組合（也就是避免笛卡兒積）；二是「有用的順序」（interesting orders），即某個計畫雖然較貴，但它的輸出剛好依照 `ORDER BY`、`GROUP BY` 或後續連接需要的欄位排好，也要保留下來，因為可以省掉之後的排序。
- **PostgreSQL**：FROM 項目少於 `geqo_threshold`（預設 12）時做近乎窮舉的搜尋；超過時改用基因演算法（GEQO）。官方文件也說明，沒有連接條件的表對只有在別無選擇時才會被考慮。
- **SQLite**：3.8.0 版（2013 年）起使用「下一代查詢規劃器」（NGQP），以「N 個最近鄰居」演算法搜尋：每一步只保留 N 條最好的部分路徑。N 起初依表數設為 1、5 或 10，後來提高到 12，星狀結構則是 18。
- **minisql**：最簡單的貪婪法。先取估計列數最小的表，之後每一步在「和已連接部分有等值條件」的表中挑估計最小的；只有找不到時才做笛卡兒積。

### 成本模型與基數估計

要比較兩個計畫哪個便宜，最佳化器需要估計**每個運算子會產生幾列**（基數，cardinality）。估計來自統計資料與幾個簡化假設：

- 欄位 = 常數：選擇率約為 1 ÷ 不同值的個數（假設每個值出現次數相同）。
- 範圍條件：沒有直方圖時只能猜。根據二手摘要，System R 對範圍條件猜 1/3，對沒有索引統計的等值條件猜 1/10。
- 兩個條件 AND：選擇率相乘（假設兩欄彼此獨立）。
- 連接 R.a = S.b：|R| × |S| ÷ 兩欄不同值個數中較大者。

這些假設在真實資料上常常不成立。minisql 的 demo 就有一個現成的例子：status 有 4 種值，所以估計 `status = 'paid'` 會留下 1/4，也就是 750 列；但範例資料刻意讓 paid 占了大約 5/9，實際是 1,621 列，差了兩倍多。錯誤還會往上傳：連接的估計是 597 列，實際 1,323 列。Leis 等人 2015 年用真實的 IMDB 資料測試多個系統，結論是基數估計的誤差常常很大，而且它對計畫品質的影響遠大於成本模型本身的粗糙。真實資料庫因此會保存更細的統計，例如 SQLite 的 `sqlite_stat4` 直方圖（需要以 `SQLITE_ENABLE_STAT4` 編譯）。

**索引不一定比較快。**經由索引取一列，要先在索引裡找到位置，再回到表裡取那一列，這是隨機讀取；全表掃描則是連續讀取。條件只選出少數幾列時用索引划算，選出大半時反而更慢。本機用 SQLite 對 100 萬列實測：

| 條件 | 符合列數 | 用索引 | 全表掃描 |
| --- | --- | --- | --- |
| `customer_id = 42` | 4,932（約 0.5%） | 1.3 ms | 24.1 ms |
| `customer_id > 10` | 949,833（約 95%） | 241.3 ms | 32.0 ms |

這份 Python 內建的 SQLite 沒有啟用 STAT4，無法估計範圍條件會選出多少列，所以兩種條件都選了索引，第二種因此慢了約 7.5 倍。minisql 用一條教學用的成本規則（經索引取一列算 4 單位，全表掃描每列算 1 單位）避開了這個情況，但同樣只是估計。結論是：**要知道資料庫實際怎麼做，看 `EXPLAIN`，不要猜。**

## 五、執行器：一列一列往上拉

### 迭代器模型

最佳化器的產物是一棵運算子樹。最經典的執行方式是 Graefe 在 Volcano 系統（1994）中整理的迭代器介面：每個運算子都提供 `open()`、`next()`、`close()` 三個操作。父運算子呼叫子運算子的 `next()` 取得一列，處理後再交給自己的父運算子。PostgreSQL 的執行器說明用的也是這個模型：它基本上是需求拉動的管線，每次呼叫一個計畫節點，節點就要交出下一列，或回報已經沒有資料。

```python
class Filter(Operator):            # minisql 的 σ
    def _next(self):
        while (row := self.child.next()) is not None:   # 向下要一列
            if truth(self.fn(row)) is True:              # 條件為 TRUE 才往上交
                return row
        return None                                      # 下面沒有了，我也沒有了
```

這個介面的好處是每個運算子都可以獨立實作、任意組合，和關聯代數的封閉性互相對應。

### 管線與阻斷

運算子分成兩類，這個差別決定了查詢的記憶體用量與「第一列多快出來」：

- **可管線化**：掃描、篩選、投影、LIMIT、hash join 的探測側。拿到一列就能交出一列，不必等。
- **阻斷式**：排序、分組聚合、hash join 的建表側。必須讀完全部輸入，才能交出第一列。

minisql 可以直接看到差別：

```
SELECT id, amount FROM orders WHERE status = 'refunded' LIMIT 5
  Limit 5 → Project → Filter → SeqScan orders   [SeqScan 實際只讀了 71 列就停了]

SELECT * FROM orders ORDER BY amount LIMIT 5
  Limit 5 → Project → Sort → SeqScan orders     [Sort 必須先讀完 3,000 列]
```

第一句裡，LIMIT 拿到 5 列就不再呼叫 `next()`，下面的掃描自然停止。這就是為什麼「加上 LIMIT」通常很快，但「ORDER BY 沒有索引可用再加 LIMIT」仍然要讀完整張表。

### 連接演算法

| 演算法 | 做法 | 成本（R 為外側，S 為內側） | 適用 |
| --- | --- | --- | --- |
| 巢狀迴圈 | 每列 R 和每列 S 比較 | 約 \|R\| × \|S\| | 小表、非等值條件 |
| 索引巢狀迴圈 | 每列 R 到 S 的索引裡查 | 約 \|R\| × log \|S\| | 內側有索引，外側不大 |
| Hash join | 較小一側建雜湊表，另一側逐列探測 | 約 \|R\| + \|S\| | 等值條件、沒有可用索引 |
| 排序合併 | 兩側依連接欄排序後合併 | 排序成本加 \|R\| + \|S\| | 輸入本來就排好序 |

不同引擎的選擇可以很不一樣。SQLite 文件說它把所有連接都實作成巢狀迴圈，搭配索引查找；需要時會在查詢期間臨時建一個只在這句語句內有效的「自動索引」，文件稱它幾乎等於 hash join，只是用 B-tree 取代雜湊表。本機的 `EXPLAIN QUERY PLAN` 對 demo 查詢顯示的就是索引巢狀迴圈：

```
├─ SCAN c
├─ SEARCH o USING INDEX orders_customer_id (customer_id=?)
├─ USE TEMP B-TREE FOR GROUP BY
└─ USE TEMP B-TREE FOR ORDER BY
```

minisql 對同一句選了 hash join，兩者答案相同。這正好說明宣告式的意義：同一句 SQL，兩個引擎各自決定怎麼算。

### 另一種執行器：bytecode 虛擬機器

SQLite 不直接執行運算子樹，而是把每句 SQL 編譯成一段 bytecode 程式，交給它的虛擬機器（VDBE）執行；`EXPLAIN` 會把這段程式列出來。本機對 `SELECT id, amount FROM orders WHERE customer_id = 42` 得到的程式（opcode 的意義依 SQLite 的 opcode 文件解讀）：

```
addr  opcode          p1  p2  p3  p4
   0  Init             0  12   0           跳到 12 做初始化
   1  OpenRead         0   6   0  3        開啟 orders 表的 B-tree（游標 0）
   2  OpenRead         1  25   0  k(2,,)   開啟索引的 B-tree（游標 1）
   3  Integer         42   1   0           把 42 放進暫存器 1
   4  SeekGE           1  11   1  1        在索引中找到第一個 ≥ 42 的位置；找不到就跳到 11
   5  IdxGT            1  11   1  1        若索引鍵已經 > 42，跳到 11 結束
   6  DeferredSeek     1   0   0           記下這筆索引對應的表列（需要時才真正去讀）
   7  Column           0   0   2           取 id 放進暫存器 2
   8  Column           0   2   3           取 amount 放進暫存器 3
   9  ResultRow        2   2   0           把暫存器 2–3 當成一列結果交出去
  10  Next             1   5   1           索引游標往下一筆，回到 5
  11  Halt             0   0   0
  12  Transaction      0   0   5  0        開始讀取交易
  13  Goto             0   1   0           回到 1
```

這段程式就是「索引查找」的全部：在索引 B-tree 裡二分搜尋到 42，沿著索引往後走，每一筆回表取兩個欄位，遇到大於 42 就停。拿它和 [dynamic-language-design-research](../../dynamic-language-design-research/README.zh-TW.md) 的 bytecode VM 對照，會發現結構幾乎一樣：暫存器、條件跳躍、迴圈。差別只在 SQLite 的指令操作的是 B-tree 游標。SQLite 也提醒，opcode 會隨版本改變，不是穩定介面。

（推論）另外兩個方向是向量化執行（一次處理一批列，攤平函式呼叫成本）與把查詢編譯成機器碼。兩者都是為了降低「每列呼叫一次 `next()`」的額外負擔，本文沒有實測，列在[來源](03-sources.zh-TW.md)的延伸閱讀中。

## 六、儲存層：表與索引放在哪裡

最後一站是磁碟。資料庫把檔案切成固定大小的**頁**，以頁為單位讀寫，並在記憶體中保留常用的頁（緩衝區或頁快取）。表和索引多半以 **B-tree**（Bayer 與 McCreight，1972）或其變形 B+tree 存放：樹很寬、很矮，找任何一個鍵只需要讀很少幾頁。

以 SQLite 為例（官方檔案格式文件）：

- 一般的表是一棵以 64 位元 rowid 為鍵的「表 B-tree」，整列資料存在葉節點。
- 每個索引是另一棵獨立的「索引 B-tree」，鍵是被索引的欄位值加上 rowid。
- 用 rowid 找一列是一次二分搜尋，成本與 log N 成正比；經由索引找 K 列，要先在索引裡找、再回表找，成本約與 (K+1) × log N 成正比。
- 如果查詢需要的欄位全都在索引裡，就不必回表，`EXPLAIN QUERY PLAN` 會顯示 `USING COVERING INDEX`。本機的 `SELECT COUNT(*) FROM orders WHERE customer_id = 42` 就是這樣。

統計資料也存在資料庫裡。執行 `ANALYZE` 後，SQLite 的 `sqlite_stat1` 對 `orders_customer_id` 索引存了 `3000 15`：第一個數字是索引的列數，第二個是「索引第一欄同一個值平均有幾列」，也就是 3000 ÷ 200 位客戶。SQLite 不會自動更新這些統計，資料分布改變後要重新執行 `ANALYZE`。

minisql 為了把重點留在查詢處理，沒有實作頁與磁碟：表是 Python 的 list，索引是排序後的 (鍵, 列位置) 清單，用二分搜尋查找。它能回答和 B-tree 一樣的等值與範圍查詢，但沒有頁、快取與寫入時的平衡維護。

## 七、一條查詢的完整旅程

把前面各站串起來，以 demo 查詢為例：

1. **詞法分析**把 220 個字元切成 65 個記號。
2. **語法分析**依文法建出語法樹：SELECT 清單三項、FROM 兩張表加一個 ON、WHERE 是一個 AND 節點……
3. **語意分析**查目錄：`c` 是 customers、`o` 是 orders，`o.customer_id` 存在，`COUNT(*)` 是聚合函式，所以這是分組查詢，`c.city` 必須出現在 GROUP BY（它有）。
4. **翻成關聯代數**：π(τ(σ(γ(σ(σ(customers × orders))))))，這定義了答案。
5. **最佳化**：拆開 WHERE、把兩個單表條件推到掃描上方、把等值條件變成 hash join、估計 customers 較小所以在它上面建雜湊表。
6. **執行**：Limit 向 Project 要列，Project 向 Sort 要，Sort 一口氣把 Filter（HAVING）的輸出全要過來；HashAggregate 讀完 HashJoin 的全部輸出；HashJoin 先讀完 customers 這側建雜湊表，再一列列讀 orders 去探測。
7. **儲存層**：兩次全表掃描，各讀一遍資料。
8. 結果：3 列。Hsinchu 331 筆，Taichung 292 筆，New Taipei 242 筆。

`python minisql.py demo` 會把第 1、2、4、5、6 步的實際內容依序印出來，並在每個運算子旁邊標出估計列數與實際列數。

## 八、常見誤解

| 誤解 | 實際情況 | 在哪裡驗證 |
| --- | --- | --- |
| SQL 依書寫順序執行 | 書寫、定義、執行是三種不同的順序；執行順序由最佳化器決定 | minisql `demo` 第 4、5 段 |
| 有索引就一定會用、用了就一定快 | 選擇率高時全表掃描反而快；有沒有用要看 EXPLAIN | `index_tradeoff.py`、`compare` 第 2 例 |
| 在欄位上做運算沒有影響 | `WHERE customer_id + 0 = 42` 讓欄位變成運算式，索引無法用來搜尋（SQLite 文件把單元 `+` 當成刻意停用索引的寫法） | `minisql.py query "... customer_id + 0 = 42" --plan` |
| `NULL = NULL` 為真 | 結果是 UNKNOWN，`WHERE` 會把這列丟掉；要用 `IS NULL` | `test_null_is_not_equal_to_null` |
| 不寫 ORDER BY 也有固定順序 | 沒有保證，計畫一換順序就可能變 | 比較 minisql 與 SQLite 的輸出順序 |
| 換個寫法就會變快 | 等價的寫法理論上應得到同一個計畫；真正影響速度的是統計資料、索引，以及最佳化器能否看穿寫法 | `compare`：FROM 順序很差的三表查詢仍得到好計畫 |
| 估計值大致準確 | 均勻與獨立的假設常常不成立，誤差會沿著計畫往上放大 | demo 第 6 段的 est 與 actual |

## 九、這篇沒有談的部分

本文只回答「查詢語法如何變成資料」。資料庫另一半的核心問題，包括交易（ACID）、並行控制（鎖與多版本）、當機後的恢復（預寫日誌），都沒有涵蓋；這一塊的奠基者之一 Jim Gray 在 1998 年獲得圖靈獎，Codd 是 1981 年，Stonebraker 是 2014 年。子查詢的去巢狀化、外連接、視窗函式、遞迴查詢、平行與分散式執行，也都留給後續研究。
