#!/usr/bin/env python3
"""minilang_vm: a bytecode compiler and stack VM for minilang.

Pipeline: source -> minilang.parse() (the stage 2 lexer and parser, reused
unchanged) -> Resolver (scopes, slots, captured variables, upvalues) ->
Compiler (bytecode, jump patching) -> Code objects (constant pool, line table,
exception table, computed maximum stack depth) -> VM (one dispatch loop; call
frames live in a VM list, not on the host stack). This file corresponds to
stage 4 of the research roadmap.

The VM is a subclass of minilang.Interpreter. It inherits the builtins, the
output hook and the generic operator and indexing code, which it calls on
slow paths, so both engines share one value model and one set of error
messages. Name resolution, control flow, calls, closures and exceptions are
the VM's own.

Usage:
    python3 minilang_vm.py program.ml          # run a file on the VM
    python3 minilang_vm.py --dis program.ml    # print the bytecode
    python3 minilang_vm.py --bench [runs]      # time both engines on programs/bench_*.ml
    python3 minilang_vm.py                     # REPL
Add --opt to enable the optional compiler optimizations.
"""
import bisect
import operator
import os
import platform
import statistics
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "minilang"))

import minilang  # noqa: E402  stage 2: front end and runtime library
from minilang import Builtin, RuntimeErr, stringify, type_name, values_equal  # noqa: E402

TreeWalker = minilang.Interpreter  # the reference engine, captured before tests patch minilang.Interpreter

# --------------------------------------------------------------- opcodes ----
# An instruction is two list items, opcode then operand (0 when unused). Fixed
# width keeps jump targets and the line table simple, like CPython's 16-bit
# code units. The second column is the stack effect; None means "depends on
# the operand or on the branch taken" and is handled in stack_depth().

OPCODES = (
    ("LOAD_CONST", 1),            # push consts[arg]
    ("LOAD_LOCAL", 1),            # push slot arg
    ("STORE_LOCAL", -1),          # pop into slot arg
    ("LOAD_CELL", 1),             # push the value in the cell held by slot arg
    ("STORE_CELL", -1),           # pop into the cell held by slot arg
    ("MAKE_CELL", 0),             # slot arg = new empty cell (scope entry)
    ("CELL_ARG", 0),              # slot arg = new cell holding the argument in it
    ("LOAD_UPVAL", 1),            # push the value of the closure's upvalue arg
    ("STORE_UPVAL", -1),
    ("LOAD_GLOBAL", 1),           # push globals[consts[arg]]
    ("STORE_GLOBAL", -1),         # assign to an existing global
    ("DEFINE_GLOBAL", -1),        # top-level let and fn
    ("LOAD_CHAIN", 1),            # chains[arg]: first defined candidate wins
    ("STORE_CHAIN", -1),
    ("POP", -1),
    ("DUP", 1),
    ("ADD", -1), ("SUB", -1), ("MUL", -1), ("DIV", -1), ("MOD", -1),
    ("EQ", -1), ("NE", -1), ("LT", -1), ("LE", -1), ("GT", -1), ("GE", -1),
    ("NEG", 0), ("NOT", 0),
    ("INDEX", -1),                # obj, index -> item
    ("SET_INDEX", -2),            # obj, index, value -> value
    ("BUILD_LIST", None),         # pop arg items, push a list
    ("MAKE_CLOSURE", 1),          # push a closure over the Code in consts[arg]
    ("CALL", None),               # pop callee and arg arguments, push the result
    ("JUMP", 0),
    ("JUMP_IF_FALSE", -1),        # pop; jump if falsy
    ("JUMP_IF_FALSE_OR_POP", None),  # `and`: falsy -> keep and jump, else pop
    ("JUMP_IF_TRUE_OR_POP", None),   # `or`: truthy -> keep and jump, else pop
    ("RETURN", -1),
    ("THROW", -1),
    ("BREAK_OUT", 0),             # break outside a loop: unwind to a caller's loop
    ("HALT", -1),                 # end of the script; returns the trailing value
    # Superinstructions, emitted only when optimizing (see OPTIMIZATIONS):
    ("ADD_CONST", 0),             # top = top + consts[arg]
    ("SUB_CONST", 0),             # top = top - consts[arg]
    ("LT_CONST", 0),              # top = top < consts[arg]
    ("JUMP_IF_NOT_LT", -2),       # pop b, a; jump unless a < b
)

# Optional compiler optimizations; --opt (optimize=True) turns on all of them.
OPTIMIZATIONS = (
    "assign-statement",   # `x = e;` as a statement: store without DUP and POP
    "const-operand",      # `a + 1`, `a - 1`, `a < 1`: one instruction instead of LOAD_CONST + op
    "compare-jump",       # `if (a < b)` / `while (a < b)`: compare and branch in one instruction
)


def optimizations(optimize):
    """Normalize the optimize argument: False, True, or an iterable of names."""
    if optimize is True:
        return frozenset(OPTIMIZATIONS)
    chosen = frozenset(optimize or ())
    unknown = chosen - set(OPTIMIZATIONS)
    if unknown:
        raise ValueError("unknown optimization(s): %s" % ", ".join(sorted(unknown)))
    return chosen
OPNAMES = [name for name, _ in OPCODES]
STACK_EFFECT = [effect for _, effect in OPCODES]
(LOAD_CONST, LOAD_LOCAL, STORE_LOCAL, LOAD_CELL, STORE_CELL, MAKE_CELL, CELL_ARG,
 LOAD_UPVAL, STORE_UPVAL, LOAD_GLOBAL, STORE_GLOBAL, DEFINE_GLOBAL, LOAD_CHAIN,
 STORE_CHAIN, POP, DUP, ADD, SUB, MUL, DIV, MOD, EQ, NE, LT, LE, GT, GE, NEG, NOT,
 INDEX, SET_INDEX, BUILD_LIST, MAKE_CLOSURE, CALL, JUMP, JUMP_IF_FALSE,
 JUMP_IF_FALSE_OR_POP, JUMP_IF_TRUE_OR_POP, RETURN, THROW, BREAK_OUT, HALT,
 ADD_CONST, SUB_CONST, LT_CONST, JUMP_IF_NOT_LT) = range(len(OPCODES))

