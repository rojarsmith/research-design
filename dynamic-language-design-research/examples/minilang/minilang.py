#!/usr/bin/env python3
"""minilang: a single-file tree-walking interpreter for a small dynamic language.

Pipeline: source -> tokenize() -> Parser (recursive descent + precedence
climbing) -> AST (Node) -> Interpreter (environment chain, closures,
exceptions). The host language's garbage collector manages memory, so this
file corresponds to stage 2 of the research roadmap.

Usage:
    python3 minilang.py program.ml     # run a file
    python3 minilang.py                # REPL
"""
import sys
import time

# ---------------------------------------------------------------- errors ----


class LangError(Exception):
    """Base class for errors reported to the user with a line number."""

    def __init__(self, message, line=None):
        super().__init__(message)
        self.message = message
        self.line = line

    def __str__(self):
        if self.line is None:
            return self.message
        return "[line %d] %s" % (self.line, self.message)


class SyntaxErr(LangError):
    pass


class RuntimeErr(LangError):
    """Runtime error. `value` carries the thrown value for `throw`."""

    def __init__(self, message, line=None, value=None):
        super().__init__(message, line)
        self.value = message if value is None else value


# ----------------------------------------------------------------- lexer ----

KEYWORDS = {
    "let", "fn", "if", "else", "while", "return", "break", "true", "false",
    "nil", "and", "or", "not", "throw", "try", "catch",
}
TWO_CHAR_OPS = {"==", "!=", "<=", ">="}
ONE_CHAR_OPS = set("+-*/%(){}[],;=<>")
ESCAPES = {"n": "\n", "t": "\t", '"': '"', "\\": "\\"}


class Token:
    __slots__ = ("kind", "value", "line")

    def __init__(self, kind, value, line):
        self.kind = kind      # NUMBER, STRING, IDENT, keyword text, operator text, EOF
        self.value = value
        self.line = line

    def __repr__(self):
        return "Token(%s, %r, line %d)" % (self.kind, self.value, self.line)


def tokenize(src):
    tokens = []
    i, n, line = 0, len(src), 1
    while i < n:
        c = src[i]
        if c == "\n":
            line += 1
            i += 1
        elif c.isspace():
            i += 1
        elif c == "#":
            while i < n and src[i] != "\n":
                i += 1
        elif c.isdigit():
            start = i
            while i < n and src[i].isdigit():
                i += 1
            if i + 1 < n and src[i] == "." and src[i + 1].isdigit():
                i += 1
                while i < n and src[i].isdigit():
                    i += 1
                tokens.append(Token("NUMBER", float(src[start:i]), line))
            else:
                tokens.append(Token("NUMBER", int(src[start:i]), line))
        elif c.isalpha() or c == "_":
            start = i
            while i < n and (src[i].isalnum() or src[i] == "_"):
                i += 1
            word = src[start:i]
            if word in KEYWORDS:
                tokens.append(Token(word, word, line))
            else:
                tokens.append(Token("IDENT", word, line))
        elif c == '"':
            i += 1
            chars = []
            start_line = line
            while True:
                if i >= n:
                    raise SyntaxErr("unterminated string", start_line)
                c = src[i]
                if c == '"':
                    i += 1
                    break
                if c == "\\":
                    i += 1
                    if i >= n or src[i] not in ESCAPES:
                        raise SyntaxErr("bad escape sequence", line)
                    chars.append(ESCAPES[src[i]])
                else:
                    if c == "\n":
                        line += 1
                    chars.append(c)
                i += 1
            tokens.append(Token("STRING", "".join(chars), start_line))
        elif src[i:i + 2] in TWO_CHAR_OPS:
            tokens.append(Token(src[i:i + 2], src[i:i + 2], line))
            i += 2
        elif c in ONE_CHAR_OPS:
            tokens.append(Token(c, c, line))
            i += 1
        else:
            raise SyntaxErr("unexpected character %r" % c, line)
    tokens.append(Token("EOF", None, line))
    return tokens


# ------------------------------------------------------------------- AST ----


class Node:
    """Generic AST node: a kind, a source line, and named fields."""

    def __init__(self, kind, line, **fields):
        self.kind = kind
        self.line = line
        self.__dict__.update(fields)

    def __repr__(self):
        fields = {k: v for k, v in self.__dict__.items() if k not in ("kind", "line")}
        return "%s(%s)" % (self.kind, ", ".join("%s=%r" % kv for kv in fields.items()))


# ---------------------------------------------------------------- parser ----


