#!/usr/bin/env python3
"""tinyc: a teaching compiler for a small C subset, emitting RV32IM assembly.

Subset: `int` and `int *` values, fixed-size `int` arrays (local and global),
functions with up to 8 parameters, if/else, while, for, return, the usual
arithmetic, comparison and logical operators, `&x`, `*p`, `a[i]`, and two
builtins that map to environment calls: `print_int(x)` and `print_char(c)`.

The pipeline is deliberately explicit so each classic compiler phase can be
read on its own:

    source text --lex--> tokens --parse--> AST --check--> typed AST --gen--> asm

The output follows the RISC-V ILP32 calling convention closely enough to be
assembled by a real `riscv64-unknown-elf-as -march=rv32im -mabi=ilp32` and
run under `rv32sim.py` or a RARS-compatible simulator (ecall numbers 1, 11, 93).
"""
from __future__ import annotations

import re
import sys
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

# --------------------------------------------------------------------------
# 1. Lexer: characters -> tokens
# --------------------------------------------------------------------------

KEYWORDS = {"int", "if", "else", "while", "for", "return"}
PUNCT = ["==", "!=", "<=", ">=", "&&", "||",
         "+", "-", "*", "/", "%", "=", "<", ">", "!", "&",
         "(", ")", "{", "}", "[", "]", ";", ","]


@dataclass
class Token:
    kind: str      # 'num' | 'ident' | 'kw' | 'punct' | 'eof'
    text: str
    value: int = 0
    line: int = 0


class CompileError(Exception):
    pass


def lex(src: str) -> List[Token]:
    toks: List[Token] = []
    i, line = 0, 1
    n = len(src)
    while i < n:
        c = src[i]
        if c == "\n":
            line += 1
            i += 1
        elif c.isspace():
            i += 1
        elif src.startswith("//", i):
            j = src.find("\n", i)
            i = n if j < 0 else j
        elif src.startswith("/*", i):
            j = src.find("*/", i + 2)
            if j < 0:
                raise CompileError(f"line {line}: unterminated comment")
            line += src.count("\n", i, j)
            i = j + 2
        elif c.isdigit():
            m = re.match(r"0[xX][0-9a-fA-F]+|\d+", src[i:])
            text = m.group(0)
            toks.append(Token("num", text, int(text, 0), line))
            i += len(text)
        elif c == "'":
            m = re.match(r"'(\\.|[^\\'])'", src[i:])
            if not m:
                raise CompileError(f"line {line}: bad character literal")
            body = m.group(1)
            esc = {"\\n": 10, "\\t": 9, "\\0": 0, "\\\\": 92, "\\'": 39}
            value = esc[body] if body in esc else ord(body)
            toks.append(Token("num", m.group(0), value, line))
            i += len(m.group(0))
        elif c.isalpha() or c == "_":
            m = re.match(r"[A-Za-z_][A-Za-z0-9_]*", src[i:])
            text = m.group(0)
            toks.append(Token("kw" if text in KEYWORDS else "ident", text, 0, line))
            i += len(text)
        else:
            for p in PUNCT:
                if src.startswith(p, i):
                    toks.append(Token("punct", p, 0, line))
                    i += len(p)
                    break
            else:
                raise CompileError(f"line {line}: unexpected character {c!r}")
    toks.append(Token("eof", "<eof>", 0, line))
    return toks


# --------------------------------------------------------------------------
# 2. AST: the parser produces these nodes
# --------------------------------------------------------------------------

@dataclass
class Node:
    kind: str                   # see parser for the kinds
    line: int = 0
    # Expression payload
    value: int = 0              # 'num'
    name: str = ""              # 'var', 'call', 'func'
    op: str = ""                # 'binary', 'unary'
    lhs: Optional["Node"] = None
    rhs: Optional["Node"] = None
    args: List["Node"] = field(default_factory=list)
    # Statement payload
    cond: Optional["Node"] = None
    then: Optional["Node"] = None
    els: Optional["Node"] = None
    init: Optional["Node"] = None
    step: Optional["Node"] = None
    body: List["Node"] = field(default_factory=list)
    # Declarations
    ty: str = "int"             # 'int' | 'ptr' | 'arr'
    size: int = 0               # array length
    params: List[Tuple[str, str]] = field(default_factory=list)   # (name, ty)
    # Filled by the checker / code generator
    slot: Optional["Symbol"] = None
    rtype: str = "int"          # result type of an expression: 'int' | 'ptr'