BINARY_OPCODES = {"+": ADD, "-": SUB, "*": MUL, "/": DIV, "%": MOD, "==": EQ,
                  "!=": NE, "<": LT, "<=": LE, ">": GT, ">=": GE}
JUMP_OPCODES = {JUMP, JUMP_IF_FALSE, JUMP_IF_FALSE_OR_POP, JUMP_IF_TRUE_OR_POP, JUMP_IF_NOT_LT}
LOCAL_OPCODES = {LOAD_LOCAL, STORE_LOCAL, LOAD_CELL, STORE_CELL, MAKE_CELL, CELL_ARG}
CONST_OPCODES = {LOAD_CONST, LOAD_GLOBAL, STORE_GLOBAL, DEFINE_GLOBAL, ADD_CONST,
                 SUB_CONST, LT_CONST}


class _Undefined:
    """Marks a cell whose `let` has not run yet (see LOAD_CHAIN)."""

    def __repr__(self):
        return "<undefined>"


UNDEFINED = _Undefined()

# ----------------------------------------------------------- code object ----


class Code:
    """A compiled function: CPython's code object, Lua's function prototype."""

    def __init__(self, name, params):
        self.name = name
        self.params = list(params)
        self.nparams = len(self.params)
        self.code = []          # opcode, operand, opcode, operand, ...
        self.consts = []        # constant pool: literals, global names, nested Code objects
        self._const_keys = {}
        self.linetable = []     # (first offset, line) runs; a new run starts when the line changes
        self.line_starts = []
        self.handlers = []      # exception table: (start, end, handler), innermost first
        self.loops = []         # loop table: (start, end), innermost first; used by BREAK_OUT
        self.chains = []        # operands of LOAD_CHAIN / STORE_CHAIN
        self.slot_names = []    # debug names of the local slots
        self.upval_specs = ()   # per upvalue: (taken from the enclosing frame's slot?, index)
        self.upval_names = ()
        self.nslots = 0
        self.max_stack = 0
        self.padding = []       # [None] * (nslots - nparams + max_stack), appended to the args on each call

    def __repr__(self):
        return "<code %s>" % self.name

    def const(self, value):
        """Index of value in the constant pool, adding it once. 1, 1.0 and true
        compare equal in Python, so the key includes the type."""
        key = ("code", id(value)) if isinstance(value, Code) else (type(value), repr(value))
        index = self._const_keys.get(key)
        if index is None:
            index = self._const_keys[key] = len(self.consts)
            self.consts.append(value)
        return index

    def line_at(self, offset):
        return self.linetable[bisect.bisect_right(self.line_starts, offset) - 1][1]

    def handler_at(self, offset):
        for start, end, handler in self.handlers:
            if start <= offset < end:
                return handler
        return None

    def loop_exit_at(self, offset):
        for start, end in self.loops:
            if start <= offset < end:
                return end
        return None

    def instructions(self):
        """Yield (offset, opcode, operand) for each instruction."""
        code = self.code
        for offset in range(0, len(code), 2):
            yield offset, code[offset], code[offset + 1]


class Closure(minilang.Function):
    """A VM function value: a Code object plus the cells it captured.

    Subclassing minilang.Function keeps type(), str() and == identical to the
    tree-walker, because those helpers are shared."""

    def __init__(self, code, upvals):
        self.name = code.name
        self.params = code.params
        self.code = code
        self.upvals = upvals


# -------------------------------------------------------------- resolver ----
# minilang environments are dictionaries filled as `let` runs, and a name
# means the innermost environment that holds it *at the moment of lookup*.
# Inside one function that is decided by source order, because a scope's
# statements run once each, in order (loop bodies get a fresh scope per
# iteration). A closure is different: it may run before or after a later
# `let` in an enclosing scope. So a name used inside a closure resolves to
#   - a definite binding: declared before the closure was created, or
#   - a chain: outer bindings declared after the closure was created, which
#     the VM tries innermost first, skipping cells still UNDEFINED, and then
#     the definite binding or the global table.
# Variables used by an inner function live in cells (heap boxes), created
# fresh each time their scope is entered; this is CPython's cell design. Lua
# and clox keep them on the stack as open upvalues and move them to the heap
# when the scope ends; in Python a box from the start is simpler and cheaper.


class Binding:
    __slots__ = ("name", "slot", "func", "declared", "captured")

    def __init__(self, name, slot, func, declared):
        self.name = name
        self.slot = slot
        self.func = func
        self.declared = declared   # has its let/fn been passed in source order?
        self.captured = False      # used by an inner function -> lives in a cell


class Scope:
    def __init__(self, parent, func, is_global=False):
        self.parent = parent
        self.func = func
        self.is_global = is_global
        self.bindings = {}


class FuncInfo:
    def __init__(self, parent, name):
        self.parent = parent
        self.name = name
        self.slot_names = []
        self.upvals = []          # (from the enclosing frame?, slot or enclosing upvalue index)
        self.upval_names = []
        self.upval_index = {}     # Binding -> upvalue index

    def new_slot(self, name):
        self.slot_names.append(name)
        return len(self.slot_names) - 1


