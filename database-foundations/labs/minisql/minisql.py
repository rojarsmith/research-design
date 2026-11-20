#!/usr/bin/env python3
"""minisql: a teaching-sized relational query engine (Python standard library only).

Pipeline:
    SQL text -> tokenize() -> Parser -> AST -> Planner -> plan of operators -> rows
                                               |                 |
                     canonical relational algebra   Volcano iterators: open() / next() / close()
                     or optimizer rewrites

Usage:
    python minisql.py demo ["SQL"]          walk one query through every stage
    python minisql.py compare               same queries with and without the optimizer
    python minisql.py query "SQL" [--no-opt] [--plan]
    python minisql.py run script.sql [--sample]

The walkthrough is in README.zh-TW.md next to this file.
"""
from __future__ import annotations

import bisect
import math
import operator
import sys
import time
from dataclasses import dataclass


class SQLError(Exception):
    """An error reported to the user: bad syntax, unknown column, and so on."""


# ===================================================================== 1. lexer

KEYWORDS = {
    "SELECT", "DISTINCT", "FROM", "WHERE", "GROUP", "BY", "HAVING", "ORDER", "ASC",
    "DESC", "LIMIT", "JOIN", "INNER", "ON", "AS", "AND", "OR", "NOT", "IS", "NULL",
    "CREATE", "TABLE", "INDEX", "INSERT", "INTO", "VALUES", "EXPLAIN", "ANALYZE",
}
TYPES = {"INT": int, "INTEGER": int, "REAL": float, "TEXT": str}


@dataclass(frozen=True)
class Token:
    kind: str  # KEYWORD, IDENT, NUMBER, STRING, OP, END
    value: object
    pos: int

    def __str__(self):
        if self.kind == "END":
            return "END"
        if self.kind == "STRING":
            return f"STRING '{self.value}'"
        return f"{self.kind} {self.value}"


def tokenize(sql: str) -> list[Token]:
    """Turn characters into tokens. Unquoted identifiers are case-insensitive."""
    tokens, i, n = [], 0, len(sql)
    while i < n:
        ch = sql[i]
        if ch.isspace():
            i += 1
        elif sql.startswith("--", i):  # comment to end of line
            j = sql.find("\n", i)
            i = n if j < 0 else j
        elif ch.isdigit():
            j = i
            while j < n and sql[j].isdigit():
                j += 1
            if j < n and sql[j] == "." and j + 1 < n and sql[j + 1].isdigit():
                j += 1
                while j < n and sql[j].isdigit():
                    j += 1
                tokens.append(Token("NUMBER", float(sql[i:j]), i))
            else:
                tokens.append(Token("NUMBER", int(sql[i:j]), i))
            i = j
        elif ch == "'":
            j, text = i + 1, []
            while True:
                if j >= n:
                    raise SQLError(f"unterminated string starting at position {i}")
                if sql[j] == "'":
                    if j + 1 < n and sql[j + 1] == "'":  # '' is an escaped quote
                        text.append("'")
                        j += 2
                        continue
                    break
                text.append(sql[j])
                j += 1
            tokens.append(Token("STRING", "".join(text), i))
            i = j + 1
        elif ch.isalpha() or ch == "_":
            j = i
            while j < n and (sql[j].isalnum() or sql[j] == "_"):
                j += 1
            word = sql[i:j]
            if word.upper() in KEYWORDS:
                tokens.append(Token("KEYWORD", word.upper(), i))
            else:
                tokens.append(Token("IDENT", word.lower(), i))
            i = j
        elif sql.startswith(("<>", "!=", "<=", ">="), i):
            op = sql[i:i + 2]
            tokens.append(Token("OP", "<>" if op == "!=" else op, i))
            i += 2
        elif ch in "=<>+-*/%(),.;":
            tokens.append(Token("OP", ch, i))
            i += 1
        else:
            raise SQLError(f"unexpected character {ch!r} at position {i}")
    tokens.append(Token("END", None, n))
    return tokens


# ======================================================================= 2. AST
# Expressions are frozen dataclasses, so two identical expressions compare equal.
# The planner relies on that to match "c.city" in SELECT with "c.city" in GROUP BY.

@dataclass(frozen=True)
class Literal:
    value: object


@dataclass(frozen=True)
class Column:
    table: str | None
    name: str


@dataclass(frozen=True)
class Unary:
    op: str  # "NOT" or "-"
    operand: object


@dataclass(frozen=True)
class Binary:
    op: str  # AND OR = <> < <= > >= + - * / %
    left: object
    right: object


@dataclass(frozen=True)
class IsNull:
    operand: object
    negated: bool


@dataclass(frozen=True)
class Call:
    name: str  # count sum avg min max
    arg: object  # None means count(*)


@dataclass(frozen=True)
class Slot:
    """A column of an aggregate's output row (created by the planner, never parsed)."""
    index: int
    label: str


AGGREGATES = {"count", "sum", "avg", "min", "max"}
COMPARISONS = {"=", "<>", "<", "<=", ">", ">="}
_PREC = {"OR": 1, "AND": 2, "=": 4, "<>": 4, "<": 4, "<=": 4, ">": 4, ">=": 4,
         "+": 5, "-": 5, "*": 6, "/": 6, "%": 6}


def to_sql(e, parent: int = 0) -> str:
    """Print an expression back as SQL, adding parentheses only where needed."""
    if isinstance(e, Literal):
        v = e.value
        if v is None:
            return "NULL"
        if isinstance(v, str):
            return "'" + v.replace("'", "''") + "'"
        return repr(v)
    if isinstance(e, Column):
        return f"{e.table}.{e.name}" if e.table else e.name
    if isinstance(e, Slot):
        return e.label
    if isinstance(e, Call):
        return f"{e.name}({'*' if e.arg is None else to_sql(e.arg)})"
    if isinstance(e, Unary):
        if e.op == "-":
            return "-" + to_sql(e.operand, 7)
        text, prec = "NOT " + to_sql(e.operand, 3), 3
    elif isinstance(e, IsNull):
        text, prec = to_sql(e.operand, 5) + (" IS NOT NULL" if e.negated else " IS NULL"), 4
    elif isinstance(e, Binary):
        prec = _PREC[e.op]
        text = f"{to_sql(e.left, prec)} {e.op} {to_sql(e.right, prec + 1)}"
    else:
        raise TypeError(e)
    return f"({text})" if prec < parent else text


def walk(e):
    """Yield e and every sub-expression."""
    if e is None:
        return
    yield e
    if isinstance(e, Binary):
        yield from walk(e.left)
        yield from walk(e.right)
    elif isinstance(e, (Unary, IsNull)):
        yield from walk(e.operand)
    elif isinstance(e, Call):
        yield from walk(e.arg)


def columns_in(e):
    return [x for x in walk(e) if isinstance(x, Column)]


def calls_in(e):
    return [x for x in walk(e) if isinstance(x, Call)]


def conjuncts_of(e):
    """Split a AND b AND c into [a, b, c]."""
    if isinstance(e, Binary) and e.op == "AND":
        return conjuncts_of(e.left) + conjuncts_of(e.right)
    return [e]


def and_all(preds):
    out = preds[0]
    for p in preds[1:]:
        out = Binary("AND", out, p)
    return out


@dataclass
class TableRef:
    name: str
    alias: str


@dataclass
class SelectItem:
    expr: object  # None means * (or alias.* when star_table is set)
    alias: str | None
    star_table: str | None = None