class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0

    # -- token helpers --
    def peek(self, offset=0):
        return self.tokens[self.pos + offset]

    def at_end(self):
        return self.peek().kind == "EOF"

    def check(self, kind):
        return self.peek().kind == kind

    def match(self, *kinds):
        if self.peek().kind in kinds:
            self.pos += 1
            return self.tokens[self.pos - 1]
        return None

    def expect(self, kind, what=None):
        tok = self.match(kind)
        if tok is None:
            got = self.peek()
            raise SyntaxErr("expected %s but found %s" % (what or repr(kind), got.kind), got.line)
        return tok

    # -- grammar --
    def parse_program(self):
        body = []
        while not self.at_end():
            body.append(self.statement())
        return Node("Program", 1, body=body)

    def statement(self):
        tok = self.peek()
        if self.match("let"):
            name = self.expect("IDENT", "a variable name").value
            self.expect("=")
            value = self.expression()
            self.expect(";")
            return Node("Let", tok.line, name=name, value=value)
        if self.check("fn") and self.peek(1).kind == "IDENT":
            self.match("fn")
            name = self.expect("IDENT").value
            params, body = self.function_tail()
            return Node("FnDecl", tok.line, name=name, params=params, body=body)
        if self.match("if"):
            return self.if_statement(tok.line)
        if self.match("while"):
            self.expect("(")
            cond = self.expression()
            self.expect(")")
            body = self.block()
            return Node("While", tok.line, cond=cond, body=body)
        if self.match("return"):
            value = None if self.check(";") else self.expression()
            self.expect(";")
            return Node("Return", tok.line, value=value)
        if self.match("break"):
            self.expect(";")
            return Node("Break", tok.line)
        if self.match("throw"):
            value = self.expression()
            self.expect(";")
            return Node("Throw", tok.line, value=value)
        if self.match("try"):
            body = self.block()
            self.expect("catch")
            self.expect("(")
            name = self.expect("IDENT", "a variable name").value
            self.expect(")")
            handler = self.block()
            return Node("Try", tok.line, body=body, name=name, handler=handler)
        if self.check("{"):
            return self.block()
        expr = self.expression()
        self.expect(";")
        return Node("ExprStmt", tok.line, expr=expr)

    def if_statement(self, line):
        self.expect("(")
        cond = self.expression()
        self.expect(")")
        then = self.block()
        otherwise = None
        if self.match("else"):
            if self.match("if"):
                otherwise = self.if_statement(self.peek(-1).line)
            else:
                otherwise = self.block()
        return Node("If", line, cond=cond, then=then, otherwise=otherwise)

    def block(self):
        start = self.expect("{", "'{'")
        body = []
        while not self.check("}"):
            if self.at_end():
                raise SyntaxErr("unexpected end of input, missing '}'", start.line)
            body.append(self.statement())
        self.expect("}")
        return Node("Block", start.line, body=body)

    def function_tail(self):
        self.expect("(")
        params = []
        if not self.check(")"):
            params.append(self.expect("IDENT", "a parameter name").value)
            while self.match(","):
                params.append(self.expect("IDENT", "a parameter name").value)
        self.expect(")")
        body = self.block()
        return params, body

    # expressions, lowest precedence first
    def expression(self):
        return self.assignment()

    def assignment(self):
        target = self.logic_or()
        if self.check("="):
            eq = self.match("=")
            value = self.assignment()
            if target.kind == "Var":
                return Node("Assign", eq.line, name=target.name, value=value)
            if target.kind == "Index":
                return Node("SetIndex", eq.line, obj=target.obj, index=target.index, value=value)
            raise SyntaxErr("invalid assignment target", eq.line)
        return target

    def logic_or(self):
        left = self.logic_and()
        while self.check("or"):
            op = self.match("or")
            left = Node("Logical", op.line, op="or", left=left, right=self.logic_and())
        return left

    def logic_and(self):
        left = self.equality()
        while self.check("and"):
            op = self.match("and")
            left = Node("Logical", op.line, op="and", left=left, right=self.equality())
        return left

    def binary_level(self, next_level, ops):
        left = next_level()
        while self.peek().kind in ops:
            op = self.match(*ops)
            left = Node("Binary", op.line, op=op.kind, left=left, right=next_level())
        return left

    def equality(self):
        return self.binary_level(self.comparison, ("==", "!="))

    def comparison(self):
        return self.binary_level(self.term, ("<", "<=", ">", ">="))

    def term(self):
        return self.binary_level(self.factor, ("+", "-"))

    def factor(self):
        return self.binary_level(self.unary, ("*", "/", "%"))

    def unary(self):
        op = self.match("-", "not")
        if op:
            return Node("Unary", op.line, op=op.kind, operand=self.unary())
        return self.postfix()

    def postfix(self):
        expr = self.primary()
        while True:
            if self.check("("):
                paren = self.match("(")
                args = []
                if not self.check(")"):
                    args.append(self.expression())
                    while self.match(","):
                        args.append(self.expression())
                self.expect(")")
                expr = Node("Call", paren.line, callee=expr, args=args)
            elif self.check("["):
                bracket = self.match("[")
                index = self.expression()
                self.expect("]")
                expr = Node("Index", bracket.line, obj=expr, index=index)
            else:
                return expr

    def primary(self):
        tok = self.peek()
        if self.match("NUMBER", "STRING"):
            return Node("Literal", tok.line, value=tok.value)
        if self.match("true"):
            return Node("Literal", tok.line, value=True)
        if self.match("false"):
            return Node("Literal", tok.line, value=False)
        if self.match("nil"):
            return Node("Literal", tok.line, value=None)
        if self.match("IDENT"):
            return Node("Var", tok.line, name=tok.value)
        if self.match("("):
            expr = self.expression()
            self.expect(")")
            return expr
        if self.match("["):
            items = []
            if not self.check("]"):
                items.append(self.expression())
                while self.match(","):
                    items.append(self.expression())
            self.expect("]")
            return Node("ListLit", tok.line, items=items)
        if self.match("fn"):
            params, body = self.function_tail()
            return Node("FnExpr", tok.line, params=params, body=body)
        raise SyntaxErr("unexpected token %s" % (tok.kind if tok.value is None else repr(tok.value)), tok.line)