class Resolver:
    """Walks the AST once, in execution order, and records how every name is
    reached. The Compiler reads these tables and never tracks scopes itself."""

    def __init__(self):
        self.refs = {}       # Var / Assign node -> access tuple
        self.targets = {}    # Let / FnDecl node -> Binding, or None for a global; Try node -> catch Binding
        self.scopes = {}     # Block / function / Try node -> {name: Binding} declared in that scope
        self.funcs = {}      # Program / FnDecl / FnExpr node -> FuncInfo
        self.func = None
        self.scope = None

    def program(self, node):
        self.func = self.funcs[node] = FuncInfo(None, "<script>")
        self.scope = Scope(None, self.func, is_global=True)   # top level runs in the globals
        for stmt in node.body:
            self.stmt(stmt)

    def push_scope(self, key, statements, declared=()):
        scope = Scope(self.scope, self.func)
        for name in declared:   # parameters and the catch variable; a repeated name keeps the last slot
            scope.bindings[name] = Binding(name, self.func.new_slot(name), self.func, True)
        for stmt in statements:
            if stmt.kind in ("Let", "FnDecl") and stmt.name not in scope.bindings:
                scope.bindings[stmt.name] = Binding(stmt.name, self.func.new_slot(stmt.name), self.func, False)
        self.scopes[key] = scope.bindings
        self.scope = scope

    def pop_scope(self):
        self.scope = self.scope.parent

    def declare(self, node):
        if self.scope.is_global:
            self.targets[node] = None
        else:
            binding = self.scope.bindings[node.name]
            binding.declared = True
            self.targets[node] = binding

    # -- statements --
    def stmt(self, node):
        kind = node.kind
        if kind == "ExprStmt":
            self.expr(node.expr)
        elif kind == "Let":
            self.expr(node.value)      # `let x = x + 1;` reads the outer x
            self.declare(node)
        elif kind == "FnDecl":
            self.declare(node)         # declared before the body, so it can call itself
            self.function(node, node.name)
        elif kind == "Block":
            self.block(node)
        elif kind == "If":
            self.expr(node.cond)
            self.block(node.then)
            if node.otherwise is not None:
                self.stmt(node.otherwise)   # a Block or an else-if
        elif kind == "While":
            self.expr(node.cond)
            self.block(node.body)
        elif kind in ("Return", "Throw"):
            if node.value is not None:
                self.expr(node.value)
        elif kind == "Try":
            self.block(node.body)
            self.push_scope(node, node.handler.body, declared=[node.name])
            self.targets[node] = self.scope.bindings[node.name]
            for stmt in node.handler.body:
                self.stmt(stmt)
            self.pop_scope()
        # Break needs no names

    def block(self, node):
        self.push_scope(node, node.body)
        for stmt in node.body:
            self.stmt(stmt)
        self.pop_scope()

    def function(self, node, name):
        outer_func, outer_scope = self.func, self.scope
        self.func = self.funcs[node] = FuncInfo(outer_func, name)
        self.push_scope(node, node.body.body, declared=node.params)   # params take slots 0..n-1
        for stmt in node.body.body:
            self.stmt(stmt)
        self.func, self.scope = outer_func, outer_scope

    # -- expressions --
    def expr(self, node):
        kind = node.kind
        if kind == "Var":
            self.refs[node] = self.lookup(node.name)
        elif kind == "Assign":
            self.expr(node.value)
            self.refs[node] = self.lookup(node.name)
        elif kind == "FnExpr":
            self.function(node, "<anonymous>")
        elif kind in ("Logical", "Binary"):
            self.expr(node.left)
            self.expr(node.right)
        elif kind == "Unary":
            self.expr(node.operand)
        elif kind == "ListLit":
            for item in node.items:
                self.expr(item)
        elif kind == "Index":
            self.expr(node.obj)
            self.expr(node.index)
        elif kind == "SetIndex":
            self.expr(node.obj)
            self.expr(node.index)
            self.expr(node.value)
        elif kind == "Call":
            self.expr(node.callee)
            for arg in node.args:
                self.expr(arg)
        # Literal needs no names

    def lookup(self, name):
        """Return ("local", Binding) | ("upval", index) | ("global", name)
        | ("chain", name, (maybe upvalue indices...), final access)."""
        maybe, found = [], None
        scope = self.scope
        while not scope.is_global:
            binding = scope.bindings.get(name)
            if binding is not None:
                if binding.declared:
                    found = binding
                    break
                if binding.func is not self.func:   # declared later in an enclosing function
                    maybe.append(binding)
                # declared later in this same function: not visible yet, keep looking outward
            scope = scope.parent
        if found is None:
            final = ("global", name)
        elif found.func is self.func:
            final = ("local", found)
        else:
            final = ("upval", self.upvalue(self.func, found))
        if not maybe:
            return final
        return ("chain", name, tuple(self.upvalue(self.func, b) for b in maybe), final)

    def upvalue(self, func, binding):
        """Index of binding among func's upvalues, threading it through every
        function in between (clox's resolveUpvalue)."""
        index = func.upval_index.get(binding)
        if index is None:
            if func.parent is binding.func:
                binding.captured = True
                spec = (True, binding.slot)
            else:
                spec = (False, self.upvalue(func.parent, binding))
            func.upvals.append(spec)
            func.upval_names.append(binding.name)
            index = func.upval_index[binding] = len(func.upvals) - 1
        return index


# -------------------------------------------------------------- compiler ----


