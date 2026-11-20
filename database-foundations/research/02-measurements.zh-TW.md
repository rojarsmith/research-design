[主報告](../README.zh-TW.md) | [01 原理](01-how-sql-works.zh-TW.md) | [03 來源](03-sources.zh-TW.md)

# 實測紀錄：同一句 SQL 的不同算法

> **量測日期：2026-11-20。** 環境：Windows 11、Python 3.13.3、Python 內建的 SQLite 3.49.1（編譯選項沒有 `SQLITE_ENABLE_STAT4`）。原始輸出在 [`labs/minisql/example-output/`](../labs/minisql/example-output/)。時間會因機器與執行次數而變，請把重點放在列數與比例；列數是確定的，每次執行都相同。

## 範例資料

`minisql.sample_tables()` 以固定的偽亂數產生，任何 Python 版本產生的資料都相同：

| 表 | 列數 | 欄位 | 刻意放進去的特徵 |
| --- | --- | --- | --- |
| cities | 6 | name, region | 北、中、南三區 |
| customers | 200 | id, name, city | 8 位客戶的 city 是 NULL |
| orders | 3,000 | id, customer_id, amount, status, day | status 有 4 種值，但 paid 約占 5/9；30 筆 amount 是 NULL；12 筆指向不存在的客戶 999 |

minisql 另有兩個索引：`customers(id)` 與 `orders(customer_id)`。

## 一、最佳化前後（`python minisql.py compare`）

「中間列數」是計畫中所有運算子產出列數的總和，粗略代表做了多少工作。

| 查詢 | 未最佳化：中間列數 | 未最佳化：時間 | 最佳化後：中間列數 | 最佳化後：時間 | 結果相同 |
| --- | --- | --- | --- | --- | --- |
| demo：兩表連接、篩選、分組 | 607,530 | 227.6 ms | 6,326 | 3.2 ms | 是 |
| 以索引找客戶 42 的訂單 | 3,042 | 1.1 ms | 42 | 0.7 ms | 是 |
| 三表連接，FROM 順序很差 | 3,622,055 | 1,688.8 ms | 4,114 | 2.1 ms | 是 |
| `LIMIT 5`（兩種計畫相同） | 86 | < 0.1 ms | 86 | < 0.1 ms | 是 |

### 三表查詢的兩個計畫

```sql
SELECT ci.region, COUNT(*) AS n FROM cities ci, orders o, customers c
WHERE o.customer_id = c.id AND c.city = ci.name AND ci.region = 'South'
GROUP BY ci.region
```

依定義翻譯時，FROM 的頭兩張表 cities 與 orders 之間沒有任何連接條件，於是先產生 6 × 3,000 = 18,000 列，再和 200 位客戶配成 3,600,000 列，最後才篩選：

```
Project ci.region, count(*) AS n  [est 3, actual 1]
└─ HashAggregate GROUP BY ci.region: count(*)  [est 3, actual 1]
   └─ Filter o.customer_id = c.id AND c.city = ci.name AND ci.region = 'South'  [est 995, actual 847]
      └─ NestedLoopJoin ×  [est 3,600,000, actual 3,600,000]
         ├─ NestedLoopJoin ×  [est 18,000, actual 18,000]
         │  ├─ SeqScan cities AS ci  [est 6, actual 6]
         │  └─ SeqScan orders AS o  [est 3,000, actual 3,000]
         └─ SeqScan customers AS c  [est 200, actual 200]
```

最佳化器不管 FROM 的順序：先取篩選後估計最小的 cities（2 列），接上和它有條件相連的 customers，最後才接 orders，全程沒有笛卡兒積：

```
Project ci.region, count(*) AS n  [est 3, actual 1]
└─ HashAggregate GROUP BY ci.region: count(*)  [est 3, actual 1]
   └─ HashJoin o.customer_id = c.id  (hash table built on the 2nd input)  [est 995, actual 847]
      ├─ SeqScan orders AS o  [est 3,000, actual 3,000]
      └─ HashJoin c.city = ci.name  (hash table built on the 2nd input)  [est 67, actual 57]
         ├─ SeqScan customers AS c  [est 200, actual 200]
         └─ Filter ci.region = 'South'  [est 2, actual 2]
            └─ SeqScan cities AS ci  [est 6, actual 6]
```

（推論）這也說明為什麼 SQL 使用者通常不必在意 FROM 裡表的順序：順序屬於「怎麼算」，由最佳化器決定。只有在最佳化器搜尋不完或估計錯誤時，順序才可能影響結果計畫。

### 估計與實際

demo 查詢最佳化後的計畫中，估計值（est）與實際值（actual）的差距：

