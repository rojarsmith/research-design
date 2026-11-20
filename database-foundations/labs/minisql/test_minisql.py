"""Tests for minisql.

The main test is differential: every query runs on minisql twice (canonical plan
and optimized plan) and once on SQLite with the same data, and all three results
must agree. Run:  python -m unittest test_minisql -v
"""
import sqlite3
import unittest

import minisql
from minisql import SQLError, sample_database, sample_tables

QUERIES = [
    # (sql, ordered?)  ordered=False compares as multisets
    ("SELECT * FROM customers", False),
    ("SELECT id, name FROM customers WHERE city = 'Taipei'", False),
    ("SELECT id FROM customers WHERE city IS NULL", False),
    ("SELECT id FROM customers WHERE city = NULL", False),
    ("SELECT id FROM customers WHERE NOT (city = 'Taipei')", False),
    ("SELECT id FROM customers WHERE city <> 'Taipei' OR city IS NULL", False),
    ("SELECT COUNT(*), COUNT(amount), SUM(amount), MIN(amount), MAX(amount) FROM orders", True),
    ("SELECT AVG(amount) FROM orders", True),
    ("SELECT status, COUNT(*) FROM orders GROUP BY status ORDER BY status", True),
    ("SELECT status, COUNT(*) AS n FROM orders GROUP BY status HAVING COUNT(*) > 30 "
     "ORDER BY n DESC, status", True),
    ("SELECT c.name, o.amount FROM customers c JOIN orders o ON o.customer_id = c.id "
     "WHERE o.amount > 4000 ORDER BY o.amount DESC, o.id LIMIT 10", True),
    ("SELECT c.city, SUM(o.amount) FROM customers c, orders o WHERE c.id = o.customer_id "
     "GROUP BY c.city ORDER BY c.city", True),
    ("SELECT ci.region, COUNT(*) FROM orders o JOIN customers c ON o.customer_id = c.id "
     "JOIN cities ci ON ci.name = c.city GROUP BY ci.region ORDER BY ci.region", True),
    ("SELECT ci.region, COUNT(*) AS n FROM cities ci, orders o, customers c "
     "WHERE o.customer_id = c.id AND c.city = ci.name AND ci.region = 'South' "
     "GROUP BY ci.region", False),
    ("SELECT DISTINCT status FROM orders ORDER BY status", True),
    ("SELECT DISTINCT c.city FROM customers c ORDER BY 1", True),
    ("SELECT id, amount * 2 + 1 AS x FROM orders WHERE id <= 5 ORDER BY id", True),
    ("SELECT id, amount / 3, amount % 7, -amount, (amount - 3000) / 7 FROM orders "
     "WHERE id < 20 ORDER BY id", True),
    ("SELECT COUNT(*) FROM orders WHERE customer_id = 7", True),
    ("SELECT * FROM orders WHERE customer_id >= 35 ORDER BY id", True),
    ("SELECT id FROM orders WHERE 30 < customer_id AND customer_id <= 33", False),
    ("SELECT COUNT(*) FROM customers c, cities ci", True),
    ("SELECT SUM(amount), COUNT(*), AVG(amount) FROM orders WHERE id < 0", True),
    ("SELECT c.id, o.id FROM customers c JOIN orders o ON o.customer_id = c.id "
     "AND o.status = 'refunded' WHERE c.city = 'Kaohsiung' ORDER BY c.id, o.id", True),
    ("SELECT c.city FROM customers c JOIN orders o ON o.customer_id = c.id", False),
    ("SELECT name FROM customers WHERE id = 3 OR id = 5 ORDER BY name", True),
    ("SELECT status, MAX(day) - MIN(day) FROM orders GROUP BY status ORDER BY 2 DESC, 1", True),
    ("SELECT o.id FROM orders o JOIN customers c ON o.customer_id = c.id WHERE c.id IS NULL", False),
    ("SELECT COUNT(*) FROM orders WHERE customer_id = 999", True),
    ("SELECT o.id, c.id FROM orders o, customers c WHERE o.customer_id = c.id AND o.day < c.id", False),
    ("SELECT city, COUNT(*) FROM customers GROUP BY city ORDER BY COUNT(*) DESC, city", True),
    ("SELECT id FROM orders WHERE amount IS NULL AND status <> 'paid'", False),
    ("SELECT id FROM orders WHERE 1 = 1 AND id < 4", False),
]


def normalize(rows):
    def cell(v):
        return round(v, 6) if isinstance(v, float) else v
    return [tuple(cell(v) for v in r) for r in rows]


def as_multiset(rows):
    return sorted(rows, key=repr)