class Compiler:
    """Emits bytecode from the AST using the Resolver's tables."""

    def __init__(self, resolver, optimize=False):
        self.r = resolver
        self.opts = optimizations(optimize)
        self.co = None
        self.loops = []    # one list of pending break jumps per enclosing loop in this function

    # -- emission helpers --
    def emit(self, op, arg, line):
        co = self.co
        if not co.linetable or co.linetable[-1][1] != line:
            co.linetable.append((len(co.code), line))
        co.code.append(op)
        co.code.append(arg)
        return len(co.code) - 2

    def emit_jump(self, op, line):
        return self.emit(op, -1, line)    # target patched later

    def patch(self, at):
        self.co.code[at + 1] = len(self.co.code)

    def cells(self, bindings, line):
        for binding in bindings.values():
            if binding.captured:
                self.emit(CELL_ARG if binding.slot < self.co.nparams else MAKE_CELL, binding.slot, line)

    def finish(self, co, info):
        co.nslots = len(info.slot_names)
        co.slot_names = info.slot_names
        co.upval_specs = tuple(info.upvals)
        co.upval_names = tuple(info.upval_names)
        co.line_starts = [start for start, _ in co.linetable]
        co.max_stack = stack_depth(co)
        co.padding = [None] * (co.nslots - co.nparams + co.max_stack)
        return co

    # -- entry points --
    def script(self, program):
        co = self.co = Code("<script>", [])
        body = program.body
        last_line = body[-1].line if body else 1
        for i, stmt in enumerate(body):
            if i == len(body) - 1 and stmt.kind == "ExprStmt":
                self.expr(stmt.expr)        # the REPL shows this value
                break
            self.stmt(stmt)
        else:
            self.emit(LOAD_CONST, co.const(None), last_line)
        self.emit(HALT, 0, last_line)
        return self.finish(co, self.r.funcs[program])

    def function(self, node, name):
        outer, outer_loops = self.co, self.loops
        co = self.co = Code(name, node.params)
        self.loops = []
        self.cells(self.r.scopes[node], node.line)
        for stmt in node.body.body:
            self.stmt(stmt)
        self.emit(LOAD_CONST, co.const(None), node.line)
        self.emit(RETURN, 0, node.line)
        self.finish(co, self.r.funcs[node])
        self.co, self.loops = outer, outer_loops
        self.emit(MAKE_CLOSURE, self.co.const(co), node.line)

    # -- statements --
    def stmt(self, node):
        kind, line, co = node.kind, node.line, self.co
        if kind == "ExprStmt":
            expr = node.expr
            if "assign-statement" in self.opts and expr.kind == "Assign":
                self.expr(expr.value)               # no DUP / POP pair
                self.store(self.r.refs[expr], expr.line)
            else:
                self.expr(expr)
                self.emit(POP, 0, line)
        elif kind == "Let":
            self.expr(node.value)
            self.define(self.r.targets[node], node.name, line)
        elif kind == "FnDecl":
            self.function(node, node.name)
            self.define(self.r.targets[node], node.name, line)
        elif kind == "Block":
            self.block(node)
        elif kind == "If":
            skip_then = self.condition(node.cond, line)
            self.block(node.then)
            if node.otherwise is None:
                self.patch(skip_then)
            else:
                skip_else = self.emit_jump(JUMP, line)
                self.patch(skip_then)
                self.stmt(node.otherwise)
                self.patch(skip_else)
        elif kind == "While":
            start = len(co.code)
            exit_jump = self.condition(node.cond, line)
            self.loops.append([])
            self.block(node.body)
            self.emit(JUMP, start, line)
            self.patch(exit_jump)
            for jump in self.loops.pop():
                self.patch(jump)
            co.loops.append((start, len(co.code)))
        elif kind == "Return":
            if node.value is None:
                self.emit(LOAD_CONST, co.const(None), line)
            else:
                self.expr(node.value)
            self.emit(RETURN, 0, line)
        elif kind == "Break":
            if self.loops:
                self.loops[-1].append(self.emit_jump(JUMP, line))
            else:
                # The tree-walker signals break with a host exception, which
                # crosses function calls and stops the caller's loop.
                self.emit(BREAK_OUT, 0, line)
        elif kind == "Throw":
            self.expr(node.value)
            self.emit(THROW, 0, line)
        elif kind == "Try":
            start = len(co.code)
            self.block(node.body)
            end = len(co.code)
            skip_handler = self.emit_jump(JUMP, line)
            co.handlers.append((start, end, len(co.code)))   # the VM pushes the thrown value
            self.cells(self.r.scopes[node], line)
            self.define(self.r.targets[node], node.name, line)
            for stmt in node.handler.body:
                self.stmt(stmt)
            self.patch(skip_handler)
        else:
            raise RuntimeErr("unknown statement %s" % kind, line)

    def block(self, node):
        self.cells(self.r.scopes[node], node.line)
        for stmt in node.body:
            self.stmt(stmt)

    def condition(self, cond, line):
        """Compile a branch condition; return the jump to patch for 'false'."""
        if "compare-jump" in self.opts and cond.kind == "Binary" and cond.op == "<":
            self.expr(cond.left)
            self.expr(cond.right)
            return self.emit(JUMP_IF_NOT_LT, -1, cond.line)
        self.expr(cond)
        return self.emit_jump(JUMP_IF_FALSE, line)

    # -- names --
    def define(self, binding, name, line):
        if binding is None:
            self.emit(DEFINE_GLOBAL, self.co.const(name), line)
        else:
            self.emit(STORE_CELL if binding.captured else STORE_LOCAL, binding.slot, line)

    def load(self, access, line):
        kind = access[0]
        if kind == "local":
            binding = access[1]
            self.emit(LOAD_CELL if binding.captured else LOAD_LOCAL, binding.slot, line)
        elif kind == "upval":
            self.emit(LOAD_UPVAL, access[1], line)
        elif kind == "global":
            self.emit(LOAD_GLOBAL, self.co.const(access[1]), line)
        else:
            self.emit(LOAD_CHAIN, self.chain(access), line)

    def store(self, access, line):
        kind = access[0]
        if kind == "local":
            binding = access[1]
            self.emit(STORE_CELL if binding.captured else STORE_LOCAL, binding.slot, line)
        elif kind == "upval":
            self.emit(STORE_UPVAL, access[1], line)
        elif kind == "global":
            self.emit(STORE_GLOBAL, self.co.const(access[1]), line)
        else:
            self.emit(STORE_CHAIN, self.chain(access), line)

    def chain(self, access):
        _, name, maybe, final = access
        self.co.chains.append((name, maybe, final[0], final[1]))
        return len(self.co.chains) - 1

    # -- expressions --
    def expr(self, node):
        kind, line = node.kind, node.line
        if kind == "Literal":
            self.emit(LOAD_CONST, self.co.const(node.value), line)
        elif kind == "Var":
            self.load(self.r.refs[node], line)
        elif kind == "Assign":
            self.expr(node.value)
            self.emit(DUP, 0, line)
            self.store(self.r.refs[node], line)
        elif kind == "ListLit":
            for item in node.items:
                self.expr(item)
            self.emit(BUILD_LIST, len(node.items), line)
        elif kind == "FnExpr":
            self.function(node, "<anonymous>")
        elif kind == "Logical":
            self.expr(node.left)
            jump = self.emit_jump(JUMP_IF_TRUE_OR_POP if node.op == "or" else JUMP_IF_FALSE_OR_POP, line)
            self.expr(node.right)
            self.patch(jump)
        elif kind == "Unary":
            self.expr(node.operand)
            self.emit(NOT if node.op == "not" else NEG, 0, line)
        elif kind == "Binary":
            self.expr(node.left)
            right = node.right
            if ("const-operand" in self.opts and node.op in ("+", "-", "<") and right.kind == "Literal"
                    and type(right.value) in (int, float)):
                op = {"+": ADD_CONST, "-": SUB_CONST, "<": LT_CONST}[node.op]
                self.emit(op, self.co.const(right.value), line)
            else:
                self.expr(right)
                self.emit(BINARY_OPCODES[node.op], 0, line)
        elif kind == "Index":
            self.expr(node.obj)
            self.expr(node.index)
            self.emit(INDEX, 0, line)
        elif kind == "SetIndex":
            self.expr(node.obj)
            self.expr(node.index)
            self.expr(node.value)
            self.emit(SET_INDEX, 0, line)
        elif kind == "Call":
            self.expr(node.callee)
            for arg in node.args:
                self.expr(arg)
            self.emit(CALL, len(node.args), line)
        else:
            raise RuntimeErr("unknown expression %s" % kind, line)