@dataclass
class Select:
    distinct: bool
    items: list
    tables: list  # TableRef, in FROM order
    joins_on: list  # ON condition that introduced tables[i]; None for commas and the first table
    where: object
    group_by: list
    having: object
    order_by: list  # (expr, descending)
    limit: int | None


@dataclass
class CreateTable:
    name: str
    columns: list  # (name, type)


@dataclass
class CreateIndex:
    name: str
    table: str
    column: str


@dataclass
class Insert:
    table: str
    columns: list | None
    rows: list


@dataclass
class Explain:
    query: Select
    analyze: bool


# ==================================================================== 3. parser

class Parser:
    """Recursive descent. Each grammar rule is one method; precedence comes from
    which method calls which (or -> and -> not -> comparison -> + - -> * / % -> unary)."""

    def __init__(self, sql: str):
        self.tokens = tokenize(sql)
        self.i = 0

    # ---- token helpers
    def peek(self, offset=0):
        return self.tokens[min(self.i + offset, len(self.tokens) - 1)]

    def at(self, kind, value=None, offset=0):
        t = self.peek(offset)
        return t.kind == kind and (value is None or t.value == value)

    def take(self, kind, value=None):
        if self.at(kind, value):
            self.i += 1
            return self.tokens[self.i - 1]
        return None

    def need(self, kind, value=None):
        t = self.take(kind, value)
        if t is None:
            got = self.peek()
            want = value if value is not None else kind.lower()
            found = "end of input" if got.kind == "END" else str(got)
            raise SQLError(f"expected {want} at position {got.pos}, found {found}")
        return t

    def ident(self):
        return self.need("IDENT").value

    # ---- statements
    def script(self):
        stmts = []
        while not self.at("END"):
            if self.take("OP", ";"):
                continue
            stmts.append(self.statement())
            if not self.at("END"):
                self.need("OP", ";")
        return stmts

    def statement(self):
        if self.take("KEYWORD", "EXPLAIN"):
            analyze = self.take("KEYWORD", "ANALYZE") is not None
            return Explain(self.select(), analyze)
        if self.at("KEYWORD", "SELECT"):
            return self.select()
        if self.take("KEYWORD", "CREATE"):
            if self.take("KEYWORD", "TABLE"):
                return self.create_table()
            self.need("KEYWORD", "INDEX")
            return self.create_index()
        if self.take("KEYWORD", "INSERT"):
            return self.insert()
        t = self.peek()
        raise SQLError(f"expected a statement at position {t.pos}, found {t}")

    def create_table(self):
        name = self.ident()
        self.need("OP", "(")
        cols = []
        while True:
            col = self.ident()
            typ = self.ident().upper()
            if typ not in TYPES:
                raise SQLError(f"unknown type {typ}; use INT, REAL or TEXT")
            cols.append((col, typ))
            if not self.take("OP", ","):
                break
        self.need("OP", ")")
        return CreateTable(name, cols)

    def create_index(self):
        name = self.ident()
        self.need("KEYWORD", "ON")
        table = self.ident()
        self.need("OP", "(")
        col = self.ident()
        self.need("OP", ")")
        return CreateIndex(name, table, col)

    def insert(self):
        self.need("KEYWORD", "INTO")
        table, cols = self.ident(), None
        if self.take("OP", "("):
            cols = [self.ident()]
            while self.take("OP", ","):
                cols.append(self.ident())
            self.need("OP", ")")
        self.need("KEYWORD", "VALUES")
        rows = []
        while True:
            self.need("OP", "(")
            row = [self.expr()]
            while self.take("OP", ","):
                row.append(self.expr())
            self.need("OP", ")")
            rows.append(row)
            if not self.take("OP", ","):
                break
        return Insert(table, cols, rows)

    def select(self):
        self.need("KEYWORD", "SELECT")
        distinct = self.take("KEYWORD", "DISTINCT") is not None
        items = [self.select_item()]
        while self.take("OP", ","):
            items.append(self.select_item())
        self.need("KEYWORD", "FROM")
        tables, joins_on = [self.table_ref()], [None]
        while True:
            if self.take("OP", ","):
                tables.append(self.table_ref())
                joins_on.append(None)
            elif self.at("KEYWORD", "JOIN") or self.at("KEYWORD", "INNER"):
                self.take("KEYWORD", "INNER")
                self.need("KEYWORD", "JOIN")
                tables.append(self.table_ref())
                self.need("KEYWORD", "ON")
                joins_on.append(self.expr())
            else:
                break
        where = self.expr() if self.take("KEYWORD", "WHERE") else None
        group_by = []
        if self.take("KEYWORD", "GROUP"):
            self.need("KEYWORD", "BY")
            group_by.append(self.expr())
            while self.take("OP", ","):
                group_by.append(self.expr())
        having = self.expr() if self.take("KEYWORD", "HAVING") else None
        order_by = []
        if self.take("KEYWORD", "ORDER"):
            self.need("KEYWORD", "BY")
            while True:
                e = self.expr()
                desc = self.take("KEYWORD", "DESC") is not None
                if not desc:
                    self.take("KEYWORD", "ASC")
                order_by.append((e, desc))
                if not self.take("OP", ","):
                    break
        limit = None
        if self.take("KEYWORD", "LIMIT"):
            t = self.need("NUMBER")
            if not isinstance(t.value, int):
                raise SQLError(f"LIMIT needs an integer at position {t.pos}")
            limit = t.value
        return Select(distinct, items, tables, joins_on, where, group_by, having, order_by, limit)

    def select_item(self):
        if self.take("OP", "*"):
            return SelectItem(None, None)
        if self.at("IDENT") and self.at("OP", ".", 1) and self.at("OP", "*", 2):
            table = self.ident()
            self.i += 2
            return SelectItem(None, None, table)
        e = self.expr()
        alias = None
        if self.take("KEYWORD", "AS"):
            alias = self.ident()
        elif self.at("IDENT"):
            alias = self.ident()
        return SelectItem(e, alias)

    def table_ref(self):
        name = alias = self.ident()
        if self.take("KEYWORD", "AS"):
            alias = self.ident()
        elif self.at("IDENT"):
            alias = self.ident()
        return TableRef(name, alias)

    # ---- expressions, lowest precedence first
    def expr(self):
        e = self.and_expr()
        while self.take("KEYWORD", "OR"):
            e = Binary("OR", e, self.and_expr())
        return e

    def and_expr(self):
        e = self.not_expr()
        while self.take("KEYWORD", "AND"):
            e = Binary("AND", e, self.not_expr())
        return e

    def not_expr(self):
        if self.take("KEYWORD", "NOT"):
            return Unary("NOT", self.not_expr())
        return self.comparison()

    def comparison(self):
        e = self.additive()
        for op in ("=", "<>", "<=", ">=", "<", ">"):
            if self.take("OP", op):
                return Binary(op, e, self.additive())
        if self.take("KEYWORD", "IS"):
            negated = self.take("KEYWORD", "NOT") is not None
            self.need("KEYWORD", "NULL")
            return IsNull(e, negated)
        return e

    def additive(self):
        e = self.multiplicative()
        while self.at("OP", "+") or self.at("OP", "-"):
            op = self.take("OP").value
            e = Binary(op, e, self.multiplicative())
        return e

    def multiplicative(self):
        e = self.unary()
        while self.at("OP", "*") or self.at("OP", "/") or self.at("OP", "%"):
            op = self.take("OP").value
            e = Binary(op, e, self.unary())
        return e

    def unary(self):
        if self.take("OP", "-"):
            operand = self.unary()
            if isinstance(operand, Literal) and isinstance(operand.value, (int, float)):
                return Literal(-operand.value)
            return Unary("-", operand)
        return self.primary()

    def primary(self):
        t = self.peek()
        if self.take("NUMBER") or self.take("STRING"):
            return Literal(t.value)
        if self.take("KEYWORD", "NULL"):
            return Literal(None)
        if self.take("OP", "("):
            e = self.expr()
            self.need("OP", ")")
            return e
        if self.at("IDENT"):
            name = self.ident()
            if self.take("OP", "("):
                if name not in AGGREGATES:
                    raise SQLError(f"unknown function {name}() at position {t.pos}")
                if self.take("OP", "*"):
                    if name != "count":
                        raise SQLError(f"{name}(*) is not valid at position {t.pos}")
                    arg = None
                else:
                    arg = self.expr()
                self.need("OP", ")")
                return Call(name, arg)
            if self.take("OP", "."):
                return Column(name, self.ident())
            return Column(None, name)
        found = "end of input" if t.kind == "END" else str(t)
        raise SQLError(f"expected an expression at position {t.pos}, found {found}")


