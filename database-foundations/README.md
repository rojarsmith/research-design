**English** | [繁體中文](README.zh-TW.md)

# Relational Databases and SQL: Why a Piece of Syntax Can Fetch Data

Research date: 2026-11-20. The complete research is in Traditional Chinese; this page is an overview. The lab code is in English, and its README and console output are in Traditional Chinese.

## Question

How does SQL in a relational database work, and why can a piece of syntax retrieve data?

## Findings in brief

- **Data is first forced into one mathematical shape: the relation (a table).** Codd's 1970 relational model keeps data only in tables with fixed columns, unordered rows, and links between tables made by values rather than pointers. Queries then become operations on tables whose results are again tables, so they compose freely. The goal was data independence: users should not need to know how data is laid out in the machine.
- **Every SQL clause maps to a relational-algebra operator, so the syntax has a precise meaning.** `FROM` is a Cartesian product, `WHERE` a selection, `GROUP BY` grouping and aggregation, the `SELECT` list a projection, `ORDER BY` a sort. A query is first translated into an algebra expression, and the answer is whatever that expression evaluates to. Written order (SELECT first), definitional order (FROM first) and actual execution order are three different things.
- **SQL says what, the optimizer decides how.** Equivalence rules (push selections down, reorder inner joins, turn product plus selection into a join) give many ways to compute the same answer, and the optimizer picks the cheapest by estimated cost. In the lab, a three-table query evaluated by definition produces about 3.6 million intermediate rows in about 1.7 s; the optimized plan produces 4,114 rows in about 2 ms with the same answer.
- **Estimation is the optimizer's weak spot.** It assumes uniform, independent columns, which real data often violates: the demo estimates 750 rows for `status = 'paid'` and gets 1,621. An index is not always faster either: on 1 million rows in SQLite, an index is about 19× faster when 0.5% of rows match and about 7.5× slower when 95% match.
- **The executor pulls rows up one at a time; storage turns logical operations into page reads.** A plan is a tree of operators with open/next/close, each asking its child for the next row (PostgreSQL calls this a demand-pull pipeline). SQLite instead compiles SQL to bytecode for its virtual machine. Tables and indexes are usually B-trees, so a key lookup costs on the order of log N.

## Lab

`labs/minisql/` is a relational query engine of about 1,800 lines using only the Python standard library. `python minisql.py demo` prints one query as tokens, an AST, the canonical relational-algebra plan (with a 600,000-row Cartesian product), the optimized plan with the reasons for each rewrite, and the executed plan with estimated and actual row counts per operator. `compare` runs four queries with and without the optimizer. `test_minisql.py` checks 33 queries against SQLite under both plans; all agree. `sqlite_plans.py` shows SQLite 3.49.1's `EXPLAIN QUERY PLAN`, `ANALYZE` statistics and bytecode for the same data, and `index_tradeoff.py` times index versus full scan on 1 million rows.

## Documents (Traditional Chinese)

| Document | Content |
| --- | --- |
| [Main report](README.zh-TW.md) | Conclusions, document map, quick start, scope |
| [01 How SQL works](research/01-how-sql-works.zh-TW.md) | Relational model and algebra, SQL history and standards, clause-to-operator mapping, NULL and duplicates, query pipeline, optimizer (equivalences, join order, cardinality estimation, index trade-off), iterators and pipelining, join algorithms, SQLite bytecode walkthrough, B-trees and statistics, common misconceptions |
| [02 Measurements](research/02-measurements.zh-TW.md) | Rows and time before and after optimization, estimate versus actual, SQLite plans, index trade-off on 1 million rows, differential test coverage |
| [03 Sources](research/03-sources.zh-TW.md) | Primary papers, SQLite and PostgreSQL documentation, the SQL standard, with read status per source |
| [minisql lab](labs/minisql/README.zh-TW.md) | Usage, supported SQL, code walkthrough, eight exercises |