def stack_depth(co):
    """Maximum operand stack depth over every path through the code, checking
    that paths agree where they meet (CPython's assembler does the same)."""
    code, seen, deepest = co.code, {}, 0
    work = [(0, 0)] + [(handler, 1) for _, _, handler in co.handlers]
    while work:
        pc, depth = work.pop()
        while pc not in seen:
            seen[pc] = depth
            deepest = max(deepest, depth)
            op, arg = code[pc], code[pc + 1]
            if op == CALL:
                depth -= arg
            elif op == BUILD_LIST:
                depth += 1 - arg
            elif op in (JUMP_IF_FALSE_OR_POP, JUMP_IF_TRUE_OR_POP):
                work.append((arg, depth))   # the jump keeps the value
                depth -= 1
            else:
                depth += STACK_EFFECT[op]
            if depth < 0:
                raise AssertionError("stack underflow at offset %d in %s" % (pc, co.name))
            deepest = max(deepest, depth)
            if op in (JUMP_IF_FALSE, JUMP_IF_NOT_LT):
                work.append((arg, depth))
            if op == JUMP:
                pc = arg
            elif op in (RETURN, THROW, BREAK_OUT, HALT):
                break
            else:
                pc += 2
        else:
            if seen[pc] != depth:
                raise AssertionError("stack depth %d vs %d at offset %d in %s"
                                     % (seen[pc], depth, pc, co.name))
    return deepest


def compile_program(program, optimize=False):
    resolver = Resolver()
    resolver.program(program)
    return Compiler(resolver, optimize).script(program)


# ---------------------------------------------------------- disassembler ----


def disassemble(co):
    """Text listing of a Code object and every function nested in it."""
    out = ["== %s  params=%s slots=%d upvalues=%d max_stack=%d ==" % (
        co.name, ",".join(co.params) or "-", co.nslots, len(co.upval_specs), co.max_stack)]
    out.append("constants: " + (", ".join("%d=%r" % kv for kv in enumerate(co.consts)) or "-"))
    if co.upval_specs:
        out.append("upvalues: " + ", ".join(
            "%d=%s(%s %d)" % (i, name, "slot" if local else "upvalue", index)
            for i, (name, (local, index)) in enumerate(zip(co.upval_names, co.upval_specs))))
    last_line = None
    for offset, op, arg in co.instructions():
        line = co.line_at(offset)
        shown = "%4d" % line if line != last_line else "   |"
        last_line = line
        out.append("%5d %s  %-20s %s" % (offset, shown, OPNAMES[op], _operand(co, op, arg)))
    for start, end, handler in co.handlers:
        out.append("exception: %d-%d -> %d" % (start, end, handler))
    for start, end in co.loops:
        out.append("loop: %d-%d" % (start, end))
    out.append("line table: " + " ".join("%d:%d" % run for run in co.linetable))
    for const in co.consts:
        if isinstance(const, Code):
            out.append("")
            out.append(disassemble(const))
    return "\n".join(out)


def _operand(co, op, arg):
    if op in JUMP_OPCODES:
        return "-> %d" % arg
    if op in CONST_OPCODES or op == MAKE_CLOSURE:
        return "%d (%r)" % (arg, co.consts[arg])
    if op in LOCAL_OPCODES:
        return "%d (%s)" % (arg, co.slot_names[arg])
    if op in (LOAD_UPVAL, STORE_UPVAL):
        return "%d (%s)" % (arg, co.upval_names[arg])
    if op in (LOAD_CHAIN, STORE_CHAIN):
        name, maybe, kind, final = co.chains[arg]
        steps = ["upvalue %d if defined" % i for i in maybe]
        steps.append("global" if kind == "global" else "upvalue %d" % final)
        return "%d (%s: %s)" % (arg, name, ", then ".join(steps))
    if op in (CALL, BUILD_LIST):
        return str(arg)
    return ""


