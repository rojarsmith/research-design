# minisql：看得見每一步的迷你 SQL 引擎

[回主報告](../../README.zh-TW.md)｜[01 原理](../../research/01-how-sql-works.zh-TW.md)｜[02 實測紀錄](../../research/02-measurements.zh-TW.md)

一個約 1,800 行、只用 Python 標準函式庫的關聯式查詢引擎，用來親眼看到「SQL 文字 → 記號 → 語法樹 → 關聯代數 → 最佳化後的計畫 → 一列一列的結果」。它不是可以拿來存資料的資料庫：沒有磁碟、交易與並行控制。已在 Python 3.13.3（Windows 11）執行並通過測試。

## 檔案

| 檔案 | 用途 |
| --- | --- |
| `minisql.py` | 引擎本體與命令列工具，依管線順序分成 11 段：詞法、AST、語法分析、運算式編譯、儲存、運算子、規劃器、資料庫、顯示、範例資料、CLI |
| `test_minisql.py` | 33 句查詢與 SQLite 的差分測試，加上語意、錯誤、計畫形狀的單元測試 |
| `sqlite_plans.py` | 同一份資料在 SQLite 上的 `EXPLAIN QUERY PLAN`、`ANALYZE` 統計與 bytecode |
| `index_tradeoff.py` | 100 萬列上「用索引」與「全表掃描」的實測比較 |
| `example-output/` | 上面各指令在本機的輸出 |

## 執行

```sh
cd database-foundations/labs/minisql
python minisql.py demo                     # 一句查詢走完每個階段
python minisql.py compare                  # 四句查詢，最佳化前後對照
python -m unittest test_minisql -v         # 與 SQLite 對照
python sqlite_plans.py                     # SQLite 怎麼做同樣的事
python index_tradeoff.py                   # 索引什麼時候有用（約需數秒建資料）
```

自己的查詢（使用範例資料庫：`cities`、`customers`、`orders`）：

```sh
python minisql.py query "SELECT status, COUNT(*) FROM orders GROUP BY status" --plan
python minisql.py query "SELECT ..." --no-opt --plan     # 看依定義翻譯的計畫
python minisql.py demo "SELECT ..."                      # 任何 SELECT 都可以走一遍 demo
python minisql.py run my.sql                             # 從空資料庫執行一個腳本
python minisql.py run my.sql --sample                    # 在範例資料庫上執行腳本
```

`--plan` 印出的每個運算子旁有 `[est 估計列數, actual 實際列數]`。查詢語句前面加上 `EXPLAIN` 或 `EXPLAIN ANALYZE` 也可以。

## 支援的 SQL

```sql
CREATE TABLE t (a INT, b TEXT, c REAL);
CREATE INDEX t_a ON t(a);
INSERT INTO t VALUES (1, 'x', 1.5), (2, NULL, 0);
SELECT [DISTINCT] 運算式 [AS 別名], ... | * | t.*
FROM t [別名] [, u ...] [[INNER] JOIN v ON 條件 ...]
[WHERE 條件] [GROUP BY 運算式, ...] [HAVING 條件]
[ORDER BY 運算式|別名|位置 [ASC|DESC], ...] [LIMIT n];
```

運算式：數字、字串、`NULL`、欄位、`+ - * / %`、比較 `= <> != < <= > >=`、`AND OR NOT`、`IS [NOT] NULL`，以及聚合函式 `COUNT(*)`、`COUNT`、`SUM`、`AVG`、`MIN`、`MAX`。

語意盡量跟 SQLite 一致，差分測試就是在檢查這件事：NULL 採三值邏輯；排序時 NULL 最小；整數除法向零截斷；除以零得 NULL；`SUM` 遇到空輸入得 NULL、`COUNT` 得 0。不一致的地方：沒有型別親和性轉換，未分組的欄位直接報錯（SQLite 允許，PostgreSQL 不允許）。

## 程式怎麼讀

依管線順序讀，每段都很短：

1. **`tokenize()`**：一個字元一個字元地切出記號。關鍵字轉大寫，識別字轉小寫（SQL 的識別字不分大小寫）。
2. **AST 資料類別**：`Literal`、`Column`、`Binary`……都是 `frozen=True`，所以兩個一樣的運算式會相等。規劃器靠這點把 SELECT 裡的 `c.city` 對應到 GROUP BY 裡的 `c.city`。
3. **`Parser`**：遞迴下降。`expr → and_expr → not_expr → comparison → additive → multiplicative → unary → primary`，越後面的規則優先順序越高。
4. **`compile_expr()`**：把運算式樹編成 Python 閉包 `row -> value`。NULL 就是 `None`，`sql_and`、`sql_or`、`sql_not` 實作三值邏輯。
5. **`Table`、`Index`**：表是 tuple 的 list；索引是排序過的 (鍵, 列位置) 清單，用 `bisect` 做等值與範圍查詢，回答的問題和 B-tree 一樣。`Table.stats()` 算出每欄的不同值個數與 NULL 比例，相當於 `ANALYZE`。
6. **運算子**：每個都有 `open()`、`next()`、`close()`。`SeqScan`、`IndexScan`、`Filter`（σ）、`NestedLoopJoin`（×）、`HashJoin`（⋈）、`HashAggregate`（γ）、`Sort`（τ）、`Project`（π）、`Distinct`（δ）、`Limit`。基底類別的 `next()` 會計數，所以 EXPLAIN ANALYZE 能印出實際列數。
7. **`Planner`**：`join_canonical()` 依定義翻譯（FROM 是 ×，ON 與 WHERE 是 σ）；`join_optimized()` 做下推、存取路徑選擇（`access_path()`）、貪婪連接順序與 hash join；`finish()` 處理聚合以上的部分；`selectivity()` 是基數估計。
8. **`Database.run()`**：執行各種語句。

## 練習

由易到難，每題都能用 `test_minisql.py` 的差分測試確認答案和 SQLite 一致：

1. 加上 `BETWEEN a AND b`：在 parser 裡展開成 `x >= a AND x <= b`，看看 `index_match()` 是否還能用上索引。
2. 加上 `IN (1, 2, 3)`。想一想它的選擇率該怎麼估。
3. 讓 `HAVING` 的估計不再是固定的 1/3。
4. 加上投影下推：掃描之後立刻丟掉上層用不到的欄位，觀察中間 tuple 變窄。
5. 加上索引巢狀迴圈連接：內側表在連接欄上有索引時，對每列外側到索引裡查，對照 SQLite 的 `SEARCH o USING INDEX`。
6. 把貪婪連接順序換成 Selinger 式的動態規劃：對每個表的子集合保留最便宜的計畫。用 5 張以上的表比較兩者的計畫。
7. 加上 `LEFT JOIN`。哪些下推在外連接下會變成錯的？寫一個會被錯誤下推弄錯答案的測試。
8. 為 `Table.stats()` 加上最常見值清單，修正 demo 中 `status = 'paid'` 估成 750、實際 1,621 的誤差。