# ========================================================== 4. expression compiler
# NULL is Python None. Comparisons and AND/OR/NOT use SQL's three-valued logic:
# True, False, or None (UNKNOWN). WHERE and HAVING keep a row only when the
# predicate is True.

def sort_key(v):
    """SQLite's cross-type order: NULL < numbers < text."""
    if v is None:
        return (0, 0)
    if isinstance(v, str):
        return (2, v)
    return (1, v)


def truth(v):
    return None if v is None else bool(v)


def sql_and(a, b):
    a, b = truth(a), truth(b)
    if a is False or b is False:
        return False
    if a is None or b is None:
        return None
    return True


def sql_or(a, b):
    a, b = truth(a), truth(b)
    if a is True or b is True:
        return True
    if a is None or b is None:
        return None
    return False


def sql_not(a):
    a = truth(a)
    return None if a is None else not a


def _div(a, b):
    if b == 0:
        return None  # SQLite returns NULL for division by zero
    if isinstance(a, int) and isinstance(b, int):  # integer division truncates toward zero
        q = abs(a) // abs(b)
        return q if (a >= 0) == (b >= 0) else -q
    return a / b


def _mod(a, b):
    if b == 0:
        return None
    if isinstance(a, int) and isinstance(b, int):  # result takes the sign of a, as in C
        r = abs(a) % abs(b)
        return r if a >= 0 else -r
    return math.fmod(a, b)


_CMP = {"=": operator.eq, "<>": operator.ne, "<": operator.lt, "<=": operator.le,
        ">": operator.gt, ">=": operator.ge}
_ARITH = {"+": operator.add, "-": operator.sub, "*": operator.mul, "/": _div, "%": _mod}
_FLIP = {"=": "=", "<>": "<>", "<": ">", "<=": ">=", ">": "<", ">=": "<="}


def resolve_column(schema, col):
    """schema is a list of (qualifier, name). Return the position of col in a row."""
    hits = [i for i, (q, n) in enumerate(schema)
            if n == col.name and (col.table is None or q == col.table)]
    if not hits:
        raise SQLError(f"no such column: {to_sql(col)}")
    if len(hits) > 1:
        raise SQLError(f"ambiguous column name: {to_sql(col)}")
    return hits[0]


def compile_expr(e, schema):
    """Turn an expression tree into a Python function row -> value."""
    if isinstance(e, Literal):
        v = e.value
        return lambda row: v
    if isinstance(e, (Column, Slot)):
        i = e.index if isinstance(e, Slot) else resolve_column(schema, e)
        return lambda row: row[i]
    if isinstance(e, Call):
        raise SQLError(f"aggregate {to_sql(e)} is not allowed here")
    if isinstance(e, IsNull):
        f = compile_expr(e.operand, schema)
        if e.negated:
            return lambda row: f(row) is not None
        return lambda row: f(row) is None
    if isinstance(e, Unary):
        f = compile_expr(e.operand, schema)
        if e.op == "NOT":
            return lambda row: sql_not(f(row))
        return lambda row: None if (v := f(row)) is None else -v
    if isinstance(e, Binary):
        left, right = compile_expr(e.left, schema), compile_expr(e.right, schema)
        if e.op == "AND":
            return lambda row: False if truth(a := left(row)) is False else sql_and(a, right(row))
        if e.op == "OR":
            return lambda row: True if truth(a := left(row)) is True else sql_or(a, right(row))
        if e.op in _CMP:
            cmp = _CMP[e.op]

            def compare(row):
                a, b = left(row), right(row)
                if a is None or b is None:
                    return None  # anything compared with NULL is UNKNOWN
                return cmp(sort_key(a), sort_key(b))
            return compare
        fn = _ARITH[e.op]

        def arith(row):
            a, b = left(row), right(row)
            if a is None or b is None:
                return None
            try:
                return fn(a, b)
            except TypeError:
                raise SQLError(f"cannot apply {e.op} to {a!r} and {b!r}") from None
        return arith
    raise TypeError(e)


# ================================================================= 5. storage

class Table:
    def __init__(self, name, columns):
        self.name = name
        self.columns = [c for c, _ in columns]
        self.type_names = [t for _, t in columns]
        self.types = [TYPES[t] for t in self.type_names]
        self.rows: list[tuple] = []
        self.indexes: dict[str, Index] = {}  # column -> index
        self._stats: dict[str, tuple] = {}

    def column_index(self, col):
        try:
            return self.columns.index(col)
        except ValueError:
            raise SQLError(f"no such column: {self.name}.{col}") from None

    def insert(self, values):
        row = []
        for v, typ, col in zip(values, self.types, self.columns):
            if v is not None:
                if typ is float and isinstance(v, int):
                    v = float(v)
                elif not isinstance(v, typ):
                    raise SQLError(f"{self.name}.{col} is {typ.__name__}, got {v!r}")
            row.append(v)
        self.rows.append(tuple(row))
        self._stats.clear()
        for index in self.indexes.values():
            index.dirty = True

    def stats(self, col):
        """(distinct non-NULL values, fraction of NULLs): roughly what ANALYZE stores."""
        if col not in self._stats:
            i = self.column_index(col)
            values = [r[i] for r in self.rows]
            present = [v for v in values if v is not None]
            nulls = (len(values) - len(present)) / len(values) if values else 0.0
            self._stats[col] = (max(1, len(set(present))), nulls)
        return self._stats[col]


class Index:
    """A sorted list of (key, row position): an in-memory stand-in for a B+tree.
    Like a B+tree it answers = and range lookups in O(log n) and returns rows in key order."""

    def __init__(self, name, table, column):
        self.name, self.table, self.column = name, table, column
        table.column_index(column)
        self.dirty = True

    def _build(self):
        i = self.table.column_index(self.column)
        entries = sorted((r[i], pos) for pos, r in enumerate(self.table.rows) if r[i] is not None)
        self.keys = [k for k, _ in entries]
        self.positions = [p for _, p in entries]
        self.dirty = False

    def lookup(self, op, value):
        if self.dirty:
            self._build()
        keys = self.keys
        lo, hi = {
            "=": lambda: (bisect.bisect_left(keys, value), bisect.bisect_right(keys, value)),
            "<": lambda: (0, bisect.bisect_left(keys, value)),
            "<=": lambda: (0, bisect.bisect_right(keys, value)),
            ">": lambda: (bisect.bisect_right(keys, value), len(keys)),
            ">=": lambda: (bisect.bisect_left(keys, value), len(keys)),
        }[op]()
        return self.positions[lo:hi]