| 運算子 | 估計 | 實際 | 原因 |
| --- | --- | --- | --- |
| `Filter o.status = 'paid'` | 750 | 1,621 | 假設 4 種 status 平均分布；實際 paid 約占 5/9 |
| `Filter c.city <> 'Tainan'` | 160 | 163 | 分布接近均勻，估得準 |
| `HashJoin` | 597 | 1,323 | 上一層的低估傳了上來 |
| `HashAggregate` | 6 | 5 | 6 個城市，但 Tainan 已被篩掉 |

在這個例子裡低估不影響計畫選擇；資料再大、表再多時，同樣的誤差就可能讓最佳化器選錯連接順序或演算法。

## 二、SQLite 怎麼處理同樣的查詢（`python sqlite_plans.py`）

| 情境 | `EXPLAIN QUERY PLAN` | 解讀 |
| --- | --- | --- |
| 沒有索引的點查詢 | `SCAN orders` | 全表掃描 |
| 建立索引後 | `SEARCH orders USING INDEX orders_customer_id (customer_id=?)` | 在索引 B-tree 裡搜尋，再回表 |
| 只要計數 | `SEARCH orders USING COVERING INDEX orders_customer_id (customer_id=?)` | 索引就有足夠資訊，不必回表 |
| `customer_id + 0 = 42` 計數 | `SCAN orders USING COVERING INDEX orders_customer_id` | 無法用索引搜尋，但掃描比表小的索引仍比掃表划算 |
| ORDER BY 無索引欄位 | `SCAN orders` ＋ `USE TEMP B-TREE FOR ORDER BY` | 用暫存 B-tree 排序（阻斷式） |
| demo 查詢 | `SCAN c` → `SEARCH o USING INDEX ...` ＋ GROUP BY、ORDER BY 各用一個暫存 B-tree | 索引巢狀迴圈連接；minisql 則選了 hash join |
| 三表查詢，未 ANALYZE | `SCAN ci` → `SEARCH c USING AUTOMATIC COVERING INDEX (city=?)` → `SEARCH o USING COVERING INDEX ...` | customers.city 沒有索引，SQLite 為這句語句臨時建了一個 |
| 三表查詢，ANALYZE 後 | `SCAN ci` → `SCAN c` → `SEARCH o USING COVERING INDEX ...` | 統計顯示 customers 只有 200 列，不值得臨時建索引 |

`ANALYZE` 之後 `sqlite_stat1` 的內容：

```
('cities', None, '6')
('customers', None, '200')
('orders', 'orders_customer_id', '3000 15')
```

前兩列只記錄表的列數（這兩張表沒有索引）。第三列表示索引有 3,000 筆，索引第一欄每個值平均對應 15 列（3,000 筆訂單 ÷ 200 位客戶）。

點查詢編譯成的 bytecode 與逐行解讀，見 [01 第五節](01-how-sql-works.zh-TW.md#另一種執行器bytecode-虛擬機器)。

`sqlite_plans.py` 最後一段把 demo、點查詢與三表查詢在 SQLite 和 minisql 上各執行一次，三句的結果都相同。

## 三、索引什麼時候有用（`python index_tradeoff.py`）

100 萬列、customer_id 均勻分布在 1–200、已執行 `ANALYZE`。在欄位前加單元 `+`（`+customer_id`）是 SQLite 文件記載的停用索引寫法，用來讓同一個條件分別以兩種方式執行：

| 條件 | 符合列數 | 用索引 | 全表掃描 | 較快的一方 |
| --- | --- | --- | --- | --- |
| `customer_id = 42` | 4,932 | 1.3 ms | 24.1 ms | 索引，約 19 倍 |
| `customer_id > 10` | 949,833 | 241.3 ms | 32.0 ms | 全表掃描，約 7.5 倍 |

範圍條件選出 95% 的列，經由索引等於對幾乎每一列都做一次「索引查找再回表」，比連續讀過整張表慢得多。這份 SQLite 沒有 STAT4 直方圖，不知道 `> 10` 會選出這麼多列，因此兩種條件都用了索引。這是實測時看到的行為，不代表啟用 STAT4 的 SQLite 也會這樣選。

## 四、差分測試（`python -m unittest test_minisql -v`）

33 句查詢各在 minisql 的未最佳化計畫、最佳化計畫與 SQLite 上執行，三者結果全部相同（沒有 ORDER BY 的查詢以多重集合比較）。涵蓋範圍：NULL 比較與三值邏輯、空輸入上的聚合、重複列、整數除法與取餘數的正負號、索引範圍查詢、非等值的跨表條件、指向不存在客戶的訂單、ORDER BY 別名與位置。另有 13 個單元測試檢查語意、錯誤訊息、計畫形狀（下推、hash join、索引、LIMIT 提早停止、排序阻斷）與詞法分析。本機結果：14 個測試全部通過（差分測試以 subTest 計為 1 個）。

已知差異：minisql 只實作 SQLite 行為的一個子集。它不做型別親和性轉換（`'12'` 與 `12` 不相等），也不支援外連接、子查詢、`LIKE`、`IN` 等語法；差分測試只使用兩邊語意一致的查詢。