@dataclass
class Symbol:
    name: str
    ty: str          # 'int' | 'ptr' | 'arr'
    size: int = 0
    offset: int = 0  # locals: positive distance below fp
    is_global: bool = False


# --------------------------------------------------------------------------
# 3. Parser: tokens -> AST (recursive descent, one function per grammar rule)
# --------------------------------------------------------------------------

class Parser:
    def __init__(self, toks: List[Token]):
        self.toks = toks
        self.pos = 0

    # -- helpers ----------------------------------------------------------
    def peek(self, k: int = 0) -> Token:
        return self.toks[min(self.pos + k, len(self.toks) - 1)]

    def at(self, text: str) -> bool:
        t = self.peek()
        return t.kind in ("punct", "kw") and t.text == text

    def accept(self, text: str) -> bool:
        if self.at(text):
            self.pos += 1
            return True
        return False

    def expect(self, text: str) -> Token:
        t = self.peek()
        if not self.accept(text):
            raise CompileError(f"line {t.line}: expected {text!r}, got {t.text!r}")
        return t

    def ident(self) -> Token:
        t = self.peek()
        if t.kind != "ident":
            raise CompileError(f"line {t.line}: expected identifier, got {t.text!r}")
        self.pos += 1
        return t

    # -- declarations -----------------------------------------------------
    def program(self) -> List[Node]:
        items = []
        while self.peek().kind != "eof":
            items.append(self.toplevel())
        return items

    def toplevel(self) -> Node:
        line = self.expect("int").line
        ty = "ptr" if self.accept("*") else "int"
        name = self.ident().text
        if self.at("("):
            return self.function(name, ty, line)
        node = Node("gvar", line, name=name, ty=ty)
        if self.accept("["):
            node.ty, node.size = "arr", self.number()
            self.expect("]")
        self.expect(";")
        return node

    def number(self) -> int:
        t = self.peek()
        if t.kind != "num":
            raise CompileError(f"line {t.line}: expected constant")
        self.pos += 1
        return t.value

    def function(self, name: str, ty: str, line: int) -> Node:
        self.expect("(")
        params: List[Tuple[str, str]] = []
        if not self.at(")"):
            while True:
                self.expect("int")
                pty = "ptr" if self.accept("*") else "int"
                params.append((self.ident().text, pty))
                if not self.accept(","):
                    break
        self.expect(")")
        if len(params) > 8:
            raise CompileError(f"line {line}: at most 8 parameters are supported")
        body = self.block()
        return Node("func", line, name=name, ty=ty, params=params, body=body)

    # -- statements -------------------------------------------------------
    def block(self) -> List[Node]:
        self.expect("{")
        stmts = []
        while not self.at("}"):
            stmts.append(self.statement())
        self.expect("}")
        return stmts

    def declaration(self) -> Node:
        line = self.expect("int").line
        ty = "ptr" if self.accept("*") else "int"
        name = self.ident().text
        node = Node("decl", line, name=name, ty=ty)
        if self.accept("["):
            node.ty, node.size = "arr", self.number()
            self.expect("]")
        if self.accept("="):
            if node.ty == "arr":
                raise CompileError(f"line {line}: array initializers are not supported")
            node.init = self.expression()
        return node

    def statement(self) -> Node:
        t = self.peek()
        if self.at("int"):
            node = self.declaration()
            self.expect(";")
            return node
        if self.accept("{"):
            self.pos -= 1
            return Node("block", t.line, body=self.block())
        if self.accept("if"):
            self.expect("(")
            cond = self.expression()
            self.expect(")")
            then = self.statement()
            els = self.statement() if self.accept("else") else None
            return Node("if", t.line, cond=cond, then=then, els=els)
        if self.accept("while"):
            self.expect("(")
            cond = self.expression()
            self.expect(")")
            return Node("while", t.line, cond=cond, then=self.statement())
        if self.accept("for"):
            self.expect("(")
            init = None
            if self.at("int"):
                init = self.declaration()
            elif not self.at(";"):
                init = Node("expr", t.line, lhs=self.expression())
            self.expect(";")
            cond = None if self.at(";") else self.expression()
            self.expect(";")
            step = None if self.at(")") else self.expression()
            self.expect(")")
            return Node("for", t.line, init=init, cond=cond, step=step, then=self.statement())
        if self.accept("return"):
            e = None if self.at(";") else self.expression()
            self.expect(";")
            return Node("return", t.line, lhs=e)
        if self.accept(";"):
            return Node("block", t.line)
        e = self.expression()
        self.expect(";")
        return Node("expr", t.line, lhs=e)

    # -- expressions, lowest precedence first -------------------------------
    def expression(self) -> Node:
        return self.assign()

    def assign(self) -> Node:
        lhs = self.logor()
        if self.at("="):
            line = self.expect("=").line
            if lhs.kind not in ("var", "deref"):
                raise CompileError(f"line {line}: cannot assign to this expression")
            return Node("assign", line, lhs=lhs, rhs=self.assign())
        return lhs

    def binary_level(self, ops: Tuple[str, ...], below) -> Node:
        lhs = below()
        while True:
            t = self.peek()
            if t.kind == "punct" and t.text in ops:
                self.pos += 1
                lhs = Node("binary", t.line, op=t.text, lhs=lhs, rhs=below())
            else:
                return lhs

    def logor(self) -> Node:
        return self.binary_level(("||",), self.logand)

    def logand(self) -> Node:
        return self.binary_level(("&&",), self.equality)

    def equality(self) -> Node:
        return self.binary_level(("==", "!="), self.relational)

    def relational(self) -> Node:
        return self.binary_level(("<", "<=", ">", ">="), self.additive)

    def additive(self) -> Node:
        return self.binary_level(("+", "-"), self.multiplicative)

    def multiplicative(self) -> Node:
        return self.binary_level(("*", "/", "%"), self.unary)

    def unary(self) -> Node:
        t = self.peek()
        if self.accept("-"):
            return Node("unary", t.line, op="-", lhs=self.unary())
        if self.accept("!"):
            return Node("unary", t.line, op="!", lhs=self.unary())
        if self.accept("*"):
            return Node("deref", t.line, lhs=self.unary())
        if self.accept("&"):
            operand = self.unary()
            if operand.kind not in ("var", "deref"):
                raise CompileError(f"line {t.line}: '&' needs a variable or '*' expression")
            return Node("addr", t.line, lhs=operand)
        return self.postfix()

    def postfix(self) -> Node:
        node = self.primary()
        while True:
            t = self.peek()
            if self.accept("["):
                index = self.expression()
                self.expect("]")
                # a[i] is defined as *(a + i); the checker scales the index.
                node = Node("deref", t.line, lhs=Node("binary", t.line, op="+", lhs=node, rhs=index))
            elif self.accept("("):
                if node.kind != "var":
                    raise CompileError(f"line {t.line}: only direct calls are supported")
                args = []
                if not self.at(")"):
                    while True:
                        args.append(self.expression())
                        if not self.accept(","):
                            break
                self.expect(")")
                node = Node("call", t.line, name=node.name, args=args)
            else:
                return node

    def primary(self) -> Node:
        t = self.peek()
        if t.kind == "num":
            self.pos += 1
            return Node("num", t.line, value=t.value)
        if t.kind == "ident":
            self.pos += 1
            return Node("var", t.line, name=t.text)
        if self.accept("("):
            e = self.expression()
            self.expect(")")
            return e
        raise CompileError(f"line {t.line}: unexpected token {t.text!r}")