# -------------------------------------------------------------------- VM ----


def _site(op, co, pc):
    """A stand-in AST node for the shared slow-path operator code."""
    return minilang.Node("Binary", co.line_at(pc - 2), op=op)


class VM(minilang.Interpreter):
    """Runs minilang programs as bytecode. Same constructor and run() contract
    as minilang.Interpreter, so the stage 2 test suite runs on it unchanged."""

    def __init__(self, output=None, optimize=False):
        super().__init__(output)
        self.optimize = optimizations(optimize)

    def compile(self, src):
        return compile_program(minilang.parse(src), self.optimize)

    def run(self, src):
        """Parse, compile and execute. Returns the value of a trailing expression statement."""
        program = minilang.parse(src)
        old_limit = sys.getrecursionlimit()
        sys.setrecursionlimit(max(old_limit, self.MAX_DEPTH * 12 + 500))   # compiler recursion, as in stage 2
        try:
            return self.execute(compile_program(program, self.optimize))
        finally:
            sys.setrecursionlimit(old_limit)

    def execute(self, script):
        g = self.globals.vars
        binary = self.binary
        max_depth = self.MAX_DEPTH
        frames = []                       # saved (code, pc, regs, sp, upvals) of each caller
        co = script
        code, consts, upvals = co.code, co.consts, ()
        # One list per frame: local slots first, then the operand stack, sized
        # by the compiler's max_stack (CPython's "localsplus" layout).
        regs = [None] * (co.nslots + co.max_stack)
        sp = co.nslots
        pc = 0
        while True:
            try:
                while True:
                    op = code[pc]
                    arg = code[pc + 1]
                    pc += 2
                    # Ordered roughly by how often the benchmarks execute them.
                    if op == LOAD_LOCAL:
                        regs[sp] = regs[arg]
                        sp += 1
                    elif op == LOAD_CONST:
                        regs[sp] = consts[arg]
                        sp += 1
                    elif op == STORE_LOCAL:
                        sp -= 1
                        regs[arg] = regs[sp]
                    elif op == JUMP_IF_FALSE:
                        sp -= 1
                        a = regs[sp]
                        if a is None or a is False:
                            pc = arg
                    elif op == LT:
                        sp -= 1
                        a = regs[sp - 1]
                        b = regs[sp]
                        t = type(a)
                        if t is type(b) and (t is int or t is float or t is str):
                            regs[sp - 1] = a < b
                        else:
                            regs[sp - 1] = binary(_site("<", co, pc), a, b)
                    elif op == ADD:
                        sp -= 1
                        a = regs[sp - 1]
                        b = regs[sp]
                        t = type(a)
                        if t is type(b) and (t is int or t is str or t is float):
                            regs[sp - 1] = a + b
                        else:
                            regs[sp - 1] = binary(_site("+", co, pc), a, b)
                    elif op == SUB:
                        sp -= 1
                        a = regs[sp - 1]
                        b = regs[sp]
                        t = type(a)
                        if t is type(b) and (t is int or t is float):
                            regs[sp - 1] = a - b
                        else:
                            regs[sp - 1] = binary(_site("-", co, pc), a, b)
                    elif op == JUMP:
                        pc = arg
                    elif op == LOAD_GLOBAL:
                        try:
                            regs[sp] = g[consts[arg]]
                        except KeyError:
                            raise RuntimeErr("undefined variable '%s'" % consts[arg], co.line_at(pc - 2)) from None
                        sp += 1
                    elif op == CALL:
                        fn = regs[sp - arg - 1]
                        if type(fn) is Closure:
                            callee = fn.code
                            if arg != callee.nparams:
                                raise RuntimeErr("%s expects %d argument(s), got %d"
                                                 % (fn, callee.nparams, arg), co.line_at(pc - 2))
                            if len(frames) >= max_depth:
                                raise RuntimeErr("stack overflow (max call depth %d)" % max_depth,
                                                 co.line_at(pc - 2))
                            new_regs = regs[sp - arg:sp]
                            new_regs += callee.padding
                            frames.append((co, pc, regs, sp - arg - 1, upvals))
                            co, code, consts, upvals = callee, callee.code, callee.consts, fn.upvals
                            regs, sp, pc = new_regs, callee.nslots, 0
                        elif type(fn) is Builtin:
                            if fn.arity is not None and arg != fn.arity:
                                raise RuntimeErr("%s() expects %d argument(s), got %d"
                                                 % (fn.name, fn.arity, arg), co.line_at(pc - 2))
                            args = regs[sp - arg:sp]
                            sp -= arg
                            regs[sp - 1] = fn.fn(args, co.line_at(pc - 2))
                        else:
                            raise RuntimeErr("cannot call a %s" % type_name(fn), co.line_at(pc - 2))
                    elif op == RETURN:
                        a = regs[sp - 1]
                        if not frames:
                            raise minilang.ReturnSignal(a)   # top-level return escapes, as in stage 2
                        co, pc, regs, sp, upvals = frames.pop()
                        code, consts = co.code, co.consts
                        regs[sp] = a
                        sp += 1
                    elif op == POP:
                        sp -= 1
                    elif op == DUP:
                        regs[sp] = regs[sp - 1]
                        sp += 1
                    # -- superinstructions (--opt) --
                    elif op == JUMP_IF_NOT_LT:
                        sp -= 2
                        a = regs[sp]
                        b = regs[sp + 1]
                        t = type(a)
                        if t is type(b) and (t is int or t is float or t is str):
                            if not a < b:
                                pc = arg
                        elif not binary(_site("<", co, pc), a, b):
                            pc = arg
                    elif op == ADD_CONST:
                        a = regs[sp - 1]
                        b = consts[arg]
                        t = type(a)
                        if t is int or t is float:
                            regs[sp - 1] = a + b
                        else:
                            regs[sp - 1] = binary(_site("+", co, pc), a, b)
                    elif op == SUB_CONST:
                        a = regs[sp - 1]
                        b = consts[arg]
                        t = type(a)
                        if t is int or t is float:
                            regs[sp - 1] = a - b
                        else:
                            regs[sp - 1] = binary(_site("-", co, pc), a, b)
                    elif op == LT_CONST:
                        a = regs[sp - 1]
                        b = consts[arg]
                        t = type(a)
                        if t is int or t is float:
                            regs[sp - 1] = a < b
                        else:
                            regs[sp - 1] = binary(_site("<", co, pc), a, b)
                    # -- closures and names --
                    elif op == LOAD_CELL:
                        regs[sp] = regs[arg][0]
                        sp += 1
                    elif op == STORE_CELL:
                        sp -= 1
                        regs[arg][0] = regs[sp]
                    elif op == LOAD_UPVAL:
                        regs[sp] = upvals[arg][0]
                        sp += 1
                    elif op == STORE_UPVAL:
                        sp -= 1
                        upvals[arg][0] = regs[sp]
                    elif op == MAKE_CELL:
                        regs[arg] = [UNDEFINED]
                    elif op == CELL_ARG:
                        regs[arg] = [regs[arg]]
                    elif op == MAKE_CLOSURE:
                        callee = consts[arg]
                        regs[sp] = Closure(callee, tuple([regs[i] if local else upvals[i]
                                                          for local, i in callee.upval_specs]))
                        sp += 1
                    elif op == DEFINE_GLOBAL:
                        sp -= 1
                        g[consts[arg]] = regs[sp]
                    elif op == STORE_GLOBAL:
                        sp -= 1
                        name = consts[arg]
                        if name not in g:
                            raise RuntimeErr("assignment to undefined variable '%s'" % name, co.line_at(pc - 2))
                        g[name] = regs[sp]
                    elif op == LOAD_CHAIN:
                        name, maybe, kind, final = co.chains[arg]
                        for i in maybe:
                            a = upvals[i][0]
                            if a is not UNDEFINED:
                                break
                        else:
                            if kind == "upval":
                                a = upvals[final][0]
                            elif name in g:
                                a = g[name]
                            else:
                                raise RuntimeErr("undefined variable '%s'" % name, co.line_at(pc - 2))
                        regs[sp] = a
                        sp += 1
                    elif op == STORE_CHAIN:
                        sp -= 1
                        name, maybe, kind, final = co.chains[arg]
                        for i in maybe:
                            if upvals[i][0] is not UNDEFINED:
                                upvals[i][0] = regs[sp]
                                break
                        else:
                            if kind == "upval":
                                upvals[final][0] = regs[sp]
                            elif name in g:
                                g[name] = regs[sp]
                            else:
                                raise RuntimeErr("assignment to undefined variable '%s'" % name,
                                                 co.line_at(pc - 2))
                    # -- the remaining operators --
                    elif op in (EQ, NE):
                        sp -= 1
                        a = regs[sp - 1]
                        b = regs[sp]
                        t = type(a)
                        same = a == b if t is type(b) and (t is int or t is str) else values_equal(a, b)
                        regs[sp - 1] = same if op == EQ else not same
                    elif op == MOD:
                        sp -= 1
                        a = regs[sp - 1]
                        b = regs[sp]
                        if type(a) is int and type(b) is int and b:
                            regs[sp - 1] = a % b
                        else:
                            regs[sp - 1] = binary(_site("%", co, pc), a, b)
                    elif op == LE or op == GT or op == GE or op == MUL:
                        sp -= 1
                        a = regs[sp - 1]
                        b = regs[sp]
                        t = type(a)
                        if t is type(b) and (t is int or t is float or (t is str and op != MUL)):
                            regs[sp - 1] = FAST_OPERATORS[op](a, b)
                        else:
                            regs[sp - 1] = binary(_site(OPERATOR_TEXT[op], co, pc), a, b)
                    elif op == DIV:   # int / int gives an int when exact; leave that rule to the shared code
                        sp -= 1
                        regs[sp - 1] = binary(_site("/", co, pc), regs[sp - 1], regs[sp])
                    elif op == NOT:
                        a = regs[sp - 1]
                        regs[sp - 1] = a is None or a is False
                    elif op == NEG:
                        a = regs[sp - 1]
                        if type(a) is not int and type(a) is not float:
                            raise RuntimeErr("unary '-' expects a number, got %s" % type_name(a), co.line_at(pc - 2))
                        regs[sp - 1] = -a
                    elif op == JUMP_IF_FALSE_OR_POP:
                        a = regs[sp - 1]
                        if a is None or a is False:
                            pc = arg
                        else:
                            sp -= 1
                    elif op == JUMP_IF_TRUE_OR_POP:
                        a = regs[sp - 1]
                        if a is not None and a is not False:
                            pc = arg
                        else:
                            sp -= 1
                    elif op == INDEX:
                        sp -= 1
                        a = regs[sp - 1]
                        b = regs[sp]
                        t = type(a)
                        if type(b) is int and (t is list or t is str) and -len(a) <= b < len(a):
                            regs[sp - 1] = a[b]
                        else:
                            regs[sp - 1] = self.index(a, b, co.line_at(pc - 2))
                    elif op == SET_INDEX:
                        sp -= 2
                        a = regs[sp - 1]
                        b = regs[sp]
                        value = regs[sp + 1]
                        if type(a) is not list:
                            raise RuntimeErr("can only assign into a list, got %s" % type_name(a), co.line_at(pc - 2))
                        self.check_index(a, b, co.line_at(pc - 2))
                        a[int(b)] = value
                        regs[sp - 1] = value
                    elif op == BUILD_LIST:
                        items = regs[sp - arg:sp]
                        sp -= arg
                        regs[sp] = items
                        sp += 1
                    elif op == THROW:
                        a = regs[sp - 1]
                        raise RuntimeErr(stringify(a), co.line_at(pc - 2), value=a)
                    elif op == BREAK_OUT:
                        while True:   # find the nearest caller that is inside a loop
                            if not frames:
                                raise minilang.BreakSignal()
                            co, pc, regs, sp, upvals = frames.pop()
                            target = co.loop_exit_at(pc - 2)
                            if target is not None:
                                break
                        code, consts = co.code, co.consts
                        sp, pc = co.nslots, target
                    elif op == HALT:
                        return regs[sp - 1]
                    else:
                        raise RuntimeErr("unknown opcode %d" % op, co.line_at(pc - 2))
            except RuntimeErr as err:
                # Unwind: the innermost handler covering the faulting instruction,
                # first in this frame, then at each caller's CALL instruction.
                fault = pc - 2
                while True:
                    target = co.handler_at(fault)
                    if target is not None:
                        break
                    if not frames:
                        raise
                    co, pc, regs, sp, upvals = frames.pop()
                    fault = pc - 2
                code, consts = co.code, co.consts
                regs[co.nslots] = err.value   # statements start with an empty stack
                sp, pc = co.nslots + 1, target