# ============================================================== 6. operators
# Every operator is a Volcano-style iterator. The parent calls child.next() to
# pull one row at a time; None means "no more rows". Pipelined operators
# (scan, filter, project, limit, the probe side of a hash join) hold one row at
# a time. Blocking operators (sort, aggregate, the build side of a hash join)
# must read their whole input in open() before they can return the first row.

class Operator:
    children: tuple = ()
    schema: list = []

    def __init__(self):
        self.est = 0.0  # rows the planner expects
        self.actual = 0  # rows actually produced (EXPLAIN ANALYZE)

    def open(self):
        self.actual = 0
        for c in self.children:
            c.open()

    def next(self):
        row = self._next()
        if row is not None:
            self.actual += 1
        return row

    def _next(self):
        raise NotImplementedError

    def close(self):
        for c in self.children:
            c.close()

    def __iter__(self):
        self.open()
        try:
            while (row := self.next()) is not None:
                yield row
        finally:
            self.close()

    def label(self):
        return type(self).__name__

    def algebra(self):
        return self.label()


def _table_label(table, alias):
    return table.name if alias == table.name else f"{table.name} AS {alias}"


class SeqScan(Operator):
    def __init__(self, table, alias):
        super().__init__()
        self.table, self.alias = table, alias
        self.schema = [(alias, c) for c in table.columns]
        self.est = len(table.rows)

    def open(self):
        super().open()
        self.pos = 0

    def _next(self):
        if self.pos >= len(self.table.rows):
            return None
        self.pos += 1
        return self.table.rows[self.pos - 1]

    def label(self):
        return "SeqScan " + _table_label(self.table, self.alias)

    def algebra(self):
        return _table_label(self.table, self.alias)


class IndexScan(Operator):
    def __init__(self, table, alias, index, op, value, est):
        super().__init__()
        self.table, self.alias, self.index, self.op, self.value = table, alias, index, op, value
        self.schema = [(alias, c) for c in table.columns]
        self.est = est

    def open(self):
        super().open()
        self.positions = self.index.lookup(self.op, self.value)
        self.pos = 0

    def _next(self):
        if self.pos >= len(self.positions):
            return None
        self.pos += 1
        return self.table.rows[self.positions[self.pos - 1]]

    def condition(self):
        return f"{self.alias}.{self.index.column} {self.op} {to_sql(Literal(self.value))}"

    def label(self):
        return (f"IndexScan {_table_label(self.table, self.alias)} "
                f"USING {self.index.name} ({self.condition()})")

    def algebra(self):
        return f"σ[{self.condition()}]({_table_label(self.table, self.alias)})"


class Filter(Operator):
    """σ: pass through the rows whose predicate is True (not False, not UNKNOWN)."""

    def __init__(self, child, predicate, est):
        super().__init__()
        self.children = (child,)
        self.child, self.predicate = child, predicate
        self.schema = child.schema
        self.fn = compile_expr(predicate, self.schema)
        self.est = est

    def _next(self):
        fn, child = self.fn, self.child
        while (row := child.next()) is not None:
            if truth(fn(row)) is True:
                return row
        return None

    def label(self):
        return "Filter " + to_sql(self.predicate)

    def algebra(self):
        return f"σ[{to_sql(self.predicate)}]({self.child.algebra()})"


class NestedLoopJoin(Operator):
    """×: pair every left row with every right row (the right input is read once and kept)."""

    def __init__(self, left, right, est):
        super().__init__()
        self.children = (left, right)
        self.left, self.right = left, right
        self.schema = left.schema + right.schema
        self.est = est

    def open(self):
        super().open()
        self.inner = []
        while (row := self.right.next()) is not None:
            self.inner.append(row)
        self.outer, self.j = None, 0

    def _next(self):
        while self.outer is None or self.j >= len(self.inner):
            self.outer, self.j = self.left.next(), 0
            if self.outer is None:
                return None
        self.j += 1
        return self.outer + self.inner[self.j - 1]

    def label(self):
        return "NestedLoopJoin ×"

    def algebra(self):
        return f"({self.left.algebra()} × {self.right.algebra()})"


class HashJoin(Operator):
    """⋈ on equality: build a hash table on one input, stream the other and probe it."""

    def __init__(self, probe, build, probe_keys, build_keys, est):
        super().__init__()
        self.children = (probe, build)
        self.probe, self.build = probe, build
        self.schema = probe.schema + build.schema
        self.cond = " AND ".join(f"{to_sql(p)} = {to_sql(b)}" for p, b in zip(probe_keys, build_keys))
        self.pk = [compile_expr(k, probe.schema) for k in probe_keys]
        self.bk = [compile_expr(k, build.schema) for k in build_keys]
        self.est = est

    def open(self):
        super().open()
        table = {}
        while (row := self.build.next()) is not None:
            key = tuple(f(row) for f in self.bk)
            if None not in key:  # NULL never equals anything, so it can never match
                table.setdefault(key, []).append(row)
        self.table = table
        self.matches, self.m, self.row = (), 0, None

    def _next(self):
        while self.m >= len(self.matches):
            self.row = self.probe.next()
            if self.row is None:
                return None
            key = tuple(f(self.row) for f in self.pk)
            self.matches = () if None in key else self.table.get(key, ())
            self.m = 0
        self.m += 1
        return self.row + self.matches[self.m - 1]

    def label(self):
        return f"HashJoin {self.cond}  (hash table built on the 2nd input)"

    def algebra(self):
        return f"({self.probe.algebra()} ⋈[{self.cond}] {self.build.algebra()})"


class HashAggregate(Operator):
    """γ: group rows by the GROUP BY values and fold each group with the aggregates."""

    def __init__(self, child, groups, calls, est):
        super().__init__()
        self.children = (child,)
        self.child, self.groups, self.calls = child, groups, calls
        self.schema = [("", to_sql(g)) for g in groups] + [("", to_sql(c)) for c in calls]
        self.gfns = [compile_expr(g, child.schema) for g in groups]
        self.afns = [None if c.arg is None else compile_expr(c.arg, child.schema) for c in calls]
        self.est = est

    def open(self):
        super().open()
        groups = {}
        while (row := self.child.next()) is not None:
            key = tuple(f(row) for f in self.gfns)
            states = groups.get(key)
            if states is None:
                states = groups[key] = [[0, None] for _ in self.calls]  # [count, value]
            for call, f, st in zip(self.calls, self.afns, states):
                if f is None:  # count(*)
                    st[0] += 1
                    continue
                v = f(row)
                if v is None:  # every aggregate except count(*) skips NULL
                    continue
                st[0] += 1
                if call.name in ("sum", "avg"):
                    st[1] = v if st[1] is None else st[1] + v
                elif call.name == "min" and (st[1] is None or sort_key(v) < sort_key(st[1])):
                    st[1] = v
                elif call.name == "max" and (st[1] is None or sort_key(v) > sort_key(st[1])):
                    st[1] = v
        if not groups and not self.groups:  # no GROUP BY: one row even for empty input
            groups[()] = [[0, None] for _ in self.calls]
        self.out = [key + tuple(self._final(c, st) for c, st in zip(self.calls, states))
                    for key, states in groups.items()]
        self.pos = 0

    @staticmethod
    def _final(call, st):
        if call.name == "count":
            return st[0]
        if call.name == "avg":
            return None if st[0] == 0 else st[1] / st[0]
        return st[1]

    def _next(self):
        if self.pos >= len(self.out):
            return None
        self.pos += 1
        return self.out[self.pos - 1]

    def label(self):
        by = ", ".join(map(to_sql, self.groups)) or "(whole input)"
        return f"HashAggregate GROUP BY {by}: " + ", ".join(map(to_sql, self.calls))

    def algebra(self):
        return (f"γ[{', '.join(map(to_sql, self.groups))}; {', '.join(map(to_sql, self.calls))}]"
                f"({self.child.algebra()})")