class Differential(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.db = sample_database(customers=40, orders=300)
        cls.conn = sqlite3.connect(":memory:")
        for name, (cols, rows) in sample_tables(customers=40, orders=300).items():
            cls.conn.execute(f"CREATE TABLE {name} ({', '.join(f'{c} {t}' for c, t in cols)})")
            cls.conn.executemany(f"INSERT INTO {name} VALUES ({', '.join('?' * len(cols))})", rows)

    def test_queries_match_sqlite(self):
        for sql, ordered in QUERIES:
            expected = normalize(self.conn.execute(sql).fetchall())
            for optimize in (False, True):
                with self.subTest(sql=sql, optimize=optimize):
                    got = normalize(self.db.execute(sql, optimize).rows)
                    if ordered:
                        self.assertEqual(got, expected)
                    else:
                        self.assertEqual(as_multiset(got), as_multiset(expected))


class Semantics(unittest.TestCase):
    def setUp(self):
        self.db = minisql.Database()
        self.db.execute("""
            CREATE TABLE t (a INT, b TEXT);
            INSERT INTO t VALUES (1, 'x'), (2, NULL), (NULL, 'y'), (2, 'x');
        """)

    def rows(self, sql):
        return self.db.execute(sql).rows

    def test_null_is_not_equal_to_null(self):
        self.assertEqual(self.rows("SELECT COUNT(*) FROM t WHERE a = NULL"), [(0,)])
        self.assertEqual(self.rows("SELECT COUNT(*) FROM t WHERE a IS NULL"), [(1,)])

    def test_not_of_unknown_is_unknown(self):
        # b <> 'x' is UNKNOWN for b = NULL, and NOT UNKNOWN is still UNKNOWN
        self.assertEqual(self.rows("SELECT a FROM t WHERE NOT (b = 'x')"), [(None,)])

    def test_duplicates_kept_unless_distinct(self):
        self.assertEqual(len(self.rows("SELECT a FROM t WHERE a = 2")), 2)
        self.assertEqual(len(self.rows("SELECT DISTINCT a FROM t WHERE a = 2")), 1)

    def test_aggregates_on_empty_input(self):
        self.assertEqual(self.rows("SELECT COUNT(*), SUM(a), AVG(a) FROM t WHERE a > 100"),
                         [(0, None, None)])
        self.assertEqual(self.rows("SELECT b, COUNT(*) FROM t WHERE a > 100 GROUP BY b"), [])

    def test_errors(self):
        for sql, message in [
            ("SELECT a, COUNT(*) FROM t GROUP BY b", "must appear in GROUP BY"),
            ("SELECT c FROM t", "no such column"),
            ("SELECT a FROM t WHERE COUNT(*) > 1", "not allowed in WHERE"),
            ("SELECT a FROM t, t", "duplicate"),
            ("SELECT a FROM t x, t y", "ambiguous"),
            ("SELECT a FROM nope", "no such table"),
            ("SELECT a FROM t WHERE", "expected an expression"),
            ("SELECT 'abc FROM t", "unterminated string"),
            ("INSERT INTO t VALUES ('one', 'x')", "is int"),
        ]:
            with self.subTest(sql=sql):
                with self.assertRaises(SQLError) as ctx:
                    self.db.execute(sql)
                self.assertIn(message, str(ctx.exception))


class Planner(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.db = sample_database()

    def plan_text(self, sql, optimize=True):
        return "\n".join(minisql.explain(self.db.plan(sql, optimize)))

    def test_canonical_plan_is_product_then_filter(self):
        text = self.plan_text(minisql.DEMO_SQL, optimize=False)
        self.assertIn("NestedLoopJoin ×", text)
        self.assertNotIn("HashJoin", text)

    def test_optimizer_pushes_filters_and_hash_joins(self):
        plan = self.db.plan(minisql.DEMO_SQL)
        text = "\n".join(minisql.explain(plan))
        self.assertIn("HashJoin o.customer_id = c.id", text)
        self.assertNotIn("NestedLoopJoin", text)
        lines = text.splitlines()
        status = next(i for i, l in enumerate(lines) if "Filter o.status = 'paid'" in l)
        self.assertIn("SeqScan orders AS o", lines[status + 1])

    def test_index_used_for_selective_lookup(self):
        plan = self.db.plan("SELECT * FROM orders WHERE customer_id = 42")
        rows = list(plan)
        text = "\n".join(minisql.explain(plan, analyze=True))
        self.assertIn("IndexScan orders USING orders_customer_id", text)
        self.assertNotIn("SeqScan", text)
        self.assertEqual(len(rows), 14)

    def test_index_skipped_when_not_selective(self):
        text = self.plan_text("SELECT * FROM orders WHERE customer_id > 10")
        self.assertIn("SeqScan orders", text)

    def test_join_order_avoids_cartesian_product(self):
        sql = ("SELECT COUNT(*) FROM cities ci, orders o, customers c "
               "WHERE o.customer_id = c.id AND c.city = ci.name")
        self.assertIn("NestedLoopJoin", self.plan_text(sql, optimize=False))
        self.assertNotIn("NestedLoopJoin", self.plan_text(sql))

    def test_limit_stops_the_scan_early(self):
        plan = self.db.plan("SELECT * FROM orders LIMIT 5")
        self.assertEqual(len(list(plan)), 5)
        scan = plan
        while scan.children:
            scan = scan.children[0]
        self.assertEqual(scan.actual, 5)

    def test_sort_blocks_the_pipeline(self):
        plan = self.db.plan("SELECT * FROM orders ORDER BY amount LIMIT 5")
        list(plan)
        scan = plan
        while scan.children:
            scan = scan.children[0]
        self.assertEqual(scan.actual, 3000)


class Lexer(unittest.TestCase):
    def test_tokens(self):
        kinds = [(t.kind, t.value) for t in minisql.tokenize("select a.b, 'it''s' -- note\n<> 1.5")]
        self.assertEqual(kinds, [
            ("KEYWORD", "SELECT"), ("IDENT", "a"), ("OP", "."), ("IDENT", "b"), ("OP", ","),
            ("STRING", "it's"), ("OP", "<>"), ("NUMBER", 1.5), ("END", None)])


if __name__ == "__main__":
    unittest.main()