def parse(src):
    return Parser(tokenize(src)).parse_program()


# ---------------------------------------------------------------- values ----


class Environment:
    __slots__ = ("vars", "parent")

    def __init__(self, parent=None):
        self.vars = {}
        self.parent = parent

    def define(self, name, value):
        self.vars[name] = value

    def lookup(self, name, line):
        env = self
        while env is not None:
            if name in env.vars:
                return env.vars[name]
            env = env.parent
        raise RuntimeErr("undefined variable '%s'" % name, line)

    def assign(self, name, value, line):
        env = self
        while env is not None:
            if name in env.vars:
                env.vars[name] = value
                return
            env = env.parent
        raise RuntimeErr("assignment to undefined variable '%s'" % name, line)


class Function:
    """A user function: parameters, body, and the environment it closed over."""

    def __init__(self, name, params, body, closure):
        self.name = name
        self.params = params
        self.body = body
        self.closure = closure

    def __repr__(self):
        return "<fn %s>" % self.name


class Builtin:
    def __init__(self, name, fn, arity=None):
        self.name = name
        self.fn = fn
        self.arity = arity

    def __repr__(self):
        return "<builtin %s>" % self.name


class ReturnSignal(Exception):
    def __init__(self, value):
        self.value = value


class BreakSignal(Exception):
    pass


def type_name(value):
    if value is None:
        return "nil"
    if isinstance(value, bool):
        return "bool"
    if isinstance(value, (int, float)):
        return "number"
    if isinstance(value, str):
        return "string"
    if isinstance(value, list):
        return "list"
    if isinstance(value, (Function, Builtin)):
        return "function"
    return type(value).__name__


def stringify(value):
    if value is None:
        return "nil"
    if value is True:
        return "true"
    if value is False:
        return "false"
    if isinstance(value, float) and value.is_integer():
        return str(int(value))
    if isinstance(value, list):
        return "[" + ", ".join(stringify(v) for v in value) + "]"
    return str(value)


def truthy(value):
    # Lua-style: only nil and false are falsy, so 0 and "" are truthy.
    return value is not None and value is not False