class Sort(Operator):
    """τ: read everything, sort, then hand rows out one at a time."""

    def __init__(self, child, keys):
        super().__init__()
        self.children = (child,)
        self.child, self.keys = child, keys
        self.schema = child.schema
        self.fns = [(compile_expr(e, self.schema), desc) for e, desc in keys]
        self.est = child.est

    def open(self):
        super().open()
        rows = []
        while (row := self.child.next()) is not None:
            rows.append(row)
        for fn, desc in reversed(self.fns):  # stable sorts, least significant key first
            rows.sort(key=lambda r: sort_key(fn(r)), reverse=desc)
        self.out, self.pos = rows, 0

    def _next(self):
        if self.pos >= len(self.out):
            return None
        self.pos += 1
        return self.out[self.pos - 1]

    def _keys(self):
        return ", ".join(to_sql(e) + (" DESC" if d else "") for e, d in self.keys)

    def label(self):
        return "Sort " + self._keys()

    def algebra(self):
        return f"τ[{self._keys()}]({self.child.algebra()})"


class Project(Operator):
    """π: compute the SELECT list."""

    def __init__(self, child, exprs, names):
        super().__init__()
        self.children = (child,)
        self.child, self.exprs, self.names = child, exprs, names
        self.schema = [("", n) for n in names]
        self.fns = [compile_expr(e, child.schema) for e in exprs]
        self.est = child.est

    def _next(self):
        row = self.child.next()
        return None if row is None else tuple(f(row) for f in self.fns)

    def _items(self):
        return ", ".join(to_sql(e) if to_sql(e) == n else f"{to_sql(e)} AS {n}"
                         for e, n in zip(self.exprs, self.names))

    def label(self):
        return "Project " + self._items()

    def algebra(self):
        return f"π[{self._items()}]({self.child.algebra()})"


class Distinct(Operator):
    """δ: drop duplicate rows (SQL keeps duplicates unless asked)."""

    def __init__(self, child):
        super().__init__()
        self.children = (child,)
        self.child, self.schema, self.est = child, child.schema, child.est

    def open(self):
        super().open()
        self.seen = set()

    def _next(self):
        while (row := self.child.next()) is not None:
            if row not in self.seen:
                self.seen.add(row)
                return row
        return None

    def label(self):
        return "Distinct"

    def algebra(self):
        return f"δ({self.child.algebra()})"


class Limit(Operator):
    """Stop pulling after n rows; pipelined children below never produce the rest."""

    def __init__(self, child, n):
        super().__init__()
        self.children = (child,)
        self.child, self.n, self.schema = child, n, child.schema
        self.est = min(n, child.est)

    def _next(self):
        return None if self.actual >= self.n else self.child.next()

    def label(self):
        return f"Limit {self.n}"

    def algebra(self):
        return f"limit[{self.n}]({self.child.algebra()})"


# ================================================================== 7. planner

DEFAULT_RANGE = 1 / 3  # selectivity guess for <, >, etc. without a histogram (System R used 1/3)
DEFAULT_OTHER = 1 / 3
RANDOM_READ_COST = 4  # teaching constant: fetching one row through an index ~ 4 sequential reads