# --------------------------------------------------------------------------
# 4. Semantic analysis + 5. Code generation (merged: one pass over the AST)
# --------------------------------------------------------------------------

BUILTINS = {"print_int": 1, "print_char": 11}   # name -> ecall number (RARS convention)
WORD = 4


class CodeGen:
    def __init__(self) -> None:
        self.out: List[str] = []
        self.globals: Dict[str, Symbol] = {}
        self.funcs: Dict[str, Node] = {}
        self.scopes: List[Dict[str, Symbol]] = []
        self.frame_size = 0
        self.label_id = 0
        self.ret_label = ""
        self.stack_depth = 0        # words pushed for temporaries, for diagnostics

    # -- helpers ----------------------------------------------------------
    def emit(self, s: str) -> None:
        self.out.append("    " + s if not s.endswith(":") else s)

    def label(self) -> str:
        self.label_id += 1
        return f".L{self.label_id}"

    def lookup(self, name: str, line: int) -> Symbol:
        for scope in reversed(self.scopes):
            if name in scope:
                return scope[name]
        if name in self.globals:
            return self.globals[name]
        raise CompileError(f"line {line}: undeclared identifier {name!r}")

    def declare_local(self, name: str, ty: str, size: int, line: int) -> Symbol:
        if name in self.scopes[-1]:
            raise CompileError(f"line {line}: {name!r} already declared in this scope")
        nbytes = WORD * (size if ty == "arr" else 1)
        self.frame_size += nbytes
        sym = Symbol(name, ty, size, offset=self.frame_size)
        self.scopes[-1][name] = sym
        return sym

    def push(self, reg: str = "a0") -> None:
        self.emit("addi sp, sp, -4")
        self.emit(f"sw {reg}, 0(sp)")
        self.stack_depth += 1

    def pop(self, reg: str) -> None:
        self.emit(f"lw {reg}, 0(sp)")
        self.emit("addi sp, sp, 4")
        self.stack_depth -= 1

    # -- program ----------------------------------------------------------
    def program(self, items: List[Node]) -> str:
        for it in items:
            if it.kind == "gvar":
                if it.name in self.globals:
                    raise CompileError(f"line {it.line}: duplicate global {it.name!r}")
                self.globals[it.name] = Symbol(it.name, it.ty, it.size, is_global=True)
            else:
                if it.name in self.funcs or it.name in BUILTINS:
                    raise CompileError(f"line {it.line}: duplicate function {it.name!r}")
                self.funcs[it.name] = it
        if "main" not in self.funcs:
            raise CompileError("no 'main' function")

        self.emit(".text")
        self.emit(".globl _start")
        # Program entry: C says execution starts at main; somebody must call it
        # and turn its return value into an exit status. That somebody is crt0.
        self.emit("_start:")
        self.emit("call main")
        self.emit("li a7, 93")          # exit(a0)
        self.emit("ecall")
        for f in self.funcs.values():
            self.function(f)
        if self.globals:
            self.emit(".data")
            for g in self.globals.values():
                self.emit(".align 2")
                self.emit(f"{g.name}:")
                self.emit(f".zero {WORD * (g.size if g.ty == 'arr' else 1)}")
        return "\n".join(self.out) + "\n"

    # -- functions --------------------------------------------------------
    def function(self, f: Node) -> None:
        # Body is generated first into a side buffer so the frame size is known
        # when the prologue is written.
        saved_out, self.out = self.out, []
        self.frame_size = 16        # bytes fp-1..fp-16 hold the saved ra and fp
        self.scopes = [{}]
        self.ret_label = self.label()
        param_syms = [self.declare_local(name, ty, 0, f.line) for name, ty in f.params]
        for i, sym in enumerate(param_syms):
            self.emit(f"sw a{i}, -{sym.offset}(fp)")       # spill incoming args
        for s in f.body:
            self.statement(s)
        self.emit("li a0, 0")                              # fall off the end -> 0
        self.emit(f"{self.ret_label}:")
        body, self.out = self.out, saved_out
        self.scopes = []

        frame = (self.frame_size + 15) // 16 * 16     # keep sp 16-byte aligned (psABI)
        self.emit(f".globl {f.name}")
        self.emit(f"{f.name}:")
        self.emit("addi sp, sp, -16")
        self.emit("sw ra, 12(sp)")
        self.emit("sw fp, 8(sp)")
        self.emit("addi fp, sp, 16")
        if frame > 16:
            self.emit(f"addi sp, sp, -{frame - 16}")
        self.out.extend(body)
        self.emit("addi sp, fp, -16")
        self.emit("lw ra, 12(sp)")
        self.emit("lw fp, 8(sp)")
        self.emit("addi sp, sp, 16")
        self.emit("ret")

    # -- statements -------------------------------------------------------
    def statement(self, s: Node) -> None:
        k = s.kind
        if k == "decl":
            sym = self.declare_local(s.name, s.ty, s.size, s.line)
            if s.init is not None:
                self.expression(s.init)
                self.emit(f"sw a0, -{sym.offset}(fp)")
        elif k == "block":
            self.scopes.append({})
            for st in s.body:
                self.statement(st)
            self.scopes.pop()
        elif k == "expr":
            self.expression(s.lhs)
        elif k == "return":
            if s.lhs is not None:
                self.expression(s.lhs)
            else:
                self.emit("li a0, 0")
            self.emit(f"j {self.ret_label}")
        elif k == "if":
            l_else, l_end = self.label(), self.label()
            self.expression(s.cond)
            self.emit(f"beqz a0, {l_else}")
            self.statement(s.then)
            self.emit(f"j {l_end}")
            self.emit(f"{l_else}:")
            if s.els is not None:
                self.statement(s.els)
            self.emit(f"{l_end}:")
        elif k == "while":
            l_top, l_end = self.label(), self.label()
            self.emit(f"{l_top}:")
            self.expression(s.cond)
            self.emit(f"beqz a0, {l_end}")
            self.statement(s.then)
            self.emit(f"j {l_top}")
            self.emit(f"{l_end}:")
        elif k == "for":
            self.scopes.append({})
            l_top, l_end = self.label(), self.label()
            if s.init is not None:
                self.statement(s.init)
            self.emit(f"{l_top}:")
            if s.cond is not None:
                self.expression(s.cond)
                self.emit(f"beqz a0, {l_end}")
            self.statement(s.then)
            if s.step is not None:
                self.expression(s.step)
            self.emit(f"j {l_top}")
            self.emit(f"{l_end}:")
            self.scopes.pop()
        else:
            raise CompileError(f"line {s.line}: unknown statement {k}")

    # -- expressions: result always ends up in a0 ---------------------------
    def address_of(self, e: Node) -> str:
        """Emit code that leaves the address of lvalue `e` in a0; return its type."""
        if e.kind == "var":
            sym = self.lookup(e.name, e.line)
            if sym.is_global:
                self.emit(f"la a0, {sym.name}")
            else:
                self.emit(f"addi a0, fp, -{sym.offset}")
            return sym.ty
        if e.kind == "deref":
            t = self.expression(e.lhs)
            if t != "ptr":
                raise CompileError(f"line {e.line}: '*' applied to a non-pointer")
            return "int"
        raise CompileError(f"line {e.line}: expression is not an lvalue")

    def expression(self, e: Node) -> str:
        """Emit code for `e`, leaving its value in a0; return 'int' or 'ptr'."""
        k = e.kind
        if k == "num":
            self.emit(f"li a0, {e.value}")
            return "int"
        if k == "var":
            sym = self.lookup(e.name, e.line)
            t = self.address_of(e)
            if t == "arr":                     # arrays decay to a pointer to element 0
                return "ptr"
            self.emit("lw a0, 0(a0)")
            return t
        if k == "deref":
            self.address_of(e)
            self.emit("lw a0, 0(a0)")
            return "int"
        if k == "addr":
            t = self.address_of(e.lhs)
            return "ptr"
        if k == "assign":
            self.expression(e.rhs)
            self.push()
            t = self.address_of(e.lhs)
            if t == "arr":
                raise CompileError(f"line {e.line}: cannot assign to an array")
            self.pop("t0")
            self.emit("sw t0, 0(a0)")
            self.emit("mv a0, t0")
            return t
        if k == "unary":
            self.expression(e.lhs)
            self.emit("neg a0, a0" if e.op == "-" else "seqz a0, a0")
            return "int"
        if k == "call":
            return self.call(e)
        if k == "binary":
            return self.binary(e)
        raise CompileError(f"line {e.line}: unknown expression {k}")

    def call(self, e: Node) -> str:
        if e.name in BUILTINS:
            if len(e.args) != 1:
                raise CompileError(f"line {e.line}: {e.name} takes one argument")
            self.expression(e.args[0])
            self.emit(f"li a7, {BUILTINS[e.name]}")
            self.emit("ecall")
            return "int"
        f = self.funcs.get(e.name)
        if f is None:
            raise CompileError(f"line {e.line}: call to undefined function {e.name!r}")
        if len(e.args) != len(f.params):
            raise CompileError(f"line {e.line}: {e.name} expects {len(f.params)} arguments")
        # Evaluate every argument first (each may itself call), park them on the
        # stack, then move them into the argument registers a0..a7.
        for a in e.args:
            self.expression(a)
            self.push()
        for i in reversed(range(len(e.args))):
            self.pop(f"a{i}")
        self.emit(f"call {e.name}")
        return f.ty

    def binary(self, e: Node) -> str:
        op = e.op
        if op in ("&&", "||"):
            l_end = self.label()
            self.expression(e.lhs)
            self.emit("snez a0, a0")
            self.emit(f"{'beqz' if op == '&&' else 'bnez'} a0, {l_end}")
            self.expression(e.rhs)
            self.emit("snez a0, a0")
            self.emit(f"{l_end}:")
            return "int"

        lt = self.expression(e.lhs)
        self.push()
        rt = self.expression(e.rhs)
        self.emit("mv t1, a0")          # t1 = rhs
        self.pop("t0")                  # t0 = lhs

        if op in ("+", "-"):
            # Pointer arithmetic scales by the element size, like real C.
            if lt == "ptr" and rt == "int":
                self.emit(f"slli t1, t1, 2")
                rtype = "ptr"
            elif lt == "int" and rt == "ptr" and op == "+":
                self.emit(f"slli t0, t0, 2")
                rtype = "ptr"
            elif lt == "ptr" and rt == "ptr" and op == "-":
                self.emit("sub a0, t0, t1")
                self.emit("srai a0, a0, 2")
                return "int"
            elif lt == "int" and rt == "int":
                rtype = "int"
            else:
                raise CompileError(f"line {e.line}: invalid operands for {op}")
            self.emit(f"{'add' if op == '+' else 'sub'} a0, t0, t1")
            return rtype

        if lt == "ptr" or rt == "ptr":
            if op not in ("==", "!=", "<", "<=", ">", ">="):
                raise CompileError(f"line {e.line}: invalid pointer operands for {op}")

        table = {
            "*": ["mul a0, t0, t1"],
            "/": ["div a0, t0, t1"],
            "%": ["rem a0, t0, t1"],
            "==": ["sub a0, t0, t1", "seqz a0, a0"],
            "!=": ["sub a0, t0, t1", "snez a0, a0"],
            "<": ["slt a0, t0, t1"],
            ">": ["slt a0, t1, t0"],
            "<=": ["slt a0, t1, t0", "xori a0, a0, 1"],
            ">=": ["slt a0, t0, t1", "xori a0, a0, 1"],
        }
        for ins in table[op]:
            self.emit(ins)
        return "int"