def is_number(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def values_equal(a, b):
    if isinstance(a, bool) or isinstance(b, bool):
        return type(a) is type(b) and a == b
    if isinstance(a, list) and isinstance(b, list):
        return a is b
    return a == b


# ----------------------------------------------------------- interpreter ----


class Interpreter:
    MAX_DEPTH = 400

    def __init__(self, output=None):
        self.output = output if output is not None else (lambda s: print(s))
        self.globals = Environment()
        self.depth = 0
        self._install_builtins()

    # -- builtins --
    def _install_builtins(self):
        def b_print(args, line):
            self.output(" ".join(stringify(a) for a in args))
            return None

        def b_len(args, line):
            (v,) = args
            if isinstance(v, (str, list)):
                return len(v)
            raise RuntimeErr("len() expects a string or list, got %s" % type_name(v), line)

        def b_push(args, line):
            lst, v = args
            if not isinstance(lst, list):
                raise RuntimeErr("push() expects a list", line)
            lst.append(v)
            return lst

        def b_pop(args, line):
            (lst,) = args
            if not isinstance(lst, list) or not lst:
                raise RuntimeErr("pop() expects a non-empty list", line)
            return lst.pop()

        def b_str(args, line):
            return stringify(args[0])

        def b_num(args, line):
            (v,) = args
            try:
                return int(v) if isinstance(v, str) and v.strip().lstrip("-").isdigit() else float(v)
            except (TypeError, ValueError):
                raise RuntimeErr("num() cannot convert %r" % (v,), line)

        def b_type(args, line):
            return type_name(args[0])

        def b_clock(args, line):
            return time.time()

        for name, fn, arity in (
            ("print", b_print, None), ("len", b_len, 1), ("push", b_push, 2),
            ("pop", b_pop, 1), ("str", b_str, 1), ("num", b_num, 1),
            ("type", b_type, 1), ("clock", b_clock, 0),
        ):
            self.globals.define(name, Builtin(name, fn, arity))

    # -- entry points --
    def run(self, src):
        """Parse and execute a program. Returns the value of a trailing expression statement."""
        program = parse(src)
        result = None
        # Each minilang call uses several host frames; make sure the host limit
        # is above MAX_DEPTH so the language-level check fires first.
        old_limit = sys.getrecursionlimit()
        sys.setrecursionlimit(max(old_limit, self.MAX_DEPTH * 12 + 500))
        try:
            for stmt in program.body:
                result = self.exec(stmt, self.globals)
        finally:
            sys.setrecursionlimit(old_limit)
        return result

    # -- statements --
    def exec(self, node, env):
        kind = node.kind
        if kind == "ExprStmt":
            return self.eval(node.expr, env)
        if kind == "Let":
            env.define(node.name, self.eval(node.value, env))
        elif kind == "FnDecl":
            env.define(node.name, Function(node.name, node.params, node.body, env))
        elif kind == "Block":
            self.exec_block(node.body, Environment(env))
        elif kind == "If":
            if truthy(self.eval(node.cond, env)):
                self.exec(node.then, env)
            elif node.otherwise is not None:
                self.exec(node.otherwise, env)
        elif kind == "While":
            try:
                while truthy(self.eval(node.cond, env)):
                    self.exec(node.body, env)
            except BreakSignal:
                pass
        elif kind == "Return":
            raise ReturnSignal(None if node.value is None else self.eval(node.value, env))
        elif kind == "Break":
            raise BreakSignal()
        elif kind == "Throw":
            value = self.eval(node.value, env)
            raise RuntimeErr(stringify(value), node.line, value=value)
        elif kind == "Try":
            try:
                self.exec(node.body, env)
            except RuntimeErr as err:
                handler_env = Environment(env)
                handler_env.define(node.name, err.value)
                self.exec_block(node.handler.body, handler_env)
        else:
            raise RuntimeErr("unknown statement %s" % kind, node.line)
        return None

    def exec_block(self, statements, env):
        for stmt in statements:
            self.exec(stmt, env)

    # -- expressions --
    def eval(self, node, env):
        kind = node.kind
        if kind == "Literal":
            return node.value
        if kind == "Var":
            return env.lookup(node.name, node.line)
        if kind == "Assign":
            value = self.eval(node.value, env)
            env.assign(node.name, value, node.line)
            return value
        if kind == "ListLit":
            return [self.eval(item, env) for item in node.items]
        if kind == "FnExpr":
            return Function("<anonymous>", node.params, node.body, env)
        if kind == "Logical":
            left = self.eval(node.left, env)
            if node.op == "or":
                return left if truthy(left) else self.eval(node.right, env)
            return self.eval(node.right, env) if truthy(left) else left
        if kind == "Unary":
            operand = self.eval(node.operand, env)
            if node.op == "not":
                return not truthy(operand)
            if not is_number(operand):
                raise RuntimeErr("unary '-' expects a number, got %s" % type_name(operand), node.line)
            return -operand
        if kind == "Binary":
            return self.binary(node, self.eval(node.left, env), self.eval(node.right, env))
        if kind == "Index":
            obj = self.eval(node.obj, env)
            index = self.eval(node.index, env)
            return self.index(obj, index, node.line)
        if kind == "SetIndex":
            obj = self.eval(node.obj, env)
            index = self.eval(node.index, env)
            value = self.eval(node.value, env)
            if not isinstance(obj, list):
                raise RuntimeErr("can only assign into a list, got %s" % type_name(obj), node.line)
            self.check_index(obj, index, node.line)
            obj[int(index)] = value
            return value
        if kind == "Call":
            callee = self.eval(node.callee, env)
            args = [self.eval(a, env) for a in node.args]
            return self.call(callee, args, node.line)
        raise RuntimeErr("unknown expression %s" % kind, node.line)

    def binary(self, node, left, right):
        op = node.op
        if op == "==":
            return values_equal(left, right)
        if op == "!=":
            return not values_equal(left, right)
        if op == "+":
            if is_number(left) and is_number(right):
                return left + right
            if isinstance(left, str) and isinstance(right, str):
                return left + right
            if isinstance(left, list) and isinstance(right, list):
                return left + right
            raise RuntimeErr("'+' expects two numbers, two strings or two lists, got %s and %s"
                             % (type_name(left), type_name(right)), node.line)
        if op in ("<", "<=", ">", ">="):
            same_kind = (is_number(left) and is_number(right)) or (isinstance(left, str) and isinstance(right, str))
            if not same_kind:
                raise RuntimeErr("'%s' expects two numbers or two strings, got %s and %s"
                                 % (op, type_name(left), type_name(right)), node.line)
            return {"<": left < right, "<=": left <= right, ">": left > right, ">=": left >= right}[op]
        if not (is_number(left) and is_number(right)):
            raise RuntimeErr("'%s' expects two numbers, got %s and %s" % (op, type_name(left), type_name(right)), node.line)
        if op == "-":
            return left - right
        if op == "*":
            return left * right
        if op in ("/", "%"):
            if right == 0:
                raise RuntimeErr("division by zero", node.line)
            if op == "/":
                result = left / right
                return int(result) if isinstance(left, int) and isinstance(right, int) and result.is_integer() else result
            return left % right
        raise RuntimeErr("unknown operator %s" % op, node.line)

    def check_index(self, obj, index, line):
        if not is_number(index) or int(index) != index:
            raise RuntimeErr("index must be an integer, got %s" % type_name(index), line)
        if not -len(obj) <= int(index) < len(obj):
            raise RuntimeErr("index %d out of range for length %d" % (int(index), len(obj)), line)

    def index(self, obj, index, line):
        if isinstance(obj, (list, str)):
            self.check_index(obj, index, line)
            return obj[int(index)]
        raise RuntimeErr("cannot index a %s" % type_name(obj), line)

    def call(self, callee, args, line):
        if isinstance(callee, Builtin):
            if callee.arity is not None and len(args) != callee.arity:
                raise RuntimeErr("%s() expects %d argument(s), got %d" % (callee.name, callee.arity, len(args)), line)
            return callee.fn(args, line)
        if not isinstance(callee, Function):
            raise RuntimeErr("cannot call a %s" % type_name(callee), line)
        if len(args) != len(callee.params):
            raise RuntimeErr("%s expects %d argument(s), got %d" % (callee, len(callee.params), len(args)), line)
        if self.depth >= self.MAX_DEPTH:
            raise RuntimeErr("stack overflow (max call depth %d)" % self.MAX_DEPTH, line)
        env = Environment(callee.closure)
        for name, value in zip(callee.params, args):
            env.define(name, value)
        self.depth += 1
        try:
            self.exec_block(callee.body.body, env)
        except ReturnSignal as ret:
            return ret.value
        except RecursionError:
            # Fallback if the host stack runs out before MAX_DEPTH.
            raise RuntimeErr("stack overflow (host recursion limit)", line)
        finally:
            self.depth -= 1
        return None


# ------------------------------------------------------------------ main ----


def run_file(path):
    with open(path, encoding="utf-8") as f:
        src = f.read()
    interp = Interpreter()
    try:
        interp.run(src)
    except LangError as err:
        print("error: %s" % err, file=sys.stderr)
        return 1
    return 0


def repl():
    interp = Interpreter()
    print("minilang REPL. Statements end with ';'. Ctrl-D to exit.")
    buffer = ""
    while True:
        try:
            line = input("... " if buffer else ">>> ")
        except EOFError:
            print()
            return 0
        buffer += line + "\n"
        try:
            result = interp.run(buffer)
        except SyntaxErr as err:
            if "end of input" in err.message or "found EOF" in err.message:
                continue  # wait for more lines
            print("syntax error: %s" % err)
        except LangError as err:
            print("error: %s" % err)
        else:
            if result is not None:
                print(stringify(result))
        buffer = ""


if __name__ == "__main__":
    sys.setrecursionlimit(20000)
    sys.exit(run_file(sys.argv[1]) if len(sys.argv) > 1 else repl())