OPERATOR_TEXT = {op: text for text, op in BINARY_OPCODES.items()}
FAST_OPERATORS = {LE: operator.le, GT: operator.gt, GE: operator.ge, MUL: operator.mul}

# ------------------------------------------------------------- benchmarks ----

BENCHMARKS = ("bench_fib.ml", "bench_loop.ml", "bench_concat.ml")
ENGINES = (
    ("tree-walker", lambda out: TreeWalker(output=out)),
    ("vm", lambda out: VM(output=out)),
) + tuple(
    ("vm +" + name, lambda out, name=name: VM(output=out, optimize=[name])) for name in OPTIMIZATIONS
) + (
    ("vm --opt", lambda out: VM(output=out, optimize=True)),
)


def bench(runs=5):
    """Run every benchmark `runs` times on every engine, interleaved, and print
    two Markdown tables of medians: engines against the tree-walker, and each
    optimization alone against the plain VM. Times include parse and compile."""
    print("Python %s (%s), %s, %s" % (platform.python_version(), platform.python_implementation(),
                                       platform.platform(), platform.processor() or platform.machine()))
    print("%d runs per engine; median seconds (min-max)" % runs)
    results = []
    for filename in BENCHMARKS:
        with open(os.path.join(HERE, "programs", filename), encoding="utf-8") as f:
            src = f.read()
        times = {name: [] for name, _ in ENGINES}
        outputs = set()
        for _ in range(runs):
            for name, make in ENGINES:
                out = []
                start = time.perf_counter()
                make(out.append).run(src)
                times[name].append(time.perf_counter() - start)
                outputs.add(tuple(out))
        if len(outputs) != 1:
            raise AssertionError("engines disagree on %s: %r" % (filename, outputs))
        results.append((filename[:-3], " ".join(outputs.pop()), times))

    def cell(t):
        return "%.3f (%.3f-%.3f)" % (statistics.median(t), min(t), max(t))

    print()
    print("| benchmark | output | tree-walker | vm | vm --opt | vm speedup | vm --opt speedup |")
    print("| --- | --- | --- | --- | --- | --- | --- |")
    for name, output, times in results:
        base = statistics.median(times["tree-walker"])
        print("| %s | %s | %s | %s | %s | %.2fx | %.2fx |" % (
            name, output, cell(times["tree-walker"]), cell(times["vm"]), cell(times["vm --opt"]),
            base / statistics.median(times["vm"]), base / statistics.median(times["vm --opt"])))
    print()
    print("| benchmark | vm | " + " | ".join("+" + n for n in OPTIMIZATIONS) + " | all (--opt) |")
    print("| --- | --- |" + " --- |" * (len(OPTIMIZATIONS) + 1))
    for name, _, times in results:
        plain = statistics.median(times["vm"])
        cells = ["%.3f (%.2fx)" % (statistics.median(times[e]), plain / statistics.median(times[e]))
                 for e in ["vm +" + n for n in OPTIMIZATIONS] + ["vm --opt"]]
        print("| %s | %.3f | %s |" % (name, plain, " | ".join(cells)))
    return 0


