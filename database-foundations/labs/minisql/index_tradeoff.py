#!/usr/bin/env python3
"""When does an index help? Time SQLite on 1,000,000 rows with and without it.

    python index_tradeoff.py

A unary + in front of a column (+customer_id) is a documented SQLite idiom that
stops the planner from using an index on that column, so the same query can be
timed both ways. Each query runs five times and the best time is printed.
Timings depend on the machine; the ratio is the point.
"""
import sqlite3
import sys
import time

import minisql

ROWS = 1_000_000
QUERIES = [
    ("約 0.5% 的列（等值）", "customer_id = 42"),
    ("約 95% 的列（範圍）", "customer_id > 10"),
]


def best_ms(conn, sql, runs=5):
    best = float("inf")
    for _ in range(runs):
        start = time.perf_counter()
        conn.execute(sql).fetchall()
        best = min(best, time.perf_counter() - start)
    return best * 1000


def main():
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except AttributeError:
        pass
    conn = sqlite3.connect(":memory:")
    conn.execute("CREATE TABLE big (id INTEGER, customer_id INT, amount INT)")
    rng = minisql._Lcg(7)
    conn.executemany("INSERT INTO big VALUES (?, ?, ?)",
                     ((i, 1 + rng.below(200), 100 + rng.below(5000)) for i in range(ROWS)))
    conn.execute("CREATE INDEX big_customer_id ON big(customer_id)")
    conn.execute("ANALYZE")
    print(f"SQLite {sqlite3.sqlite_version}，{ROWS:,} 列，customer_id 均勻分布在 1–200，已 ANALYZE")
    for label, cond in QUERIES:
        matched = conn.execute(f"SELECT COUNT(*) FROM big WHERE {cond}").fetchone()[0]
        print(f"\n{label}：WHERE {cond}（實際 {matched:,} 列）")
        for prefix in ("", "+"):
            sql = f"SELECT SUM(amount) FROM big WHERE {prefix}{cond}"
            plan = conn.execute("EXPLAIN QUERY PLAN " + sql).fetchall()[0][3]
            print(f"  {best_ms(conn, sql):7.1f} ms  {plan}")


if __name__ == "__main__":
    main()
