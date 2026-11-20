[主報告](../README.zh-TW.md) | [01 原理](01-how-sql-works.zh-TW.md) | [02 實測紀錄](02-measurements.zh-TW.md)

# 來源與查證邊界

查核日期：2026-11-20。摘要為研究用途的自行改寫。「讀取狀態」標示本次是否實際讀到該來源的內文：多數早期論文的公開 PDF 是沒有文字層的掃描檔，本次無法讀取，相關敘述改以書目資料、官方文件或二手摘要佐證，並在表中註明。

## 第一手文獻

| 來源 | 讀取狀態 | 本研究採用的內容 |
| --- | --- | --- |
| E. F. Codd, "A Relational Model of Data for Large Shared Data Banks," *Communications of the ACM* 13(6), 1970-06, pp. 377–387. doi:[10.1145/362384.362685](https://doi.org/10.1145/362384.362685) | ACM 頁面 403；公開 PDF 為掃描檔。只確認書目與開頭的資料獨立性主張（搜尋摘要） | 資料獨立性的動機、以 n 元關係表示資料。論文定義的運算名稱（投影、連接等）依一般教科書敘述，本次未逐字核對 |
| E. F. Codd, "Relational Completeness of Data Base Sublanguages," in R. Rustin (ed.), *Data Base Systems*, Courant Computer Science Symposia 6, Prentice-Hall, 1972, pp. 65–98（1971 年 5 月研討會；另有 IBM 研究報告 RJ 987） | 僅搜尋摘要 | 關聯完備的定義；關聯代數與關聯演算等價。注意這是書中章節，不是期刊論文 |
| D. D. Chamberlin, R. F. Boyce, "SEQUEL: A Structured English Query Language," ACM SIGFIDET Workshop, Ann Arbor, 1974-05, pp. 249–264. doi:[10.1145/800296.811515](https://doi.org/10.1145/800296.811515) | 僅書目（dblp） | SQL 的前身與年份 |
| D. D. Chamberlin, "Early History of SQL," *IEEE Annals of the History of Computing* 34(4), 2012, pp. 78–82. doi:10.1109/MAHC.2012.61 | 未讀（付費牆） | 改名原因與早期商用產品的年份改依 [Wikipedia: SQL](https://en.wikipedia.org/wiki/SQL)（已讀部分）：SEQUEL 為 Hawker Siddeley Dynamics Engineering 的商標；Oracle V2 1979 年、SQL/DS 1981 年、DB2 1983 年 |
| P. G. Selinger, M. M. Astrahan, D. D. Chamberlin, R. A. Lorie, T. G. Price, "Access Path Selection in a Relational Database Management System," SIGMOD 1979. doi:[10.1145/582095.582099](https://doi.org/10.1145/582095.582099) | 公開 PDF 為掃描檔；改讀二手摘要 [The Morning Paper（2016-01-04）](https://blog.acolyer.org/2016/01/04/access-path-selection/) 全文 | 成本 = 頁讀取 + W × RSI 呼叫；無索引等值條件預設選擇率 1/10、範圍 1/3；由下往上的動態規劃；只考慮與已連接部分有連接條件的下一張表；有用的順序。這些細節均為二手摘要，起始頁 23 確認、結束頁未確認 |
| G. Graefe, "Volcano — An Extensible and Parallel Query Evaluation System," *IEEE TKDE* 6(1), 1994-02, pp. 120–135. doi:10.1109/69.273032 | 書目與摘要；PDF 為掃描檔 | 迭代器介面。open/next/close 的具體敘述本次沒有從原文讀到，執行器部分以 PostgreSQL 官方文件佐證 |
| R. Bayer, E. McCreight, "Organization and Maintenance of Large Ordered Indexes," *Acta Informatica* 1(3), 1972, pp. 173–189. doi:[10.1007/BF00288683](https://doi.org/10.1007/BF00288683) | 僅書目 | B-tree 的出處 |
| V. Leis, A. Gubichev, A. Mirchev, P. Boncz, A. Kemper, T. Neumann, "How Good Are Query Optimizers, Really?" *PVLDB* 9(3), 2015, pp. 204–215 | 僅摘要 | 基數估計誤差常常很大，影響遠大於成本模型；以 IMDB 資料建立的 Join Order Benchmark |
| J. M. Hellerstein, M. Stonebraker, J. Hamilton, "Architecture of a Database System," *Foundations and Trends in Databases* 1(2), 2007, pp. 141–259. doi:10.1561/1900000002 | 僅書目 | 列為整體架構的延伸閱讀 |

## 官方文件

**SQLite**（皆為本次讀取的現行版本）

| 頁面 | 讀取狀態 | 採用的內容 |
| --- | --- | --- |
| [Architecture of SQLite](https://www.sqlite.org/arch.html) | 全文 | 元件：tokenizer、parser（以 Lemon 產生）、程式碼產生器、bytecode 引擎、B-tree、pager、VFS |
| [The SQLite Bytecode Engine](https://www.sqlite.org/opcode.html) | 約前 85% | SQL 編譯成 bytecode 執行；EXPLAIN 列出程式；opcode 會隨版本改變。01 對 bytecode 的逐行解讀是作者依此頁自行對照 |
| [EXPLAIN QUERY PLAN](https://www.sqlite.org/eqp.html) | 全文 | SCAN、SEARCH、COVERING INDEX、TEMP B-TREE 的意義；輸出格式可能隨版本改變 |
| [The Next-Generation Query Planner](https://www.sqlite.org/queryplanner-ng.html) | 全文 | 3.8.0（2013-08-26）引入；N 個最近鄰居演算法；N 起初為 1、5、10，後來為 12，星狀結構 18 |
| [The SQLite Query Optimizer Overview](https://www.sqlite.org/optoverview.html) | 全文（分兩段讀取） | 連接一律以巢狀迴圈實作，會重排內連接以利用索引、不重排外連接；CROSS JOIN 固定順序；範圍條件的直方圖需 STAT3／STAT4 編譯選項；8.1 節以單元 `+` 停用索引；第 14 節自動索引只在單一語句內有效，14.1 節稱它幾乎等於 hash join |
| [Query Planning](https://www.sqlite.org/queryplanner.html) | 1.2–1.4 節全文 | 以 rowid 二分搜尋，成本與 log N 成正比；經索引取 K 列約 (K+1) × log N |
| [ANALYZE](https://www.sqlite.org/lang_analyze.html) | 全文 | 統計存在 sqlite_stat1；不會自動更新 |
| [Database File Format](https://www.sqlite.org/fileformat2.html) | 全文 | 一般表是以 rowid 為鍵的表 B-tree，每個索引是獨立的索引 B-tree（WITHOUT ROWID 表例外）；sqlite_stat1 的 stat 欄位意義 |

**PostgreSQL**（官方「current」文件此時為 PostgreSQL 19）

| 頁面 | 讀取狀態 | 採用的內容 |
| --- | --- | --- |
| [The Path of a Query](https://www.postgresql.org/docs/current/query-path.html) | 全文 | 連線、parser、rewrite system、planner/optimizer、executor 五階段 |
| [Planner/Optimizer](https://www.postgresql.org/docs/current/planner-optimizer.html) | 全文 | 近乎窮舉搜尋與 GEQO 的切換；沒有連接條件的表對只在別無選擇時考慮 |
| [Query Planning 設定](https://www.postgresql.org/docs/current/runtime-config-query.html) | 全文 | `geqo_threshold` 預設 12，計算的是 FROM 項目 |
| [Executor](https://www.postgresql.org/docs/current/executor.html) | 全文 | 執行器是需求拉動的管線，每次呼叫節點交出下一列 |
| [Comparison Functions and Operators](https://www.postgresql.org/docs/current/functions-comparison.html) | NULL 相關段落 | 任一輸入為 NULL 時比較結果為 NULL；判斷 NULL 要用 IS NULL |

**SQL 標準**

| 來源 | 讀取狀態 | 採用的內容 |
| --- | --- | --- |
| [ISO/IEC 9075-1:2023](https://www.iso.org/standard/93691.html)、[ISO/IEC 9075-16:2023（SQL/PGQ）](https://www.iso.org/standard/92320.html) | ISO 目錄頁經搜尋與擷取；第 2 部頁面 403 | SQL:2023 仍是最新完整版；2026 年 8 月發布部分章節的勘誤；SQL:2023 新增 SQL/PGQ 與 JSON 型別 |
| [Wikipedia: SQL](https://en.wikipedia.org/wiki/SQL) | 部分 | 各版標準的代表性新增；早期產品年份 |

**圖靈獎**：Codd 1981、Gray 1998、Stonebraker 2014。[ACM 圖靈獎網站](https://amturing.acm.org/)本次回傳 403，年份依 [ACM 2014 年得獎新聞稿](https://acm.org/media-center/2015/march/turing-award-2014)與搜尋摘要。

## 延伸閱讀（本次未查核）

- 向量化執行：P. Boncz, M. Zukowski, N. Nes, "MonetDB/X100: Hyper-Pipelining Query Execution," CIDR 2005。
- 把查詢編譯成機器碼：T. Neumann, "Efficiently Compiling Efficient Query Plans for Modern Hardware," PVLDB 4(9), 2011。
- G. Graefe, "Query Evaluation Techniques for Large Databases," *ACM Computing Surveys* 25(2), 1993：執行演算法的完整綜述。

## 本環境實測

| 項目 | 內容 |
| --- | --- |
| 環境 | Windows 11、Python 3.13.3、Python 內建 SQLite 3.49.1（`PRAGMA compile_options` 中沒有 STAT4） |
| 指令 | `python minisql.py demo`、`python minisql.py compare`、`python sqlite_plans.py`、`python index_tradeoff.py`、`python -m unittest test_minisql -v` |
| 輸出 | [`labs/minisql/example-output/`](../labs/minisql/example-output/)：`demo.txt`、`compare.txt`、`sqlite-3.49.1.txt`、`index-tradeoff.txt` |
| 結果 | 14 個測試全部通過；33 句差分查詢在 minisql 兩種計畫與 SQLite 上結果一致 |