# ------------------------------------------------------------------ main ----


def run_file(path, optimize=False):
    with open(path, encoding="utf-8") as f:
        src = f.read()
    try:
        VM(optimize=optimize).run(src)
    except minilang.LangError as err:
        print("error: %s" % err, file=sys.stderr)
        return 1
    return 0


def repl(optimize=False):
    vm = VM(optimize=optimize)
    print("minilang VM REPL. Statements end with ';'. Ctrl-D to exit.")
    buffer = ""
    while True:
        try:
            line = input("... " if buffer else ">>> ")
        except EOFError:
            print()
            return 0
        buffer += line + "\n"
        try:
            result = vm.run(buffer)
        except minilang.SyntaxErr as err:
            if "end of input" in err.message or "found EOF" in err.message:
                continue  # wait for more lines
            print("syntax error: %s" % err)
        except minilang.LangError as err:
            print("error: %s" % err)
        else:
            if result is not None:
                print(stringify(result))
        buffer = ""


def main(argv):
    optimize = "--opt" in argv
    args = [a for a in argv if a != "--opt"]
    if args[:1] == ["--bench"]:
        return bench(int(args[1]) if len(args) > 1 else 5)
    if args[:1] == ["--dis"]:
        with open(args[1], encoding="utf-8") as f:
            print(disassemble(VM(optimize=optimize).compile(f.read())))
        return 0
    return run_file(args[0], optimize) if args else repl(optimize)


if __name__ == "__main__":
    sys.setrecursionlimit(20000)
    sys.exit(main(sys.argv[1:]))