class Planner:
    """Turns a parsed SELECT into a tree of operators.

    optimize=False gives the canonical translation that defines what the query
    means: FROM is a Cartesian product, ON and WHERE are selections on top of it,
    then aggregation, HAVING, ORDER BY, the SELECT list, DISTINCT and LIMIT.

    optimize=True rewrites that into an equivalent but cheaper tree: predicates
    pushed down to the tables they mention, an index when it is cheaper than a
    full scan, hash joins for equality conditions, and a join order that avoids
    Cartesian products and starts from the smallest estimated input.
    """

    def __init__(self, db, query: Select):
        self.db, self.q = db, query
        self.alias_table = {}
        for ref in query.tables:
            if ref.alias in self.alias_table:
                raise SQLError(f"duplicate table name or alias: {ref.alias}")
            self.alias_table[ref.alias] = db.table(ref.name)
        self.notes: list[str] = []

    # ---- name resolution and statistics
    def owner(self, col):
        """Which FROM entry does this column come from?"""
        if col.table is not None:
            if col.table not in self.alias_table:
                raise SQLError(f"no such table or alias: {col.table}")
            self.alias_table[col.table].column_index(col.name)
            return col.table
        owners = [a for a, t in self.alias_table.items() if col.name in t.columns]
        if not owners:
            raise SQLError(f"no such column: {col.name}")
        if len(owners) > 1:
            raise SQLError(f"ambiguous column name: {col.name}")
        return owners[0]

    def aliases_in(self, e):
        return {self.owner(c) for c in columns_in(e)}

    def column_stats(self, col):
        return self.alias_table[self.owner(col)].stats(col.name)

    def selectivity(self, p):
        """Estimated fraction of rows for which p is True, assuming uniform, independent columns."""
        if isinstance(p, Binary) and p.op == "AND":
            return self.selectivity(p.left) * self.selectivity(p.right)
        if isinstance(p, Binary) and p.op == "OR":
            a, b = self.selectivity(p.left), self.selectivity(p.right)
            return a + b - a * b
        if isinstance(p, Unary) and p.op == "NOT":
            return 1 - self.selectivity(p.operand)
        if isinstance(p, IsNull) and isinstance(p.operand, Column):
            nulls = self.column_stats(p.operand)[1]
            return 1 - nulls if p.negated else nulls
        if isinstance(p, Binary) and p.op in COMPARISONS:
            left, right = p.left, p.right
            if isinstance(left, Column) and isinstance(right, Column):
                if p.op == "=":
                    return 1 / max(self.column_stats(left)[0], self.column_stats(right)[0])
                return DEFAULT_RANGE
            col = left if isinstance(left, Column) else right if isinstance(right, Column) else None
            if col is not None:
                ndv, nulls = self.column_stats(col)
                if p.op == "=":
                    return (1 - nulls) / ndv
                if p.op == "<>":
                    return (1 - nulls) * (1 - 1 / ndv)
                return DEFAULT_RANGE
        return DEFAULT_OTHER

    def scan(self, ref):
        return SeqScan(self.alias_table[ref.alias], ref.alias)

    # ---- entry point
    def build(self, optimize=True):
        q = self.q
        for e in [q.where, *q.joins_on]:
            if calls_in(e):
                raise SQLError("aggregate functions are not allowed in WHERE or ON")
            self.aliases_in(e)  # report unknown columns before planning
        node = self.join_optimized() if optimize else self.join_canonical()
        return self.finish(node)

    def join_canonical(self):
        q = self.q
        node = self.scan(q.tables[0])
        for ref, on in zip(q.tables[1:], q.joins_on[1:]):
            right = self.scan(ref)
            node = NestedLoopJoin(node, right, node.est * right.est)
            if on is not None:
                node = Filter(node, on, node.est * self.selectivity(on))
        if q.where is not None:
            node = Filter(node, q.where, node.est * self.selectivity(q.where))
        return node

    def join_optimized(self):
        q = self.q
        conjuncts = [c for e in [q.where, *q.joins_on] if e is not None for c in conjuncts_of(e)]
        # 1. Predicate pushdown: a condition that mentions one table goes right above its scan.
        local = {ref.alias: [] for ref in q.tables}
        cross = []
        for c in conjuncts:
            aliases = self.aliases_in(c)
            if len(aliases) == 1:
                alias = next(iter(aliases))
                local[alias].append(c)
                if len(q.tables) > 1:
                    self.notes.append(f"下推：{to_sql(c)} 只用到 {alias}，移到 {alias} 的掃描正上方")
            else:
                cross.append(c)
        # 2. Access path for each table: full scan or index.
        access = {ref.alias: self.access_path(ref, local[ref.alias]) for ref in q.tables}
        # 3. Join order: greedy, smallest estimate first, never a Cartesian product if avoidable.
        remaining = list(q.tables)
        first = min(remaining, key=lambda r: access[r.alias].est)
        remaining.remove(first)
        node, joined = access[first.alias], {first.alias}
        if remaining:
            self.notes.append(f"連接順序：先取估計列數最小的 {first.alias}（約 {node.est:,.0f} 列）")
        while remaining:
            connected = [r for r in remaining
                         if any(self.equi_keys(c, joined, r.alias) for c in cross)]
            nxt = min(connected or remaining, key=lambda r: access[r.alias].est)
            remaining.remove(nxt)
            right = access[nxt.alias]
            now = joined | {nxt.alias}
            usable = [c for c in cross if self.aliases_in(c) <= now]
            cross = [c for c in cross if c not in usable]
            equi = [(c, self.equi_keys(c, joined, nxt.alias)) for c in usable]
            rest = [c for c, keys in equi if keys is None]
            equi = [(c, keys) for c, keys in equi if keys is not None]
            if equi:
                sel = math.prod(self.selectivity(c) for c, _ in equi)
                est = node.est * right.est * sel
                left_keys = [k[0] for _, k in equi]
                right_keys = [k[1] for _, k in equi]
                if right.est <= node.est:
                    node = HashJoin(node, right, left_keys, right_keys, est)
                    build = nxt.alias
                else:
                    node = HashJoin(right, node, right_keys, left_keys, est)
                    build = "+".join(sorted(joined))
                self.notes.append(
                    f"接上 {nxt.alias}：有等值條件 {', '.join(to_sql(c) for c, _ in equi)}，"
                    f"用 HashJoin，雜湊表建在估計較小的 {build} 一側")
            else:
                node = NestedLoopJoin(node, right, node.est * right.est)
                self.notes.append(f"接上 {nxt.alias}：找不到連接條件，只能做笛卡兒積")
            if rest:
                p = and_all(rest)
                node = Filter(node, p, node.est * self.selectivity(p))
            joined = now
        if cross:  # conditions that mention no table at all, e.g. WHERE 1 = 1
            p = and_all(cross)
            node = Filter(node, p, node.est * self.selectivity(p))
        return node

    def equi_keys(self, c, left_aliases, right_alias):
        """If c is `x = y` with x from the joined side and y from right_alias, return (x, y)."""
        if isinstance(c, Binary) and c.op == "=":
            a, b = self.aliases_in(c.left), self.aliases_in(c.right)
            if a and b:
                if a <= left_aliases and b == {right_alias}:
                    return c.left, c.right
                if b <= left_aliases and a == {right_alias}:
                    return c.right, c.left
        return None

    def index_match(self, p, table):
        """Does p look like `indexed_column op constant`? Return (column, op, value)."""
        if not (isinstance(p, Binary) and p.op in ("=", "<", "<=", ">", ">=")):
            return None
        left, right, op = p.left, p.right, p.op
        if isinstance(left, Literal) and isinstance(right, Column):
            left, right, op = right, left, _FLIP[op]
        if (isinstance(left, Column) and isinstance(right, Literal) and right.value is not None
                and left.name in table.indexes):
            is_text = table.types[table.column_index(left.name)] is str
            if is_text == isinstance(right.value, str):
                return left.name, op, right.value
        return None

    def access_path(self, ref, preds):
        table = self.alias_table[ref.alias]
        n = len(table.rows)
        best = None
        for p in preds:
            m = self.index_match(p, table)
            if m is None:
                continue
            est = n * self.selectivity(p)
            cost = math.log2(n + 1) + est * RANDOM_READ_COST
            index = table.indexes[m[0]]
            if cost >= n:
                self.notes.append(
                    f"存取路徑：{ref.alias} 有索引 {index.name}，但估計取回 {est:,.0f} 列，"
                    f"成本 {cost:,.0f} ≥ 全表掃描 {n:,}，不用索引")
            elif best is None or cost < best[0]:
                best = (cost, p, m, est)
        if best is not None:
            cost, p, (col, op, value), est = best
            node = IndexScan(table, ref.alias, table.indexes[col], op, value, est)
            preds = [x for x in preds if x is not p]
            self.notes.append(
                f"存取路徑：{ref.alias} 用索引 {table.indexes[col].name}（估計 {est:,.0f} 列，"
                f"成本 {cost:,.0f} < 全表掃描 {n:,}）")
        else:
            node = self.scan(ref)
        if preds:
            p = and_all(preds)
            node = Filter(node, p, node.est * self.selectivity(p))
        return node

    # ---- everything above the joins
    def expand_items(self):
        out = []  # (expr, user alias or None, output name)
        for item in self.q.items:
            if item.expr is not None:
                out.append((item.expr, item.alias, item.alias or to_sql(item.expr)))
                continue
            refs = [r for r in self.q.tables if item.star_table in (None, r.alias)]
            if not refs:
                raise SQLError(f"no such table: {item.star_table}")
            for ref in refs:
                for col in self.alias_table[ref.alias].columns:
                    out.append((Column(ref.alias, col), None, col))
        return out

    def finish(self, node):
        q = self.q
        items = self.expand_items()
        order = []
        for e, desc in q.order_by:  # ORDER BY may name an output alias or a position
            if isinstance(e, Literal) and isinstance(e.value, int):
                if not 1 <= e.value <= len(items):
                    raise SQLError(f"ORDER BY position {e.value} is out of range")
                e = items[e.value - 1][0]
            elif isinstance(e, Column) and e.table is None:
                e = next((x for x, alias, _ in items if alias == e.name), e)
            order.append((e, desc))
        for e in [x for x, _, _ in items] + [e for e, _ in order] + list(q.group_by) + [q.having]:
            self.aliases_in(e)
        if any(calls_in(g) for g in q.group_by):
            raise SQLError("aggregate functions are not allowed in GROUP BY")

        aggregate = bool(q.group_by) or q.having is not None or any(calls_in(x) for x, _, _ in items)
        if aggregate:
            calls = []
            for e in [x for x, _, _ in items] + [q.having] + [e for e, _ in order]:
                for c in calls_in(e):
                    if c not in calls:
                        calls.append(c)
            node = HashAggregate(node, q.group_by, calls, self.group_estimate(node.est))

            def rewrite(e):
                return self.after_aggregate(e, q.group_by, calls)
            items = [(rewrite(x), a, n) for x, a, n in items]
            order = [(rewrite(e), d) for e, d in order]
            if q.having is not None:
                node = Filter(node, rewrite(q.having), node.est * DEFAULT_OTHER)
        if order:
            node = Sort(node, order)
        node = Project(node, [x for x, _, _ in items], [n for _, _, n in items])
        if q.distinct:
            node = Distinct(node)
        if q.limit is not None:
            node = Limit(node, q.limit)
        return node

    def group_estimate(self, rows):
        if not self.q.group_by:
            return 1
        guess = math.prod(self.column_stats(g)[0] if isinstance(g, Column) else rows
                          for g in self.q.group_by)
        return max(1, min(guess, rows))

    def after_aggregate(self, e, groups, calls):
        """Rewrite an expression to read the aggregate's output row instead of input rows."""
        for i, g in enumerate(groups):
            if e == g or (isinstance(e, Column) and isinstance(g, Column) and e.name == g.name
                          and self.owner(e) == self.owner(g)):
                return Slot(i, to_sql(g))
        if isinstance(e, Call):
            return Slot(len(groups) + calls.index(e), to_sql(e))
        if isinstance(e, Column):
            raise SQLError(f"column {to_sql(e)} must appear in GROUP BY or be used in an aggregate")
        if isinstance(e, Binary):
            return Binary(e.op, self.after_aggregate(e.left, groups, calls),
                          self.after_aggregate(e.right, groups, calls))
        if isinstance(e, Unary):
            return Unary(e.op, self.after_aggregate(e.operand, groups, calls))
        if isinstance(e, IsNull):
            return IsNull(self.after_aggregate(e.operand, groups, calls), e.negated)
        return e


