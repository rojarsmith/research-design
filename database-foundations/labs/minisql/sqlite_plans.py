#!/usr/bin/env python3
"""Show how SQLite plans and compiles queries on the same sample data as minisql.

    python sqlite_plans.py

Uses only the sqlite3 module that ships with Python. EXPLAIN QUERY PLAN and
EXPLAIN output is meant for people, and SQLite says its format can change
between releases, so the text depends on the SQLite version printed first.
"""
import sqlite3
import sys

import minisql

POINT = "SELECT id, amount FROM orders WHERE customer_id = 42"


def load(conn, tables):
    for name, (cols, rows) in tables.items():
        conn.execute(f"CREATE TABLE {name} ({', '.join(f'{c} {t}' for c, t in cols)})")
        conn.executemany(f"INSERT INTO {name} VALUES ({', '.join('?' * len(cols))})", rows)


def query_plan(conn, sql):
    """EXPLAIN QUERY PLAN rows are (id, parent, unused, detail); draw them as a tree."""
    rows = conn.execute("EXPLAIN QUERY PLAN " + sql).fetchall()
    kids = {}
    for node_id, parent, _, detail in rows:
        kids.setdefault(parent, []).append((node_id, detail))

    def build(parent):
        return [(detail, build(node_id)) for node_id, detail in kids.get(parent, [])]
    return minisql.render_tree(("QUERY PLAN", build(0)))


def show(conn, title, sql):
    print(f"\n== {title} ==")
    print(sql)
    print("\n".join(query_plan(conn, sql)))


def main():
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except AttributeError:
        pass
    print(f"SQLite {sqlite3.sqlite_version}")
    conn = sqlite3.connect(":memory:")
    tables = minisql.sample_tables()
    load(conn, tables)

    show(conn, "1. 沒有索引：只能全表掃描", POINT)
    conn.execute("CREATE INDEX orders_customer_id ON orders(customer_id)")
    show(conn, "2. 建立 orders(customer_id) 索引後：改成在索引 B-tree 裡搜尋", POINT)
    show(conn, "3. 只需要索引裡就有的欄位：覆蓋索引，不必回表",
         "SELECT COUNT(*) FROM orders WHERE customer_id = 42")
    show(conn, "4. 範圍很大的條件（尚未 ANALYZE）",
         "SELECT id, amount FROM orders WHERE customer_id > 10")
    show(conn, "5. ORDER BY 的欄位沒有索引：先放進暫存 B-tree 排序",
         "SELECT id, amount FROM orders ORDER BY amount DESC LIMIT 5")
    show(conn, "6. demo 查詢（尚未 ANALYZE）", minisql.DEMO_SQL)
    three = ("SELECT ci.region, COUNT(*) AS n FROM cities ci, orders o, customers c\n"
             "WHERE o.customer_id = c.id AND c.city = ci.name AND ci.region = 'South'\n"
             "GROUP BY ci.region")
    show(conn, "7. 三表連接，FROM 的順序是 cities, orders, customers", three)

    conn.execute("ANALYZE")
    print("\n== 8. ANALYZE 之後，sqlite_stat1 存下的統計 ==")
    for row in conn.execute("SELECT tbl, idx, stat FROM sqlite_stat1 ORDER BY tbl, idx"):
        print(row)
    show(conn, "9. 範圍很大的條件（ANALYZE 之後）",
         "SELECT id, amount FROM orders WHERE customer_id > 10")
    show(conn, "10. demo 查詢（ANALYZE 之後）", minisql.DEMO_SQL)
    show(conn, "11. 三表連接（ANALYZE 之後）", three)

    print("\n== 12. 同一條點查詢編譯成的 bytecode（EXPLAIN）==")
    print(POINT)
    print(f"{'addr':>4}  {'opcode':<14}{'p1':>4}{'p2':>4}{'p3':>4}  p4")
    for addr, opcode, p1, p2, p3, p4, p5, comment in conn.execute("EXPLAIN " + POINT):
        p4 = "" if p4 is None else p4
        print(f"{addr:>4}  {opcode:<14}{p1:>4}{p2:>4}{p3:>4}  {p4}")

    print("\n== 13. 與 minisql 對照結果 ==")
    db = minisql.sample_database()
    for sql in (minisql.DEMO_SQL, POINT + " ORDER BY id", three):
        a = conn.execute(sql).fetchall()
        b = db.execute(sql).rows
        first = sql.splitlines()[0]
        print(f"{'相同' if sorted(a, key=repr) == sorted(b, key=repr) else '不同'}：{first}")


if __name__ == "__main__":
    main()