# --------------------------------------------------------------------------
# Driver
# --------------------------------------------------------------------------

def compile_source(src: str) -> str:
    tokens = lex(src)
    ast = Parser(tokens).program()
    return CodeGen().program(ast)


def main(argv: List[str]) -> int:
    if len(argv) < 2 or argv[1] in ("-h", "--help"):
        print("usage: tinyc.py input.c [-o output.s] [--tokens] [--ast]", file=sys.stderr)
        return 2
    path = argv[1]
    out_path = None
    if "-o" in argv:
        out_path = argv[argv.index("-o") + 1]
    with open(path, encoding="utf-8") as fh:
        src = fh.read()
    try:
        if "--tokens" in argv:
            for t in lex(src):
                print(f"{t.line:4}  {t.kind:6} {t.text}")
            return 0
        if "--ast" in argv:
            dump_ast(Parser(lex(src)).program())
            return 0
        asm = compile_source(src)
    except CompileError as err:
        print(f"{path}: error: {err}", file=sys.stderr)
        return 1
    if out_path:
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write(asm)
    else:
        sys.stdout.write(asm)
    return 0


def dump_ast(nodes: List[Node], depth: int = 0) -> None:
    pad = "  " * depth
    for n in nodes:
        head = n.kind
        if n.name:
            head += f" {n.name}"
        if n.kind == "num":
            head += f" {n.value}"
        if n.op:
            head += f" '{n.op}'"
        if n.kind in ("decl", "gvar", "func"):
            head += f" : {n.ty}" + (f"[{n.size}]" if n.ty == "arr" else "")
        if n.params:
            head += " (" + ", ".join(f"{t} {p}" for p, t in n.params) + ")"
        print(pad + head)
        children = [c for c in (n.lhs, n.rhs, n.cond, n.init, n.step, n.then, n.els) if c]
        dump_ast(children + n.args + n.body, depth + 1)


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except BrokenPipeError:          # e.g. `tinyc.py x.c --ast | head`
        sys.exit(0)