# ================================================================= 8. database

@dataclass
class Result:
    columns: list
    rows: list
    message: str = ""
    plan: Operator | None = None


class Database:
    def __init__(self):
        self.tables: dict[str, Table] = {}

    def table(self, name):
        if name not in self.tables:
            raise SQLError(f"no such table: {name}")
        return self.tables[name]

    def execute(self, sql, optimize=True):
        """Run every statement in sql and return the result of the last one."""
        result = None
        for stmt in Parser(sql).script():
            result = self.run(stmt, optimize)
        return result

    def plan(self, sql_or_select, optimize=True, notes=None):
        q = sql_or_select
        if isinstance(q, str):
            stmts = Parser(q).script()
            if len(stmts) != 1 or not isinstance(stmts[0], Select):
                raise SQLError("expected exactly one SELECT")
            q = stmts[0]
        planner = Planner(self, q)
        root = planner.build(optimize)
        if notes is not None:
            notes.extend(planner.notes)
        return root

    def run(self, stmt, optimize=True):
        if isinstance(stmt, CreateTable):
            if stmt.name in self.tables:
                raise SQLError(f"table {stmt.name} already exists")
            self.tables[stmt.name] = Table(stmt.name, stmt.columns)
            return Result([], [], f"created table {stmt.name}")
        if isinstance(stmt, CreateIndex):
            table = self.table(stmt.table)
            table.indexes[stmt.column] = Index(stmt.name, table, stmt.column)
            return Result([], [], f"created index {stmt.name}")
        if isinstance(stmt, Insert):
            table = self.table(stmt.table)
            cols = stmt.columns or table.columns
            positions = [table.column_index(c) for c in cols]
            for exprs in stmt.rows:
                if len(exprs) != len(cols):
                    raise SQLError(f"{len(cols)} columns but {len(exprs)} values")
                values = [None] * len(table.columns)
                for pos, e in zip(positions, exprs):
                    values[pos] = compile_expr(e, [])(())
                table.insert(values)
            return Result([], [], f"inserted {len(stmt.rows)} row(s)")
        if isinstance(stmt, Explain):
            root = self.plan(stmt.query, optimize)
            if stmt.analyze:
                for _ in root:
                    pass
            return Result(["plan"], [(line,) for line in explain(root, stmt.analyze)], plan=root)
        if isinstance(stmt, Select):
            root = self.plan(stmt, optimize)
            return Result([n for _, n in root.schema], list(root), plan=root)
        raise TypeError(stmt)


# ================================================================== 9. display

def render_tree(root) -> list[str]:
    """root is (label, [children]); draw it with box characters."""
    lines = []

    def visit(node, prefix, last, top):
        label, kids = node
        if top:
            lines.append(label)
            child_prefix = ""
        else:
            lines.append(prefix + ("└─ " if last else "├─ ") + label)
            child_prefix = prefix + ("   " if last else "│  ")
        for k, kid in enumerate(kids):
            visit(kid, child_prefix, k == len(kids) - 1, False)

    visit(root, "", True, True)
    return lines


def explain(op, analyze=False) -> list[str]:
    def node(o):
        rows = f"est {o.est:,.0f}" + (f", actual {o.actual:,}" if analyze else "")
        return (f"{o.label()}  [{rows}]", [node(c) for c in o.children])
    return render_tree(node(op))


def rows_moved(op) -> int:
    """Total rows produced by every operator: a rough measure of work done."""
    return op.actual + sum(rows_moved(c) for c in op.children)


def ast_tree(q: Select):
    def expr(e):
        if isinstance(e, Literal):
            return (f"Literal {to_sql(e)}", [])
        if isinstance(e, Column):
            return (f"Column {to_sql(e)}", [])
        if isinstance(e, Binary):
            return (f"Binary {e.op}", [expr(e.left), expr(e.right)])
        if isinstance(e, Unary):
            return (f"Unary {e.op}", [expr(e.operand)])
        if isinstance(e, IsNull):
            return ("IS NOT NULL" if e.negated else "IS NULL", [expr(e.operand)])
        if isinstance(e, Call):
            return (f"Call {e.name}", [("*", [])] if e.arg is None else [expr(e.arg)])
        raise TypeError(e)

    items = []
    for it in q.items:
        if it.expr is None:
            items.append((f"{it.star_table}.*" if it.star_table else "*", []))
        else:
            label = f"AS {it.alias}" if it.alias else "item"
            items.append((label, [expr(it.expr)]))
    from_kids = []
    for ref, on in zip(q.tables, q.joins_on):
        label = "Table " + (ref.name if ref.alias == ref.name else f"{ref.name} AS {ref.alias}")
        from_kids.append((label, [] if on is None else [("ON", [expr(on)])]))
    kids = [("SELECT" + (" DISTINCT" if q.distinct else ""), items), ("FROM", from_kids)]
    if q.where is not None:
        kids.append(("WHERE", [expr(q.where)]))
    if q.group_by:
        kids.append(("GROUP BY", [expr(g) for g in q.group_by]))
    if q.having is not None:
        kids.append(("HAVING", [expr(q.having)]))
    if q.order_by:
        kids.append(("ORDER BY", [(("DESC" if d else "ASC"), [expr(e)]) for e, d in q.order_by]))
    if q.limit is not None:
        kids.append((f"LIMIT {q.limit}", []))
    return render_tree(("Select", kids))


def format_value(v):
    if v is None:
        return "NULL"
    if isinstance(v, float):
        return f"{v:.2f}"
    return str(v)


def format_table(columns, rows, max_rows=20) -> list[str]:
    cells = [[format_value(v) for v in r] for r in rows[:max_rows]]
    widths = [max([len(c)] + [len(r[i]) for r in cells]) for i, c in enumerate(columns)]
    line = lambda vals: " | ".join(v.ljust(w) for v, w in zip(vals, widths))  # noqa: E731
    out = [line(columns), "-+-".join("-" * w for w in widths)] + [line(r) for r in cells]
    if len(rows) > max_rows:
        out.append(f"... ({len(rows) - max_rows} more rows)")
    out.append(f"({len(rows)} row{'s' if len(rows) != 1 else ''})")
    return out


