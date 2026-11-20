[English](README.md) | **繁體中文**

# 關聯式資料庫與 SQL：為什麼寫一段語法就能查到資料

研究日期：2026-11-20。本主題回答一個問題：關聯式資料庫的 SQL 是怎麼運作的，為什麼用一段語法就能查到資料？除了說明原理，也附一個只用 Python 標準函式庫寫成的迷你 SQL 引擎，把每個階段印出來，並用本機的 SQLite 3.49.1 對照真實資料庫的做法。

## 核心結論

- **資料先被規定成一種數學形狀：關係（表）。** Codd 1970 年的關聯模型讓資料只以「欄位固定、列沒有順序、彼此只靠值關聯」的表存在，查詢因此可以定義成對表的運算，而且運算的結果還是表，可以一直組合下去。目的是資料獨立性：使用者不必知道資料在機器裡怎麼放。
- **SQL 的每個子句都對應一個關聯代數運算，所以語法有精確的意思。** `FROM` 是笛卡兒積，`WHERE` 是選擇，`GROUP BY` 是分組聚合，`SELECT` 是投影，`ORDER BY` 是排序。一段 SQL 先被翻成代數式，答案就是那個代數式算出來的表。書寫順序（SELECT 在前）、定義順序（FROM 先算）和實際執行順序是三件不同的事。
- **SQL 只說「要什麼」，「怎麼算」交給最佳化器。** 代數的等價規則（篩選往下推、內連接可換順序、積加篩選可改成連接）讓同一個答案有很多種算法，最佳化器用統計資料估計成本，挑最便宜的。實測中，一句三表查詢依定義計算要產生約 362 萬列中間結果、約 1.7 秒；最佳化後只有 4,114 列、約 2 毫秒，答案相同。
- **最佳化器的弱點在估計。** 它假設資料分布均勻、欄位彼此獨立，真實資料常常不是這樣。demo 中 `status = 'paid'` 估計 750 列、實際 1,621 列。索引也不是一定比較快：SQLite 在 100 萬列上選出 0.5% 時，用索引快約 19 倍；選出 95% 時，用索引反而慢約 7.5 倍。
- **執行器一列一列把結果往上拉，儲存層把邏輯運算落實成讀頁。** 計畫是一棵運算子樹，每個運算子提供 open／next／close，上層向下層要下一列（PostgreSQL 稱為需求拉動的管線）；SQLite 則把 SQL 編成 bytecode 交給虛擬機器。表與索引通常是 B-tree，依鍵查找的成本與 log N 成正比。

一句話總結：SQL 是被編譯的語言，編譯的目標不是 CPU 指令，而是對表的運算。這和本 repo 的 [C 語言與編譯器](../c-compiler-foundations/README.zh-TW.md)、[動態語言](../dynamic-language-design-research/README.zh-TW.md)兩個主題是同一套想法。

## 文件地圖

| 文件 | 內容 |
| --- | --- |
| [01 SQL 是怎麼運作的](research/01-how-sql-works.zh-TW.md) | 關聯模型與關聯代數、SQL 的由來與標準、子句與代數的對應和三種順序、NULL 與重複列、查詢處理管線、最佳化器（等價規則、連接順序、基數估計、索引的取捨）、迭代器與管線、連接演算法、SQLite bytecode 逐行解讀、B-tree 與統計、常見誤解 |
| [02 實測紀錄](research/02-measurements.zh-TW.md) | 最佳化前後的中間列數與時間、估計與實際的差距、SQLite 的 EXPLAIN QUERY PLAN 對照、ANALYZE 統計、100 萬列上的索引取捨、差分測試範圍 |
| [03 來源與查證邊界](research/03-sources.zh-TW.md) | Codd、System R、Volcano、B-tree 等第一手文獻，SQLite 與 PostgreSQL 官方文件，SQL 標準；逐項標示讀取狀態 |
| [minisql 實驗說明](labs/minisql/README.zh-TW.md) | 迷你 SQL 引擎的使用方式、支援的語法、程式導讀與八道練習 |
| [`labs/minisql/example-output/`](labs/minisql/example-output/) | 各指令在本機的原始輸出 |

## 五分鐘開始

```sh
cd database-foundations/labs/minisql
python minisql.py demo
```

只需要 Python 3，不必安裝任何套件（已在 Python 3.13.3 測試）。`demo` 會把一句 SQL 依序印成：記號序列 → 語法樹 → 依定義翻譯的關聯代數（含 60 萬列的笛卡兒積）→ 最佳化後的計畫與改寫理由 → 執行後每個運算子的估計列數與實際列數 → 結果。接著：

```sh
python minisql.py compare               # 四句查詢的最佳化前後對照
python -m unittest test_minisql -v      # 33 句查詢與 SQLite 的差分測試
python sqlite_plans.py                  # SQLite 的 EXPLAIN QUERY PLAN、統計與 bytecode
python index_tradeoff.py                # 100 萬列上用索引與全表掃描的時間
```

## 建議閱讀順序

1. 先跑 `python minisql.py demo`，把輸出從頭看到尾。
2. 讀 [01](research/01-how-sql-works.zh-TW.md) 的結論與第一、二節，理解為什麼 SQL 的每一句都是一個代數式。
3. 讀第三到五節，對照 demo 輸出的第 4、5、6 段。
4. 跑 `compare` 與 `sqlite_plans.py`，再讀 [02](research/02-measurements.zh-TW.md)，看同一句 SQL 在兩個引擎上各被怎麼算。
5. 做 [minisql 實驗說明](labs/minisql/README.zh-TW.md) 裡的練習，例如把貪婪連接順序換成動態規劃。

## 範圍與限制

- 本主題只談查詢：SQL 文字如何變成資料。交易、並行控制、當機恢復、子查詢去巢狀化、外連接、視窗函式、分散式執行都不在範圍內。
- minisql 是教學用引擎：資料在記憶體裡，索引是排序清單而不是有頁的 B-tree，最佳化器是貪婪法，成本模型只有一條教學用規則。它和 SQLite 的語意只在差分測試涵蓋的範圍內一致。
- 早期論文（Codd 1970、Selinger 1979、Graefe 1994）的公開 PDF 多為掃描檔，本次無法讀取全文。相關敘述以書目、官方文件與二手摘要佐證，[03](research/03-sources.zh-TW.md) 逐項標示了讀取狀態。
- SQLite 的 `EXPLAIN` 與 `EXPLAIN QUERY PLAN` 輸出格式官方聲明會隨版本改變；本主題的輸出來自 3.49.1，且這份建置沒有啟用 STAT4 直方圖。時間量測取自單一台 Windows 11 機器，請看比例而不是絕對值。
