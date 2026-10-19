#!/usr/bin/env python3
"""tinyc: a teaching compiler for a small C subset with two backends.

Subset: `int` and `int *` values, fixed-size `int` arrays (local and global),
functions with up to 8 parameters, if/else, while, for, return, the usual
arithmetic, comparison and logical operators, `&x`, `*p`, `a[i]`, and two
builtins that map to environment calls: `print_int(x)` and `print_char(c)`.

The pipeline is deliberately explicit so each classic compiler phase can be
read on its own:

    source text --lex--> tokens --parse--> AST --check--> typed AST --gen--> asm

The front end (lexer, parser, type rules, frame layout) is shared. Code
generation talks to a small `Machine` interface with two implementations:

    --target rv32     RV32IM assembly, ILP32 psABI subset; run on rv32sim.py or
                      assemble with `clang --target=riscv32 -march=rv32im`
    --target x86_64   x86-64 System V assembly (AT&T syntax); assemble and run
                      natively with the host `cc` plus runtime.c

Reading the two `Machine` classes side by side is the point of the lab: the
same AST becomes different instructions, register names, pointer sizes and
startup conventions, while everything above `Machine` stays untouched.
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
# 4. Target descriptions: everything the code generator must know about a CPU
# --------------------------------------------------------------------------
#
# Each Machine exposes the same tiny vocabulary. The accumulator ("acc") holds
# the value of the expression being evaluated; "tmp" holds a second operand.
# Values are 'int' (32-bit) or 'ptr' (the target's pointer size).

class Machine:
    name = ""
    int_size = 4
    ptr_size = 4
    push_size = 4          # bytes one temporary push consumes
    max_args = 0           # register-passed arguments supported
    frame_reserved = 0     # bytes below the frame pointer already used by the prologue

    def size_of(self, ty: str) -> int:
        return self.ptr_size if ty == "ptr" else self.int_size

    def slot_size(self, ty: str) -> int:       # every scalar gets a whole slot
        return max(self.int_size, self.ptr_size) if ty != "arr" else 0

    # emit-level hooks, each returns a list of assembly lines
    def header(self) -> List[str]: ...
    def footer(self, globals_: List["Symbol"]) -> List[str]: ...
    def prologue(self, name: str, frame: int) -> List[str]: ...
    def epilogue(self) -> List[str]: ...
    def spill_param(self, i: int, offset: int, ty: str) -> List[str]: ...
    def load_imm(self, v: int) -> List[str]: ...
    def local_addr(self, offset: int) -> List[str]: ...
    def global_addr(self, name: str) -> List[str]: ...
    def load(self, ty: str) -> List[str]: ...                # acc = *(acc)
    def store(self, ty: str) -> List[str]: ...               # *(acc) = tmp; acc = tmp
    def push(self) -> List[str]: ...
    def pop_tmp(self) -> List[str]: ...                      # tmp = pop()
    def pop_arg(self, i: int) -> List[str]: ...              # arg register i = pop()
    def acc_to_tmp(self) -> List[str]: ...
    def swap_pop(self) -> List[str]: ...                     # tmp = acc; acc = pop()  (lhs, rhs)
    def neg(self) -> List[str]: ...
    def lognot(self) -> List[str]: ...
    def normalize_bool(self) -> List[str]: ...               # acc = acc != 0
    def binop(self, op: str, ty: str) -> List[str]: ...      # acc = acc OP tmp
    def scale_tmp(self, shift: int) -> List[str]: ...        # tmp <<= shift
    def scale_acc(self, shift: int) -> List[str]: ...        # acc <<= shift
    def unscale_acc(self, shift: int) -> List[str]: ...      # acc >>= shift (arithmetic)
    def branch_zero(self, label: str) -> List[str]: ...
    def branch_nonzero(self, label: str) -> List[str]: ...
    def jump(self, label: str) -> List[str]: ...
    def adjust_sp(self, delta: int) -> List[str]: ...        # sp += delta
    def call(self, name: str) -> List[str]: ...
    def builtin(self, name: str) -> List[str]: ...           # argument already in acc
    def return_value_zero(self) -> List[str]: ...


class RV32(Machine):
    """RISC-V RV32IM, ILP32. acc = a0, tmp = t1, scratch = t0."""
    name = "rv32"
    ptr_size = 4
    push_size = 4
    max_args = 8
    frame_reserved = 16          # saved ra at fp-4, saved fp at fp-8, padding to 16

    def header(self):
        # Program entry: C says execution starts at main; somebody must call it
        # and turn its return value into an exit status. That somebody is crt0.
        return [".text", ".globl _start", "_start:", "call main", "li a7, 93", "ecall"]

    def footer(self, globals_):
        out = []
        if globals_:
            out.append(".data")
            for g in globals_:
                out += [".align 2", f"{g.name}:", f".zero {4 * (g.size if g.ty == 'arr' else 1)}"]
        return out

    def prologue(self, name, frame):
        out = [f".globl {name}", f"{name}:", "addi sp, sp, -16", "sw ra, 12(sp)",
               "sw fp, 8(sp)", "addi fp, sp, 16"]
        if frame > 16:
            out.append(f"addi sp, sp, -{frame - 16}")
        return out

    def epilogue(self):
        return ["addi sp, fp, -16", "lw ra, 12(sp)", "lw fp, 8(sp)", "addi sp, sp, 16", "ret"]

    def spill_param(self, i, offset, ty):
        return [f"sw a{i}, -{offset}(fp)"]

    def load_imm(self, v):
        return [f"li a0, {v}"]

    def local_addr(self, offset):
        return [f"addi a0, fp, -{offset}"]

    def global_addr(self, name):
        return [f"la a0, {name}"]

    def load(self, ty):
        return ["lw a0, 0(a0)"]

    def store(self, ty):
        return ["sw t1, 0(a0)", "mv a0, t1"]

    def push(self):
        return ["addi sp, sp, -4", "sw a0, 0(sp)"]

    def pop_tmp(self):
        return ["lw t1, 0(sp)", "addi sp, sp, 4"]

    def pop_arg(self, i):
        return [f"lw a{i}, 0(sp)", "addi sp, sp, 4"]

    def acc_to_tmp(self):
        return ["mv t1, a0"]

    def swap_pop(self):
        return ["mv t1, a0", "lw a0, 0(sp)", "addi sp, sp, 4"]

    def neg(self):
        return ["neg a0, a0"]

    def lognot(self):
        return ["seqz a0, a0"]

    def normalize_bool(self):
        return ["snez a0, a0"]

    def binop(self, op, ty):
        return {
            "+": ["add a0, a0, t1"], "-": ["sub a0, a0, t1"],
            "*": ["mul a0, a0, t1"], "/": ["div a0, a0, t1"], "%": ["rem a0, a0, t1"],
            "==": ["sub a0, a0, t1", "seqz a0, a0"],
            "!=": ["sub a0, a0, t1", "snez a0, a0"],
            "<": ["slt a0, a0, t1"], ">": ["slt a0, t1, a0"],
            "<=": ["slt a0, t1, a0", "xori a0, a0, 1"],
            ">=": ["slt a0, a0, t1", "xori a0, a0, 1"],
        }[op]

    def scale_tmp(self, shift):
        return [f"slli t1, t1, {shift}"]

    def scale_acc(self, shift):
        return [f"slli a0, a0, {shift}"]

    def unscale_acc(self, shift):
        return [f"srai a0, a0, {shift}"]

    def branch_zero(self, label):
        return [f"beqz a0, {label}"]

    def branch_nonzero(self, label):
        return [f"bnez a0, {label}"]

    def jump(self, label):
        return [f"j {label}"]

    def adjust_sp(self, delta):
        return [f"addi sp, sp, {delta}"]

    def call(self, name):
        return [f"call {name}"]

    def builtin(self, name):
        return [f"li a7, {BUILTINS[name]}", "ecall"]

    def return_value_zero(self):
        return ["li a0, 0"]


class X86_64(Machine):
    """x86-64 System V, LP64, AT&T syntax. acc = rax, tmp = rcx.

    Ints are kept sign-extended in the full 64-bit register so pointer
    arithmetic and comparisons can use the same registers. Pointers are 8 bytes,
    so a local `int *p` needs a bigger slot than on RV32: the shared frame
    layout code asks `slot_size()` instead of assuming 4.
    """
    name = "x86_64"
    ptr_size = 8
    push_size = 8
    max_args = 6
    frame_reserved = 0           # return address and saved rbp sit above rbp
    ARGS = ["rdi", "rsi", "rdx", "rcx", "r8", "r9"]
    ARGS32 = ["edi", "esi", "edx", "ecx", "r8d", "r9d"]

    def header(self):
        # No _start here: the host's crt0 calls main and passes its result to exit.
        return [".text"]

    def footer(self, globals_):
        out = []
        if globals_:
            out.append(".data")
            for g in globals_:
                out += [".align 4", f"{g.name}:", f".zero {4 * (g.size if g.ty == 'arr' else 1)}"]
        out.append('.section .note.GNU-stack,"",@progbits')
        return out

    def prologue(self, name, frame):
        out = [f".globl {name}", f"{name}:", "pushq %rbp", "movq %rsp, %rbp"]
        if frame:
            out.append(f"subq ${frame}, %rsp")
        return out

    def epilogue(self):
        return ["movq %rbp, %rsp", "popq %rbp", "ret"]

    def spill_param(self, i, offset, ty):
        if ty == "ptr":
            return [f"movq %{self.ARGS[i]}, -{offset}(%rbp)"]
        return [f"movslq %{self.ARGS32[i]}, %{self.ARGS[i]}", f"movq %{self.ARGS[i]}, -{offset}(%rbp)"]

    def load_imm(self, v):
        return [f"movq ${v}, %rax"]

    def local_addr(self, offset):
        return [f"leaq -{offset}(%rbp), %rax"]

    def global_addr(self, name):
        return [f"leaq {name}(%rip), %rax"]

    def load(self, ty):
        return ["movq (%rax), %rax"] if ty == "ptr" else ["movslq (%rax), %rax"]

    def store(self, ty):
        if ty == "ptr":
            return ["movq %rcx, (%rax)", "movq %rcx, %rax"]
        return ["movl %ecx, (%rax)", "movq %rcx, %rax"]

    def push(self):
        return ["pushq %rax"]

    def pop_tmp(self):
        return ["popq %rcx"]

    def pop_arg(self, i):
        return [f"popq %{self.ARGS[i]}"]

    def acc_to_tmp(self):
        return ["movq %rax, %rcx"]

    def swap_pop(self):
        return ["movq %rax, %rcx", "popq %rax"]

    def neg(self):
        return ["negq %rax"]

    def lognot(self):
        return ["testq %rax, %rax", "sete %al", "movzbq %al, %rax"]

    def normalize_bool(self):
        return ["testq %rax, %rax", "setne %al", "movzbq %al, %rax"]

    def binop(self, op, ty):
        setcc = {"==": "sete", "!=": "setne", "<": "setl", ">": "setg", "<=": "setle", ">=": "setge"}
        if op in setcc:
            return ["cmpq %rcx, %rax", f"{setcc[op]} %al", "movzbq %al, %rax"]
        if op == "+":
            return ["addq %rcx, %rax"] + (["movslq %eax, %rax"] if ty == "int" else [])
        if op == "-":
            return ["subq %rcx, %rax"] + (["movslq %eax, %rax"] if ty == "int" else [])
        if op == "*":
            return ["imull %ecx, %eax", "movslq %eax, %rax"]
        # idivl divides edx:eax by the operand; cltd sign-extends eax into edx.
        if op == "/":
            return ["cltd", "idivl %ecx", "movslq %eax, %rax"]
        if op == "%":
            return ["cltd", "idivl %ecx", "movslq %edx, %rax"]
        raise CompileError(f"x86_64: unsupported operator {op}")

    def scale_tmp(self, shift):
        return [f"shlq ${shift}, %rcx"]

    def scale_acc(self, shift):
        return [f"shlq ${shift}, %rax"]

    def unscale_acc(self, shift):
        return [f"sarq ${shift}, %rax"]

    def branch_zero(self, label):
        return ["testq %rax, %rax", f"je {label}"]

    def branch_nonzero(self, label):
        return ["testq %rax, %rax", f"jne {label}"]

    def jump(self, label):
        return [f"jmp {label}"]

    def adjust_sp(self, delta):
        return [f"addq ${delta}, %rsp"]

    def call(self, name):
        return [f"call {name}"]

    def builtin(self, name):
        # On a hosted system the "builtins" are ordinary functions in runtime.c.
        return ["movl %eax, %edi", f"call {name}"]

    def return_value_zero(self):
        return ["movq $0, %rax"]


MACHINES = {"rv32": RV32, "x86_64": X86_64}
BUILTINS = {"print_int": 1, "print_char": 11}   # name -> ecall number (RARS convention)
INT_SHIFT = 2                                   # log2(sizeof(int)): pointer arithmetic scale


# --------------------------------------------------------------------------
# 5. Semantic analysis + code generation (one pass over the AST, target-neutral)
# --------------------------------------------------------------------------

class CodeGen:
    def __init__(self, machine: Machine) -> None:
        self.m = machine
        self.out: List[str] = []
        self.globals: Dict[str, Symbol] = {}
        self.funcs: Dict[str, Node] = {}
        self.scopes: List[Dict[str, Symbol]] = []
        self.frame_size = 0
        self.label_id = 0
        self.ret_label = ""
        self.depth = 0              # temporaries currently pushed (for call alignment)

    # -- helpers ----------------------------------------------------------
    def emit(self, lines: List[str]) -> None:
        for s in lines:
            self.out.append(s if (s.endswith(":") or s.startswith(".")) else "    " + s)

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
        nbytes = 4 * size if ty == "arr" else self.m.slot_size(ty)
        align = 4 if ty == "arr" else nbytes
        self.frame_size = (self.frame_size + nbytes + align - 1) // align * align
        sym = Symbol(name, ty, size, offset=self.frame_size)
        self.scopes[-1][name] = sym
        return sym

    def push(self) -> None:
        self.emit(self.m.push())
        self.depth += 1

    def pop_tmp(self) -> None:
        self.emit(self.m.pop_tmp())
        self.depth -= 1

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
                if len(it.params) > self.m.max_args:
                    raise CompileError(f"line {it.line}: {self.m.name} backend passes at most "
                                       f"{self.m.max_args} arguments in registers")
                self.funcs[it.name] = it
        if "main" not in self.funcs:
            raise CompileError("no 'main' function")
        self.emit(self.m.header())
        for f in self.funcs.values():
            self.function(f)
        self.emit(self.m.footer(list(self.globals.values())))
        return "\n".join(self.out) + "\n"

    # -- functions --------------------------------------------------------
    def function(self, f: Node) -> None:
        # Body is generated first into a side buffer so the frame size is known
        # when the prologue is written.
        saved_out, self.out = self.out, []
        self.frame_size = self.m.frame_reserved
        self.scopes = [{}]
        self.depth = 0
        self.ret_label = self.label()
        for i, (name, ty) in enumerate(f.params):
            sym = self.declare_local(name, ty, 0, f.line)
            self.emit(self.m.spill_param(i, sym.offset, ty))       # spill incoming args
        for s in f.body:
            self.statement(s)
        self.emit(self.m.return_value_zero())                      # fall off the end -> 0
        self.emit([f"{self.ret_label}:"])
        body, self.out = self.out, saved_out
        self.scopes = []

        frame = (self.frame_size + 15) // 16 * 16     # keep sp 16-byte aligned (both ABIs)
        self.emit(self.m.prologue(f.name, frame))
        self.out.extend(body)
        self.emit(self.m.epilogue())

    # -- statements -------------------------------------------------------
    def statement(self, s: Node) -> None:
        k = s.kind
        if k == "decl":
            sym = self.declare_local(s.name, s.ty, s.size, s.line)
            if s.init is not None:
                self.expression(s.init)
                self.emit(self.m.acc_to_tmp())
                self.emit(self.m.local_addr(sym.offset))
                self.emit(self.m.store(sym.ty))
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
                self.emit(self.m.return_value_zero())
            self.emit(self.m.jump(self.ret_label))
        elif k == "if":
            l_else, l_end = self.label(), self.label()
            self.expression(s.cond)
            self.emit(self.m.branch_zero(l_else))
            self.statement(s.then)
            self.emit(self.m.jump(l_end))
            self.emit([f"{l_else}:"])
            if s.els is not None:
                self.statement(s.els)
            self.emit([f"{l_end}:"])
        elif k == "while":
            l_top, l_end = self.label(), self.label()
            self.emit([f"{l_top}:"])
            self.expression(s.cond)
            self.emit(self.m.branch_zero(l_end))
            self.statement(s.then)
            self.emit(self.m.jump(l_top))
            self.emit([f"{l_end}:"])
        elif k == "for":
            self.scopes.append({})
            l_top, l_end = self.label(), self.label()
            if s.init is not None:
                self.statement(s.init)
            self.emit([f"{l_top}:"])
            if s.cond is not None:
                self.expression(s.cond)
                self.emit(self.m.branch_zero(l_end))
            self.statement(s.then)
            if s.step is not None:
                self.expression(s.step)
            self.emit(self.m.jump(l_top))
            self.emit([f"{l_end}:"])
            self.scopes.pop()
        else:
            raise CompileError(f"line {s.line}: unknown statement {k}")

    # -- expressions: result always ends up in the accumulator ---------------
    def address_of(self, e: Node) -> str:
        """Emit code that leaves the address of lvalue `e` in acc; return its type."""
        if e.kind == "var":
            sym = self.lookup(e.name, e.line)
            self.emit(self.m.global_addr(sym.name) if sym.is_global else self.m.local_addr(sym.offset))
            return sym.ty
        if e.kind == "deref":
            t = self.expression(e.lhs)
            if t != "ptr":
                raise CompileError(f"line {e.line}: '*' applied to a non-pointer")
            return "int"
        raise CompileError(f"line {e.line}: expression is not an lvalue")

    def expression(self, e: Node) -> str:
        """Emit code for `e`, leaving its value in acc; return 'int' or 'ptr'."""
        k = e.kind
        if k == "num":
            self.emit(self.m.load_imm(e.value))
            return "int"
        if k == "var":
            t = self.address_of(e)
            if t == "arr":                     # arrays decay to a pointer to element 0
                return "ptr"
            self.emit(self.m.load(t))
            return t
        if k == "deref":
            self.address_of(e)
            self.emit(self.m.load("int"))
            return "int"
        if k == "addr":
            self.address_of(e.lhs)
            return "ptr"
        if k == "assign":
            self.expression(e.rhs)
            self.push()
            t = self.address_of(e.lhs)
            if t == "arr":
                raise CompileError(f"line {e.line}: cannot assign to an array")
            self.pop_tmp()
            self.emit(self.m.store(t))
            return t
        if k == "unary":
            self.expression(e.lhs)
            self.emit(self.m.neg() if e.op == "-" else self.m.lognot())
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
            self.aligned_call(lambda: self.m.builtin(e.name))
            return "int"
        f = self.funcs.get(e.name)
        if f is None:
            raise CompileError(f"line {e.line}: call to undefined function {e.name!r}")
        if len(e.args) != len(f.params):
            raise CompileError(f"line {e.line}: {e.name} expects {len(f.params)} arguments")
        # Evaluate every argument first (each may itself call), park them on the
        # stack, then move them into the argument registers.
        for a in e.args:
            self.expression(a)
            self.push()
        for i in reversed(range(len(e.args))):
            self.emit(self.m.pop_arg(i))
            self.depth -= 1
        self.aligned_call(lambda: self.m.call(e.name))
        return f.ty

    def aligned_call(self, emit_call) -> None:
        """Both ABIs require sp to be 16-byte aligned at a call. Temporaries pushed
        by enclosing expressions may have broken that, so pad around the call."""
        pad = (-self.depth * self.m.push_size) % 16
        if pad:
            self.emit(self.m.adjust_sp(-pad))
        self.emit(emit_call())
        if pad:
            self.emit(self.m.adjust_sp(pad))

    def binary(self, e: Node) -> str:
        op = e.op
        if op in ("&&", "||"):
            l_end = self.label()
            self.expression(e.lhs)
            self.emit(self.m.normalize_bool())
            self.emit(self.m.branch_zero(l_end) if op == "&&" else self.m.branch_nonzero(l_end))
            self.expression(e.rhs)
            self.emit(self.m.normalize_bool())
            self.emit([f"{l_end}:"])
            return "int"

        lt = self.expression(e.lhs)
        self.push()
        rt = self.expression(e.rhs)
        self.emit(self.m.swap_pop())      # acc = lhs, tmp = rhs
        self.depth -= 1

        if op in ("+", "-"):
            # Pointer arithmetic scales by the element size, like real C.
            if lt == "ptr" and rt == "int":
                self.emit(self.m.scale_tmp(INT_SHIFT))
                self.emit(self.m.binop(op, "ptr"))
                return "ptr"
            if lt == "int" and rt == "ptr" and op == "+":
                self.emit(self.m.scale_acc(INT_SHIFT))
                self.emit(self.m.binop(op, "ptr"))
                return "ptr"
            if lt == "ptr" and rt == "ptr" and op == "-":
                self.emit(self.m.binop("-", "ptr"))
                self.emit(self.m.unscale_acc(INT_SHIFT))
                return "int"
            if lt == "int" and rt == "int":
                self.emit(self.m.binop(op, "int"))
                return "int"
            raise CompileError(f"line {e.line}: invalid operands for {op}")

        if (lt == "ptr" or rt == "ptr") and op not in ("==", "!=", "<", "<=", ">", ">="):
            raise CompileError(f"line {e.line}: invalid pointer operands for {op}")
        self.emit(self.m.binop(op, "ptr" if (lt == "ptr" or rt == "ptr") else "int"))
        return "int"


# --------------------------------------------------------------------------
# Driver
# --------------------------------------------------------------------------

def compile_source(src: str, target: str = "rv32") -> str:
    tokens = lex(src)
    ast = Parser(tokens).program()
    return CodeGen(MACHINES[target]()).program(ast)


def main(argv: List[str]) -> int:
    if len(argv) < 2 or argv[1] in ("-h", "--help"):
        print("usage: tinyc.py input.c [-o output.s] [--target rv32|x86_64] [--tokens] [--ast]",
              file=sys.stderr)
        return 2
    path = argv[1]
    out_path = argv[argv.index("-o") + 1] if "-o" in argv else None
    target = argv[argv.index("--target") + 1] if "--target" in argv else "rv32"
    if target not in MACHINES:
        print(f"unknown target {target!r}; choose from {', '.join(MACHINES)}", file=sys.stderr)
        return 2
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
        asm = compile_source(src, target)
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