# ============================================================ 10. sample data

CITIES = [("Taipei", "North"), ("New Taipei", "North"), ("Hsinchu", "North"),
          ("Taichung", "Central"), ("Tainan", "South"), ("Kaohsiung", "South")]
STATUSES = ["paid"] * 5 + ["shipped"] * 2 + ["cancelled", "refunded"]


class _Lcg:
    """A fixed pseudo-random generator, so the sample data never changes between Python versions."""

    def __init__(self, seed):
        self.state = seed

    def below(self, n):
        self.state = (self.state * 6364136223846793005 + 1442695040888963407) % 2 ** 64
        return (self.state >> 33) % n


def sample_tables(customers=200, orders=3000, seed=2026):
    """{table: ([(column, type)], rows)}. Some cities and amounts are NULL on purpose,
    and every 250th order points at customer 999, who does not exist."""
    rng = _Lcg(seed)
    cust_rows = []
    for i in range(1, customers + 1):
        city = None if i % 23 == 0 else CITIES[rng.below(len(CITIES))][0]
        cust_rows.append((i, f"customer{i:03d}", city))
    order_rows = []
    for i in range(1, orders + 1):
        cust = 999 if i % 250 == 0 else 1 + rng.below(customers)
        amount = None if i % 97 == 0 else 100 + 10 * rng.below(491)
        status = STATUSES[rng.below(len(STATUSES))]
        order_rows.append((i, cust, amount, status, 1 + rng.below(365)))
    return {
        "cities": ([("name", "TEXT"), ("region", "TEXT")], list(CITIES)),
        "customers": ([("id", "INT"), ("name", "TEXT"), ("city", "TEXT")], cust_rows),
        "orders": ([("id", "INT"), ("customer_id", "INT"), ("amount", "INT"),
                    ("status", "TEXT"), ("day", "INT")], order_rows),
    }


SAMPLE_INDEXES = [("customers_id", "customers", "id"), ("orders_customer_id", "orders", "customer_id")]


def sample_database(customers=200, orders=3000) -> Database:
    db = Database()
    for name, (cols, rows) in sample_tables(customers, orders).items():
        table = db.tables[name] = Table(name, cols)
        for r in rows:
            table.insert(r)
    for index_name, table, col in SAMPLE_INDEXES:
        db.tables[table].indexes[col] = Index(index_name, db.tables[table], col)
    return db


# =================================================================== 11. CLI

DEMO_SQL = """SELECT c.city, COUNT(*) AS n, SUM(o.amount) AS total
FROM customers c JOIN orders o ON o.customer_id = c.id
WHERE o.status = 'paid' AND c.city <> 'Tainan'
GROUP BY c.city
HAVING COUNT(*) > 200
ORDER BY total DESC
LIMIT 3"""

COMPARE_CASES = [
    ("兩表連接、篩選、分組（與 demo 相同）", DEMO_SQL),
    ("用索引找一位客戶的訂單",
     "SELECT id, amount, status FROM orders WHERE customer_id = 42 ORDER BY id"),
    ("三表連接，FROM 的順序故意不好（cities 與 orders 之間沒有連接條件）",
     "SELECT ci.region, COUNT(*) AS n FROM cities ci, orders o, customers c\n"
     "WHERE o.customer_id = c.id AND c.city = ci.name AND ci.region = 'South'\n"
     "GROUP BY ci.region"),
    ("LIMIT 讓掃描提早停止（兩種計畫相同）",
     "SELECT id, amount FROM orders WHERE status = 'refunded' LIMIT 5"),
]


def heading(text):
    print(f"\n== {text} ==")


def cmd_demo(sql):
    db = sample_database()
    heading("0. 範例資料")
    for t in db.tables.values():
        cols = ", ".join(f"{c} {ty}" for c, ty in zip(t.columns, t.type_names))
        idx = ", ".join(i.name for i in t.indexes.values()) or "無"
        print(f"{t.name}：{len(t.rows):,} 列（{cols}）；索引：{idx}")
    heading("1. SQL 原文")
    print(sql)
    heading("2. 詞法分析：字元 → 記號（token）")
    tokens = tokenize(sql)
    print(" · ".join(map(str, tokens)))
    print(f"（共 {len(tokens)} 個記號）")
    heading("3. 語法分析：記號 → 語法樹（AST）")
    stmts = Parser(sql).script()
    if len(stmts) != 1 or not isinstance(stmts[0], Select):
        raise SQLError("demo expects exactly one SELECT")
    q = stmts[0]
    print("\n".join(ast_tree(q)))
    heading("4. 依定義翻譯成關聯代數（未最佳化：FROM 是笛卡兒積，WHERE 是上面的選擇）")
    naive = db.plan(q, optimize=False)
    print(naive.algebra())
    print()
    print("\n".join(explain(naive)))
    heading("5. 最佳化器改寫成等價但較便宜的實體計畫")
    notes = []
    plan = db.plan(q, optimize=True, notes=notes)
    print("\n".join(explain(plan)))
    print()
    for note in notes:
        print("- " + note)
    heading("6. 執行：父節點呼叫 next() 向子節點一次要一列（Volcano 迭代器）")
    rows = list(plan)
    print("\n".join(explain(plan, analyze=True)))
    heading("7. 結果")
    print("\n".join(format_table([n for _, n in plan.schema], rows)))


def cmd_compare():
    db = sample_database()
    for title, sql in COMPARE_CASES:
        print("\n" + "=" * 78)
        print(title)
        print(sql)
        results = {}
        for optimize in (False, True):
            plan = db.plan(sql, optimize=optimize)
            start = time.perf_counter()
            rows = list(plan)
            ms = (time.perf_counter() - start) * 1000
            results[optimize] = rows
            heading("最佳化後" if optimize else "未最佳化（依定義翻譯）")
            print("\n".join(explain(plan, analyze=True)))
            print(f"所有運算子合計產出 {rows_moved(plan):,} 列；耗時 {ms:,.1f} ms；結果 {len(rows)} 列")
        same = sorted(results[False], key=repr) == sorted(results[True], key=repr)
        print(f"\n兩個計畫的結果相同：{same}")


def cmd_query(sql, optimize, show_plan):
    db = sample_database()
    result = db.execute(sql, optimize)
    if result.columns:
        print("\n".join(format_table(result.columns, result.rows)))
    if show_plan and result.plan is not None:
        print()
        print("\n".join(explain(result.plan, analyze=True)))


def cmd_run(path, use_sample, optimize):
    db = sample_database() if use_sample else Database()
    with open(path, encoding="utf-8") as f:
        text = f.read()
    for stmt in Parser(text).script():
        result = db.run(stmt, optimize)
        if result.columns:
            print("\n".join(format_table(result.columns, result.rows)))
            print()
        elif result.message:
            print(result.message)


def main(argv):
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except AttributeError:
        pass
    args = [a for a in argv if not a.startswith("--")]
    flags = {a for a in argv if a.startswith("--")}
    optimize = "--no-opt" not in flags
    try:
        if not args or args[0] in ("-h", "help"):
            print(__doc__)
        elif args[0] == "demo":
            cmd_demo(args[1] if len(args) > 1 else DEMO_SQL)
        elif args[0] == "compare":
            cmd_compare()
        elif args[0] == "query" and len(args) > 1:
            cmd_query(args[1], optimize, "--plan" in flags)
        elif args[0] == "run" and len(args) > 1:
            cmd_run(args[1], "--sample" in flags, optimize)
        else:
            print(__doc__)
            return 2
    except SQLError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
