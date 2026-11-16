# minilang 1.0 language specification

[README](README.md) | [Traditional Chinese](SPEC.zh-TW.md) | [Roadmap (zh-TW)](../../research/03-design-your-own-dynamic-language.zh-TW.md)

Version 1.0, 2026-11-16. This document describes the language minilang, not an implementation. It was written by reading the three reference engines and checking every rule against them with the conformance suite in [tests/](tests/); where the engines did something by accident, the rule says so and names the decision in [Annex A](#annex-a-decision-register). Implementation notes are marked as such, in the manner of the Python language reference.

Contents: 1 Scope and terms, 2 Lexical structure, 3 Syntax, 4 Values and types, 5 Truthiness, equality and ordering, 6 Expressions, 7 Scoping and closures, 8 Control flow, 9 Functions and calls, 10 Errors and exceptions, 11 Standard functions, 12 Numbers, 13 Strings and text, 14 Limits, 15 Conformance; Annex A decision register, Annex B implementation-defined behaviour of the reference engines, Annex C proposed changes, Annex D versioning, change process and release gate.

## 1 Scope and terms

### 1.1 Scope

minilang is a small dynamically typed language with numbers, strings, booleans, `nil`, lists, first-class functions and closures, lexical blocks, `while` loops and exceptions. A program is one source file; it runs from top to bottom and its only output is the text written by `print`. There are no modules, no input and no other I/O.

The reference engines are the tree-walking interpreter `examples/minilang/minilang.py` (**tree**), the Python bytecode VM `examples/minilang-vm/minilang_vm.py` (**vm**) and the C VM `examples/minilang-c/mlvm.c` driven by `minilang_c.py` (**c**), at repository commit `518b23b`.

### 1.2 How each rule is classified

| Term | Meaning |
| --- | --- |
| **Specified** | Normative. A conforming implementation behaves exactly so. All three reference engines do. |
| **Implementation-defined** | Each implementation chooses and documents its behaviour, within the bounds given here. Annex B documents the reference engines. |
| **Legacy** | What the reference engines do today, kept for 1.0 but marked for a **proposed change** (Annex C). Required at the Full conformance level, unspecified at the Core level, so a Core implementation may already follow the proposal. Modelled on ECMAScript's Annex B. |

"Must" states a requirement on implementations; "is an error" means the condition raises the error named, never undefined behaviour.

### 1.3 Ways a run ends

A run ends in exactly one of four ways (section 10.1): it **completes**; it stops at a **syntax error** before any statement runs; it stops at an **uncaught runtime error** (a runtime error or a thrown value that no `try` caught); or it stops at a **fatal error**, a condition that ends the program and that `try` cannot catch. Fatal errors exist in 1.0 only where the reference engines let a host exception escape; section 10.7 lists every one, and proposal P5 removes most of them.

### 1.4 Notation

Grammar rules use EBNF: `|` alternatives, `( )` grouping, `?` optional, `*` zero or more, `"x"` a literal token. `T` in a message stands for the `type()` name of a value (section 4.1). Section numbers are cited by the tests as `spec: 7.4`.

## 2 Lexical structure

### 2.1 Source text

A program is a sequence of Unicode code points read from a UTF-8 file. Lines end with LF; a CR is white space. Lines are numbered from 1, and every line counts, including comment lines, blank lines and lines inside a string literal. A byte order mark is not white space: a file that starts with one is a syntax error `unexpected character '﻿'`.

### 2.2 White space and comments

Space, tab, CR, form feed and vertical tab separate tokens and are otherwise ignored. `#` outside a string starts a comment that runs to the end of the line. Whether other Unicode white space (U+00A0, U+3000) is white space is implementation-defined.

### 2.3 Identifiers and keywords

```text
IDENT    := ( letter | "_" ) ( letter | digit | "_" )*      -- not a keyword
keywords := let fn if else while return break true false nil and or not throw try catch
```

`letter` is `A`-`Z` and `a`-`z`, `digit` is `0`-`9`. Identifiers are case-sensitive. Whether non-ASCII letters and digits may appear in identifiers is implementation-defined.

### 2.4 Number literals

```text
NUMBER := digit+ ( "." digit+ )?
```

Without a fraction the literal is an integer (leading zeros allowed: `007` is 7); with one it is a float, the binary64 value nearest to the decimal. There is no sign (`-1` is unary minus applied to `1`), no exponent (`1e5` is the number `1` followed by the name `e5`), no hexadecimal form, and a decimal point needs digits on both sides (`1.` and `.5` are syntax errors `unexpected character '.'`). Integer literals have no size limit of their own. Implementation-defined in 1.0, with proposal P7: non-ASCII digit characters in literals, and integer literals of more than 4,300 digits.

### 2.5 String literals

```text
STRING := '"' ( char | escape )* '"'
escape := "\n" | "\t" | "\"" | "\\"
```

Any other character after a backslash is the syntax error `bad escape sequence`, reported at the line of the backslash; there is no `\u`, `\r` or `\0`. A string may contain raw newlines and tabs, which are kept. A string not closed before the end of the file is the syntax error `unterminated string`, reported at the line where it starts.

### 2.6 Operators and punctuation

```text
==  !=  <=  >=  +  -  *  /  %  (  )  {  }  [  ]  ,  ;  =  <  >
```

Two-character operators are matched first. Any character that starts no token is the syntax error `unexpected character C`, where C is the character as Python's `repr()` writes it (`'@'`, `'!'`). Logical operators are the keywords `and`, `or`, `not`.

## 3 Syntax

### 3.1 Grammar

```text
program    := statement* EOF
statement  := "let" IDENT "=" expression ";"
            | "fn" IDENT "(" params? ")" block
            | "if" "(" expression ")" block ( "else" ( "if" ifrest | block ) )?
            | "while" "(" expression ")" block
            | "return" expression? ";"
            | "break" ";"
            | "throw" expression ";"
            | "try" block "catch" "(" IDENT ")" block
            | block
            | expression ";"
ifrest     := "(" expression ")" block ( "else" ( "if" ifrest | block ) )?
block      := "{" statement* "}"
params     := IDENT ( "," IDENT )*
expression := assignment
assignment := logicOr ( "=" assignment )?          -- target rules in 3.4
logicOr    := logicAnd ( "or" logicAnd )*
logicAnd   := equality ( "and" equality )*
equality   := comparison ( ( "==" | "!=" ) comparison )*
comparison := term ( ( "<" | "<=" | ">" | ">=" ) term )*
term       := factor ( ( "+" | "-" ) factor )*
factor     := unary ( ( "*" | "/" | "%" ) unary )*
unary      := ( "-" | "not" ) unary | postfix
postfix    := primary ( "(" args? ")" | "[" expression "]" )*
args       := expression ( "," expression )*
primary    := NUMBER | STRING | "true" | "false" | "nil" | IDENT
            | "(" expression ")" | "[" args? "]" | "fn" "(" params? ")" block
```

A statement that starts with `fn` followed by an identifier is a function declaration; `fn` followed by `(` starts an expression statement.

### 3.2 Statements

There is no empty statement (`;` alone is `unexpected token ';'`), `let` always has an initializer, `if` and `while` bodies are always blocks, `catch` names exactly one variable, and there is no trailing comma in lists, arguments or parameters.

### 3.3 Precedence

From lowest to highest: assignment (right-associative); `or`; `and`; `==` `!=`; `<` `<=` `>` `>=`; `+` `-`; `*` `/` `%`; unary `-` and `not`; calls and indexing. All binary operators are left-associative, so comparisons do not chain: `1 < 2 < 3` compares the boolean `true` with `3` and is a runtime error (section 5.3). `not 1 == 2` is `(not 1) == 2`.

### 3.4 Assignment targets

The left side of `=` must be a name or an index expression, after parentheses are removed: `(a) = 1` assigns `a`, and `f()[0] = 1` assigns into the list `f()` returns. Anything else is the syntax error `invalid assignment target`, reported at the line of `=`.

### 3.5 Syntax errors

The whole file is tokenized and parsed before any statement runs, so a program with a syntax error anywhere, even in code that would never run, prints nothing. A syntax error is reported with a line and one of these messages:

| Message | Line |
| --- | --- |
| `unexpected character C` (C as Python's `repr()`) | the character |
| `unterminated string` | the opening quote |
| `bad escape sequence` | the backslash |
| `expected X but found K` | the token found |
| `unexpected token V` | the token found |
| `unexpected end of input, missing '}'` | the unclosed `{` |
| `invalid assignment target` | the `=` |

X is `a variable name`, `a parameter name`, `'{'` or the expected token quoted (`';'`, `')'`, `'='`); K is the found token's kind: the operator or keyword itself, or `IDENT`, `NUMBER`, `STRING`, `EOF`. V is the token's value as Python's `repr()` writes it (`';'`, `'else'`, `5`, `'abc'`), or `EOF`. The message texts are required at the Full level only (section 15.1).

## 4 Values and types

### 4.1 Types

| `type()` | Values | Notes |
| --- | --- | --- |
| `nil` | `nil` | |
| `bool` | `true`, `false` | Not numbers: `true + 1` is an error and `true == 1` is false |
| `number` | integers and floats | Two representations with one type name; section 12 |
| `string` | immutable sequences of Unicode code points | Section 13 |
| `list` | mutable sequences of values | Reference semantics |
| `function` | user functions and standard functions | Section 9 |

### 4.2 Lists

A list value is a reference: assignment, argument passing and `push` share the list, and index assignment and `push`/`pop` change it for every holder. `a + b` on two lists makes a new list. A list may contain itself.

### 4.3 Functions

Functions are values: they can be stored, passed, returned and compared (by identity). Each evaluation of a `fn` expression or declaration makes a new function.

## 5 Truthiness, equality and ordering

### 5.1 Truthiness

`nil` and `false` are false; every other value, including `0`, `0.0`, `""` and `[]`, is true. `not v` is `true` when v is false, else `false`.

### 5.2 Equality

`==` and `!=` never fail. Two values are equal when:

- both are booleans with the same value (a boolean never equals a number);
- both are numbers with the same mathematical value; integers and floats compare exactly, so `1 == 1.0` but `9007199254740993 != 9007199254740992.0`; NaN equals nothing, itself included;
- both are strings with the same code points;
- both are `nil`;
- both are the same list or the same function (identity, never contents).

Values of different types are never equal.

### 5.3 Ordering

`<`, `<=`, `>`, `>=` take two numbers (compared exactly, as in 5.2; any comparison with NaN is false) or two strings (compared code point by code point, a prefix before a longer string). Any other pair is the runtime error `'OP' expects two numbers or two strings, got T1 and T2`.

## 6 Expressions

### 6.1 Evaluation order

Operands are evaluated left to right and completely before an operator checks their types: a binary operator evaluates both sides, a call evaluates the callee and then each argument, an index read evaluates the object and then the index, an index assignment evaluates the object, the index and the value. Only then are the operation's checks made, so `5(print("a"))` prints `a` before failing, and `y = print("side")` prints before failing on the undeclared `y`.

### 6.2 Logical operators

`a and b` is `a` if `a` is false, else `b`; `a or b` is `a` if `a` is true, else `b`. Both return an operand, not a boolean, and evaluate `b` only when needed.

### 6.3 Arithmetic

| Operator | Operands | Result | Wrong operands |
| --- | --- | --- | --- |
| `+` | two numbers; two strings; two lists | sum; concatenation; new list | `'+' expects two numbers, two strings or two lists, got T1 and T2` |
| `-` `*` | two numbers | difference, product | `'OP' expects two numbers, got T1 and T2` |
| `/` `%` | two numbers | section 12.3, 12.4 | the same; a zero divisor is `division by zero` |
| unary `-` | a number | negation | `unary '-' expects a number, got T` |

There is no string repetition, no implicit conversion and no exponent operator.

### 6.4 Indexing

`x[i]` reads element `i` of a list or code point `i` of a string (a one-code-point string). `i` must be a number with an integral value; an integral float such as `1.0` is accepted. Negative indexes count from the end (`-1` is the last). Errors: `index must be an integer, got T` (a non-number or non-integral number), `index I out of range for length N` (I is the index's integer value, of any size), `cannot index a T`. Index assignment `x[i] = v` needs a list (`can only assign into a list, got T`), checks the index the same way, stores `v` and has the value `v`. Legacy (P5): an infinite or NaN float index is a fatal error.

### 6.5 Assignment

`name = v` stores into the nearest existing binding of `name` (section 7.3) and has the value `v`. It never declares: see 7.6.

## 7 Scoping and closures

### 7.1 Declarations

`let name = e;` evaluates `e`, then binds `name` in the current scope. Declaring a name that already exists in the same scope replaces its value; it is not an error. `fn name(...) {...}` binds `name` in the current scope when the statement runs; declarations are not hoisted, so calling a function before its declaration has run is `undefined variable`. A function's parameters and the top level of its body share one scope, so `let a = ...` in the body replaces the parameter `a`.

### 7.2 Scopes

The program's top level is the global scope. Each block (`{ }`, including the bodies of `if`, `while`, `try`, `catch` and functions) runs in a new scope nested in the scope where it appears; a loop body gets a new scope on every iteration. The `catch` variable is bound in the handler's scope. A scope's bindings are invisible after it ends, except through closures.

### 7.3 Name resolution

A name is resolved each time it is used, by searching the scope where the use appears, then each enclosing scope in turn, out to the global scope. A function body's enclosing scopes are those of the place where the function was created. Because resolution happens at use, a function may refer to a global or to a local function declared after it, as long as the declaration has run before the call; mutually recursive local functions and `let f = fn() { ... f() ... }` work.

### 7.4 Bindings created after a closure (legacy, P2)

The search in 7.3 sees bindings that exist **at the moment of use**. When a `let` in an enclosing scope runs after a closure was created, later uses of that name in the closure find the new binding even though earlier ones found an outer one:

```text
let x = "outer";
fn test() {
  fn show() { return x; }
  let a = show();          # "outer": test's scope has no x yet
  let x = "inner";
  print(a, show());        # outer inner
}
```

Assignments follow the same rule. This is how all three engines behave and is required at the Full level; proposal P2 would make the binding static instead.

### 7.5 Closures

A function keeps the scopes where it was created alive and refers to their variables, not copies of their values: closures that share a variable see each other's assignments, and two calls of a factory create independent variables.

### 7.6 Undefined names

Reading a name with no binding is the runtime error `undefined variable 'NAME'`. Assigning to one is `assignment to undefined variable 'NAME'`; there are no implicit globals.

## 8 Control flow

### 8.1 if

The condition is evaluated once; the first block runs when it is true (section 5.1), else the `else` part, if any. `else if` chains are nested `if` statements.

### 8.2 while

The condition is evaluated before every iteration, and the body runs while it is true.

### 8.3 break

`break` ends the innermost loop that is running. Legacy (P3): that loop does not have to be in the same function. A `break` executed in a function that is not inside a loop of its own ends the innermost loop of its caller, across any number of calls, including a loop whose condition called the function, and `try` does not stop it. A `break` when no loop is running anywhere in the call chain is a fatal error.

### 8.4 return

`return e;` ends the current function call with the value `e`; `return;` returns `nil`. It leaves any loops and `try` blocks in the function. Legacy (P4): `return` at the top level, outside any function, is a fatal error; `try` does not catch it.

## 9 Functions and calls

### 9.1 Function values

`fn name(p1, ..., pn) { body }` and `fn(p1, ..., pn) { body }` create a function with exactly n parameters. A function's text (section 13.3) is `<fn name>`, or `<fn <anonymous>>` for a function made by a `fn` expression, even after it is stored in a variable. A standard function's text is `<builtin name>`.

### 9.2 Calls

`f(a1, ..., am)` evaluates `f` and the arguments (6.1), then: if `f` is not a function, the runtime error `cannot call a T`; if m differs from n, `<fn name> expects N argument(s), got M` for a user function or `name() expects N argument(s), got M` for a standard function (`print` takes any number). Otherwise the body runs in a new scope where the parameters are bound to the arguments. Each user function call adds one frame; section 14.1 limits frames.

### 9.3 Return values

A call's value is the value of the `return` that ended it, or `nil` if the body ended without one.

## 10 Errors and exceptions

### 10.1 Kinds of error

| Kind | When | Can `try` catch it | Reported |
| --- | --- | --- | --- |
| Syntax error | Before any statement runs (3.5) | No | Line and message |
| Runtime error | An operation fails (sections 6, 7, 9, 11, 14) | Yes, as its message string | Line and message |
| Thrown value | `throw e;` | Yes, as the value | Line and the value's text |
| Fatal error | Section 10.7 | No | Implementation-defined |

A runtime error in code that never runs never happens.

### 10.2 throw and try/catch

`throw e;` evaluates `e` and raises it. `try { A } catch (name) { B }` runs `A`; if a runtime error or thrown value is raised in `A`, including inside functions it calls (the calls are unwound), `B` runs with `name` bound to the caught value. Handlers nest: the innermost `try` around the point of the error catches it. An error raised inside `B` goes to the next enclosing `try`. There is no `finally`. Unwinding restores the call depth, so a program can recurse to the full depth again after catching a stack overflow.

### 10.3 Runtime errors as values

A caught runtime error is the message string, without the line. The messages are:

| Condition | Message |
| --- | --- |
| Undefined name | `undefined variable 'NAME'` |
| Assignment to an undefined name | `assignment to undefined variable 'NAME'` |
| `+` on other operands | `'+' expects two numbers, two strings or two lists, got T1 and T2` |
| `-` `*` `/` `%` on non-numbers | `'OP' expects two numbers, got T1 and T2` |
| Ordering other operands | `'OP' expects two numbers or two strings, got T1 and T2` |
| Unary minus | `unary '-' expects a number, got T` |
| Zero divisor | `division by zero` |
| Index not integral | `index must be an integer, got T` |
| Index out of range | `index I out of range for length N` |
| Indexing a non-list, non-string | `cannot index a T` |
| Index assignment into a non-list | `can only assign into a list, got T` |
| Calling a non-function | `cannot call a T` |
| Wrong argument count | `<fn NAME> expects N argument(s), got M`; `NAME() expects N argument(s), got M` |
| Call depth exceeded | `stack overflow (max call depth 400)` |
| Standard functions | section 11 |

The exact texts are required at the Full level. At the Core level a caught runtime error must be a string, and its text is implementation-defined.

### 10.4 Error lines

| Error | Line reported |
| --- | --- |
| Binary or unary operator | the operator token |
| Call (not a function, argument count, stack overflow, any error raised by a standard function) | the call's `(` |
| Index read | the `[` |
| Index assignment, assignment to an undefined name | the `=` |
| Undefined name | the name |
| `throw` | the `throw` keyword |

An error inside a function is reported where it happens, not at the call.

### 10.5 Uncaught errors

An uncaught runtime error ends the run with its line and message. An uncaught thrown value ends it with the line of the `throw` and, as the message, the value's text (section 13.3): `throw [1, "a", nil];` reports `[1, a, nil]`. The text is computed when the value is thrown, so throwing a value that has no text (a list that contains itself) is a fatal error at the `throw`, even inside `try` (legacy, P5).

### 10.6 throw nil (legacy, P1)

`throw nil;` is caught as the string `"nil"`, not as `nil`, and uncaught it reports the message `nil`. Required at the Full level; proposal P1 would bind `nil`.

### 10.7 Fatal errors

These conditions end the program at once; `try` does not catch them, statements before them have run and printed, and the report is implementation-defined. Every entry is legacy and covered by a proposal.

| Condition | Section | Proposal |
| --- | --- | --- |
| `return` at the top level | 8.4 | P4 |
| `break` with no running loop | 8.3 | P3 |
| An integer too large for a float is converted to one: an arithmetic operator (`+ - * / %`) with a float operand, or `num()` (comparisons never convert) | 12.3 | P5 |
| An integer quotient too large for a float | 12.3 | P5 |
| Converting an integer of more than 4,300 digits to text (`print`, `str`, a thrown value, an index error message) | 14.2 | P5 |
| An infinite or NaN float used as an index | 6.4 | P5 |
| Throwing a value whose text cannot be made | 10.5 | P5 |
| Converting to text a list that contains itself, or a list nested deeper than the implementation's limit, outside any function (inside a function: Annex B) | 14.3 | P5 |

Implementation note: in the reference engines these are Python exceptions (`ReturnSignal`, `BreakSignal`, `OverflowError`, `ValueError`, `RecursionError`) that escape the interpreter; the C VM reproduces the same type names in its report.

## 11 Standard functions

The standard functions are `print`, `len`, `push`, `pop`, `str`, `num`, `type` and `clock`. Errors they raise are runtime errors at the line of the call's `(`.

### 11.1 print(v1, ..., vn)

Writes the texts (section 13.3) of its arguments separated by one space, then a line end; with no arguments, an empty line. Returns `nil`.

### 11.2 len(v)

The number of code points of a string or elements of a list; otherwise `len() expects a string or list, got T`.

### 11.3 push(list, v)

Appends `v` and returns the same list; `push() expects a list` if the first argument is not one.

### 11.4 pop(list)

Removes and returns the last element; `pop() expects a non-empty list` for an empty list or a non-list.

### 11.5 str(v)

The text of `v` (section 13.3); `str(s)` of a string is that string.

### 11.6 num(v)

- A string: converted by the grammar of section 12.6.
- A number: the float with the same value (`num(3)` is `3.0`, which prints as `3`). Legacy (P5): an integer too large for a float is a fatal error.
- Legacy (P6): `true` gives `1.0` and `false` gives `0.0`.
- Anything else, or a string the grammar rejects, is the runtime error `num() cannot convert R`. Legacy (P6): R is the value as Python's `repr()` writes it: `None`, `True`, `'abc'`, `"it's"`, `[None, 1, 'a']`, `<fn f>`.

### 11.7 type(v)

The type name of section 4.1.

### 11.8 clock()

A float number of seconds, for measuring elapsed time. The epoch, the resolution and whether the clock can be set back are implementation-defined (all reference engines read the wall clock in seconds since the Unix epoch, as Python's `time.time()`); the suite checks only that the value is a positive number and that a second call is not smaller.

### 11.9 Standard functions are global variables

The standard functions are bindings in the global scope like any other: they can be reassigned (`print = fn(x) {...};`) or shadowed (`let len = 5;` in a block), and the change affects every later use that resolves to that binding.

## 12 Numbers

### 12.1 Integers

Integers have no size limit; arithmetic on integers is exact (`9223372036854775807 + 1` is `9223372036854775808`). The only limit is on conversion to and from text (section 14.2).

### 12.2 Floats

Floats are IEEE 754 binary64 with round-to-nearest-even. Overflow gives an infinity and invalid operations give NaN, without an error: a loop that multiplies `1.0` by 10 four hundred times reaches `inf`, and `inf - inf` is `nan`. Floats come from literals with a fraction, from `num()`, from `/`, and from any operation with a float operand.

### 12.3 Mixed arithmetic and division

- `+ - *` on two integers give an integer; with a float operand the integer is converted to the nearest float and the result is a float.
- `a / b` with `b` equal to zero (integer `0`, `0.0` or `-0.0`) is `division by zero`, for floats too.
- `a / b` on two integers: the exact quotient rounded to the nearest float; if that float is a whole number, the result is the integer with the float's value, else the float. So `6 / 3` is the integer `2` and `7 / 2` is `3.5`. Legacy (P9): because the rounding comes first, a whole quotient beyond 2^53 is not exact: `9007199254740993 / 1` is `9007199254740992`. A quotient too large for a float is a fatal error (legacy, P5).
- Comparisons (`== != < <= > >=`) between integers and floats are exact and never overflow.
- Legacy (P5): any other operation that must convert an integer too large for a float is a fatal error.

### 12.4 Remainder

`a % b` has the sign of `b` (floored division): `-5 % 3` is `1`, `5 % -3` is `-1`, `-5.5 % 2` is `0.5`. Integers give an integer, otherwise a float. A zero `b` is `division by zero`.

### 12.5 Number text

The text of a number, used by `print`, `str` and error messages:

- An integer: its decimal digits, with `-` for negatives.
- A float that is a whole number, including `-0.0`: the exact integer it equals, in decimal (`2.0` is `2`, `-0.0` is `0`, `1e22` is `10000000000000000000000`, and `1e23` computed as `1e22 * 10` is `99999999999999991611392`).
- Infinities and NaN: `inf`, `-inf`, `nan`.
- Any other float: the shortest decimal digit string that reads back as the same float, written as Python's `repr()` writes it: positional when 0.0001 <= |x| (`0.1`, `0.30000000000000004`, `0.0001`), exponent form below that (`1e-05`, `1.5e-07`, `5e-324`).

Core level: integers; whole floats below 10^16 in magnitude; and non-whole floats with 0.0001 <= |x|. Full level: all of the above.

### 12.6 Text to number

`num(s)` first strips leading and trailing white space (at the Full level, Unicode white space as Python's `str.strip()`; at the Core level, ASCII white space), giving `t`.

- If `t` is `-`? followed by one or more ASCII digits, the result is that integer, exactly (`"007"` is 7). With more than one leading `-` it is an error (`"--5"`).
- Full level, otherwise: `t` is read by Python's `float()` grammar: an optional sign; digits with an optional fraction, or a fraction alone (`".5"`, `"5."`); an optional exponent (`"1e3"`, `"1E-2"`); single underscores between digits (`"1_000"`); or `inf`, `infinity`, `nan` in any case with an optional sign. The result is the nearest float; out-of-range magnitudes become `inf` (`"1e400"`) or zero.
- Core level, otherwise: `-`? digits `.` digits is the nearest float.
- Anything else is `num() cannot convert R` (11.6).

So `num("+5")` and `num("1_000")` are floats while `num("5")` is an integer; the two print alike. Implementation-defined: whether non-ASCII decimal digits are accepted (tree and vm accept `"٣"` and full-width digits; c rejects them). A string of more than 4,300 digits is an error (section 14.2).

## 13 Strings and text

### 13.1 Code points

A string is a sequence of Unicode code points. `len`, indexing and ordering count code points, so `len("😀")` is 1 and a combining accent is a code point of its own. Strings reach the output unchanged.

### 13.2 Concatenation and comparison

`+` joins two strings. Strings are equal when their code points are (5.2) and ordered code point by code point (5.3). A string is never equal to a number, and there is no conversion between them except `str()` and `num()`.

### 13.3 The text of a value

| Value | Text |
| --- | --- |
| `nil`, `true`, `false` | `nil`, `true`, `false` |
| Number | Section 12.5 |
| String | The string itself, without quotes |
| List | `[` + the texts of the elements separated by `, ` + `]`; strings inside are not quoted: `[1, a b, , nil, [2, []]]` |
| Function | `<fn name>`, `<fn <anonymous>>`, `<builtin name>` (Full level) |

## 14 Limits

### 14.1 Call depth

At most 400 user function calls may be active at once; the top level is not a call and standard functions do not count. The call that would be the 401st is the catchable runtime error `stack overflow (max call depth 400)`, reported at its `(`. Core level: at least 400, and exceeding the implementation's limit is a catchable runtime error. Full level: exactly 400 (proposal P8 would make 400 a minimum).

### 14.2 Integer text

Converting an integer to text works up to 4,300 decimal digits (the sign does not count). Converting a longer one is a fatal error (legacy, P5). `num()` reads at most 4,300 digits; a longer digit string is the catchable `num() cannot convert R`. Required at the Full level. Implementation note: this is CPython's `sys.get_int_max_str_digits()` default, which the C VM copies.

### 14.3 Nesting

Every implementation must accept at least 100 nested parenthesized expressions, 100 nested blocks, 63 nested function literals, and convert to text a list nested 200 deep. Beyond its own limits an implementation must report a syntax error or a fatal error; it must not crash or stop other runs (Core). The actual limits are implementation-defined (Annex B), as is the outcome of converting to text a list that contains itself (a fatal error at the top level in all reference engines; inside a function, Annex B).

## 15 Conformance

### 15.1 Levels

| Level | Requires | Leaves open |
| --- | --- | --- |
| **minilang 1.0 Core** | Every specified rule, with the Core variants of 12.5, 12.6 and 14.1 | Message texts (a caught runtime error must still be a string), function text, legacy behaviour, Python-derived number text and `num()` grammar beyond the Core subset, the 4,300-digit limit, the exact call depth |
| **minilang 1.0 Full** | Everything in Core, exact message texts, every legacy behaviour, and the Full variants | Only what is implementation-defined |

All three reference engines are meant to be Full implementations. A Core implementation may implement any proposal of Annex C in place of the legacy behaviour it replaces.

### 15.2 What is observable

A run's observable result is the text it printed and how it ended (section 1.3): completed; syntax error with line and message; uncaught runtime error or thrown value with line and message; or fatal error. Timing, memory use, the value of a trailing expression statement in a REPL, bytecode and the wording of fatal-error reports are not observable for conformance.

### 15.3 Command-line contract

An implementation that is tested through a command line runs as `ENGINE FILE`:

- it reads FILE as UTF-8 and writes each `print` line to standard output in UTF-8;
- it exits with status 0 when the program completes;
- on an uncaught syntax or runtime error it writes `error: [line N] MESSAGE` and a line end to standard error and exits with status 1;
- on a fatal error it exits with status 1 and writes an implementation-defined report that does not start with `error: [line`;
- any other exit status means the implementation itself failed.

Implementation note: the tree and vm command lines write standard output in the locale's encoding, so on a system whose locale is not UTF-8 they need `PYTHONIOENCODING=utf-8`; the runner sets it (README).

### 15.4 The conformance suite

The suite in [tests/](tests/) has one directory per section 2 to 14; each test is a program whose expected output and ending are a comment block at its end, and each cites the sections it checks. `conformance.py` runs it on any engine and reports per section; the test format and runner are described in the [README](README.md). A test has a level (`core` or `full`) and a status (`specified`, `legacy` with its proposal, or `impl` with a permitted range and each reference engine's documented outcome).

## Annex A: decision register

Every behaviour that was implementation-defined in the engines, or surprising, with the decision this specification makes. S: specified. I: implementation-defined. P: proposed change, legacy in 1.0 (or implementation-defined where marked).

| # | Behaviour | Decision | Section | Tests |
| --- | --- | --- | --- | --- |
| D1 | UTF-8 source, LF line ends, a byte order mark is an error | S | 2.1 | `02-lexical/bom-not-allowed`, `line-counting` |
| D2 | Non-ASCII letters in identifiers | I | 2.3 | `02-lexical/unicode-identifier` |
| D3 | Non-ASCII white space in source | I | 2.2 | `02-lexical/unicode-whitespace` |
| D4 | Number literals: no sign, exponent or hex; digits on both sides of `.`; leading zeros allowed | S | 2.4 | `02-lexical/number-literals`, `no-exponent-literals`, `no-*-dot` |
| D5 | Non-ASCII digit characters in number literals (one crashes the front end) | P7 (1.0: I) | 2.4 | `02-lexical/literal-unicode-decimal-digit`, `literal-superscript-digit` |
| D6 | Integer literals of more than 4,300 digits (fatal before running) | P7 (1.0: I) | 2.4 | `02-lexical/literal-4301-digits` |
| D7 | Four escapes only; raw newlines allowed in strings | S | 2.5 | `02-lexical/string-*` |
| D8 | `(a) = 1` is a valid assignment | S | 3.4 | `03-syntax/parenthesized-target` |
| D9 | No empty statement, trailing comma or `let` without initializer | S | 3.2 | `03-syntax/no-*`, `let-requires-initializer` |
| D10 | Comparisons do not chain | S | 3.3 | `03-syntax/comparison-does-not-chain` |
| D11 | Booleans are not numbers; `==` never fails; values of different types are unequal | S | 4.1, 5.2 | `04-values/booleans-are-not-numbers`, `05-equality/cross-type-equality` |
| D12 | Only `nil` and `false` are false | S | 5.1 | `05-equality/truthiness` |
| D13 | Lists and functions compare by identity | S | 5.2 | `05-equality/identity-equality` |
| D14 | Integer and float compare exactly; NaN is unordered | S | 5.2, 5.3 | `05-equality/number-equality-exact`, `nan` |
| D15 | Operands are all evaluated before checks | S | 6.1 | `06-expressions/order-*` |
| D16 | An integral float is a valid index | S | 6.4 | `06-expressions/index-read` |
| D17 | An infinite or NaN index is fatal | P5 | 6.4 | `06-expressions/index-infinite`, `index-nan` |
| D18 | `let` in the same scope replaces; parameters share the body's scope | S | 7.1 | `07-scope/redeclaration`, `parameters-and-body-share-scope` |
| D19 | Names resolve at use: forward references and mutual recursion work | S | 7.3 | `07-scope/forward-reference-global`, `mutual-recursion-local` |
| D20 | A later `let` changes what a closure's name refers to | P2 | 7.4 | `07-scope/later-let-*` |
| D21 | A loop body is a new scope per iteration | S | 7.2 | `07-scope/loop-body-new-scope` |
| D22 | Assignment never declares | S | 7.6 | `07-scope/assign-undeclared` |
| D23 | Standard functions are reassignable globals | S | 11.9 | `07-scope/builtins-are-globals` |
| D24 | `break` crosses function calls | P3 | 8.3 | `08-control/break-across-*`, `break-from-loop-condition` |
| D25 | `break` with no running loop is fatal | P3 | 8.3 | `08-control/break-outside-loop`, `break-in-function-outside-loop` |
| D26 | Top-level `return` is fatal | P4 | 8.4 | `08-control/top-level-return*` |
| D27 | A caught runtime error is its message string | S | 10.3 | `10-errors/runtime-error-value-is-string` |
| D28 | Exact message texts | S at Full; I at Core | 10.3, 15.1 | `10-errors/runtime-error-messages` and every `message:` |
| D29 | Error line rules | S | 10.4 | `10-errors/line-*` |
| D30 | `throw nil` is caught as `"nil"` | P1 | 10.6 | `10-errors/throw-nil*` |
| D31 | Throwing a value without text is fatal at the throw | P5 | 10.5 | `10-errors/throw-unprintable-value` |
| D32 | Reports of fatal errors | I | 10.7 | (wording not tested) |
| D33 | `num()` follows Python's `int()`/`float()` text grammar | S at Full | 12.6 | `11-builtins/num-full-*` |
| D34 | `num()` of non-ASCII decimal digits | I (engines differ) | 12.6 | `11-builtins/num-unicode-digits` |
| D35 | `num()` messages use Python's `repr()` | P6 | 11.6 | `11-builtins/num-python-repr-in-message` |
| D36 | `num(true)` is `1.0` | P6 | 11.6 | `11-builtins/num-of-bool` |
| D37 | `clock()` epoch and resolution | I | 11.8 | `11-builtins/clock` |
| D38 | Integers have no size limit | S | 12.1 | `12-numbers/integers-unbounded` |
| D39 | Float division by zero is an error, not an infinity | S | 12.3 | `12-numbers/division-by-zero-*` |
| D40 | A whole integer quotient goes through float rounding | P9 | 12.3 | `12-numbers/division-rounds-through-float` |
| D41 | An integer too large for a float is fatal when converted | P5 | 12.3 | `12-numbers/huge-*` |
| D42 | `%` takes the sign of the divisor | S | 12.4 | `12-numbers/remainder` |
| D43 | Float overflow gives `inf` without an error | S at Full | 12.2 | `12-numbers/float-overflow` |
| D44 | Number text is Python's `repr()`; whole floats print as exact integers; `-0.0` prints `0` | S (Core subset) | 12.5 | `12-numbers/float-text-*`, `negative-zero` |
| D45 | Strings count code points | S | 13.1 | `13-strings/code-points` |
| D46 | Call depth exactly 400, catchable at the call | S at Full (Core: at least 400); P8 | 14.1 | `14-limits/call-depth-*` |
| D47 | Integer text limited to 4,300 digits | S at Full; fatal part P5 | 14.2 | `14-limits/int-text-*`, `num-4300-digits` |
| D48 | Nesting limits beyond the minimums; no crash past them | I with S minimums | 14.3 | `14-limits/nesting-*` |
| D49 | Text of a list that contains itself | I; P5 | 14.3 | `14-limits/cyclic-list-text*` |
| D50 | Output encoding | I; UTF-8 in the command-line contract | 15.3 | `13-strings/unicode-output` |

## Annex B: implementation-defined behaviour of the reference engines

Measured with the suite on 2026-11-16 (Windows 11, Python 3.14.6, WinLibs GCC 16.2.0), through both the Python APIs and the command lines; the `doc-` lines in the tests record these outcomes, and `conformance.py --strict` fails if an engine stops matching them.

| Item | tree | vm | c |
| --- | --- | --- | --- |
| Non-ASCII identifier letters, white space (D2, D3) | Python's `str.isalpha`, `isalnum`, `isspace` | Same lexer (imports tree's) | Same lexer (Python front end) |
| Non-ASCII digits in literals (D5) | Decimal digits (`str.isdigit` and `int()` agree) are read as their value; other digit characters such as `²` stop the front end with a `ValueError` before anything runs | Same | Same |
| Integer literal over 4,300 digits (D6) | `ValueError` in the lexer, before anything runs | Same | Same |
| Fatal error report (D32) | Python traceback naming the exception, exit status 1 | Same | One line starting `error: ` that names the same Python exception type, exit status 1 |
| `num()` of non-ASCII decimal digits (D34) | Accepted (`"٣"` is 3, full-width `"１２"` is 12) | Accepted | Rejected: `num() cannot convert '٣'` |
| Characters escaped in `num()` messages | Python's Unicode database | Same | A fixed list (controls, spaces, format characters, surrogates, private use); rare code points may differ |
| `clock()` (D37) | `time.time()` | Same | Wall-clock seconds since the Unix epoch |
| Nested parentheses, blocks, function literals (D48) | Limited by Python's recursion limit, raised to 20,000 by the command line; 1,000 parentheses, 500 blocks and 250 function literals were run | Same parser; the compiler also recurses | Same front end; the C loader accepts 63 nested function literals and **crashes at 64** (see README) |
| Text of a nested list (D48) | Python stack: 1,100 levels printed | Same | 999 levels; 1,000 or more is a fatal `RecursionError` |
| Text of a list that contains itself, outside a function (D49) | Fatal `RecursionError` | Same | Same |
| The same inside a function, inside `try` (D49) | Catchable runtime error `stack overflow (host recursion limit)` at the call | **Fatal `RecursionError`** | Catchable, as tree |
| Output encoding (D50) | The locale's encoding; on a cp950 system, non-ASCII output fails with `UnicodeEncodeError` unless `PYTHONIOENCODING=utf-8` | Same | UTF-8 bytes |

## Annex C: proposed changes

For the owner to decide. None is implemented; each lists the tests that would change. All but P7 change behaviour that 1.0 requires at the Full level, so they belong in a 2.0 (Annex D).

| # | Proposal | Replaces | Rationale | Cost |
| --- | --- | --- | --- | --- |
| P1 | `throw nil` binds `nil`; uncaught, it reports `nil` as now | D30 | A thrown value should arrive unchanged, as every other value does. The string is an artifact of the tree-walker storing the message when the value is `None` | Small in all three engines; flips `10-errors/throw-nil` |
| P2 | Static block scoping: a name refers to the innermost enclosing block that declares it anywhere; using it before its `let` has run is `undefined variable` | D20 | One name in one closure should not mean two variables at different times. Lua, JavaScript (`let`) and Python resolve statically; forward references and mutual recursion (D19) keep working because the declaration has run by the time of the call | The vm's `LOAD_CHAIN`/`STORE_CHAIN` machinery and its bytecode can be removed; the tree-walker needs a resolver pass; MLBC major change |
| P3 | `break` outside a loop of the same function is a syntax error | D24, D25 | Control flow becomes visible in the source; removes a fatal error; matches every mainstream language | The vm's `BREAK_OUT` and loop tables go away (MLBC major change); front end gains a check |
| P4 | `return` outside a function is a syntax error | D26 | Removes a fatal error that can only be a mistake; Python and JavaScript reject it | Front end only |
| P5 | No fatal errors from ordinary operations: converting a too-large integer to float, a too-large quotient, the 4,300-digit text limit, a NaN or infinite index and an unprintable thrown value become catchable runtime errors with specified messages; a list that contains itself prints as `[...]` | D17, D31, D41, D47, D49 | Every error a program can cause should be catchable and reported with a line; today try cannot protect against them and the engines already disagree inside functions (Annex B) | Tree and vm: catch the host exceptions at the operation; c: map its host errors to runtime errors |
| P6 | `num()` without Python leftovers: messages use minilang text (`num() cannot convert nil`, `"a"` quoted the minilang way), booleans are rejected, and the accepted grammar is written out in this document instead of by reference to Python | D35, D36 | Python's `None` and `True` in a minilang message are leaks of the host; a second implementation should not need Python's `repr()` | Small; the C VM's `repr` port can be deleted |
| P7 | Number literals use ASCII digits only; any other character is `unexpected character`; integer literals have no length limit; the front end never fails with a host exception | D5, D6 | The front end currently crashes on `²`; this is a bug fix more than a design change and only touches implementation-defined behaviour, so it can go in a 1.x | Front end only (shared by all three) |
| P8 | The call depth limit becomes a minimum of 400, and the message drops the number: `stack overflow` | D46 | A C implementation with a real stack can allow deeper recursion; programs should not rely on the exact limit | Small; changes Full-level messages |
| P9 | `int / int` that divides exactly gives the exact integer quotient; otherwise the correctly rounded float | D40 | `9007199254740993 / 1` should not change the number; exact quotients of huge integers would no longer overflow | Small in all three; the C VM's quotient code already has the exact path |

## Annex D: versioning, change process and release gate

### D.1 Two version numbers

- **Language version** (this document): MAJOR.MINOR. A MINOR version may add behaviour (a new standard function, a new syntax form) and change implementation-defined behaviour; it never changes the result of a program whose result 1.0 specifies at the Full level. A MAJOR version may change specified or legacy behaviour; accepting P1 to P6, P8 or P9 makes 2.0, where the legacy rules they replace are deleted.
- **Bytecode format** (MLBC, documented in [minilang-c](../minilang-c/README.md#bytecode-file-format-10), now 1.0): an interface between the Python compiler and the two VMs, not part of the language. Its existing rules stay: readers reject another major version, a newer minor version or a different opcode count, and renumbering opcodes is a major change. Added rule: a language change that alters what an existing instruction does (P2 for `LOAD_CHAIN`, P3 for `BREAK_OUT`, P1 and P5 for `THROW` and the operators) is an MLBC major change even if no opcode is added, because MLBC files do not record the language version and an old file would silently change meaning.
- **Suite**: the tests in this folder are the 1.0 suite; `conformance.py` prints the language version it checks.

### D.2 How a change moves

1. **Proposal.** An entry in Annex C: the rule, what it replaces, the rationale, the tests that change and the cost per engine. Many come from differential testing, as D20, D24 and D30 did.
2. **Specification.** The owner accepts it for a version. The text of both SPEC files and the tests change in one commit; tests for the new rule fail on the engines (the runner shows which), and the replaced tests become the new rule.
3. **Engines.** Each engine is changed until the suite passes in both modes and all configurations, and its own tests (`test_minilang`, `test_minilang_vm`, `test_minilang_c`, `difftest.py`) still pass. A front-end change lands in `minilang.py` and reaches the other two engines through it.
4. **Release.** The version is bumped in both SPEC files and in `conformance.py`; the README records the matrix.

The roadmap's TC39 analogue asks for two compatible implementations before a feature is final. Here the three engines are not independent everywhere: the lexer and parser exist once, the vm reuses the tree-walker's standard functions, text and operator slow paths, and only the c engine reimplements operators, numbers and text. Agreement on sections 2 and 3 is one implementation agreeing with itself; agreement on sections 6, 11, 12 and 13 is two; on control flow and scoping it is three.

### D.3 Release gate

A release of the language, or of any engine, requires:

1. `python3 conformance.py --all-configs` and `python3 conformance.py --cli --all-configs` exit with status 0 (every test at the Full level passes on every engine and configuration);
2. `--strict` also passes: every engine still behaves as Annex B documents, or Annex B is updated in the same change;
3. each engine's own test suite passes.

A failing test is resolved by fixing the engine, or by changing the rule through D.2; never by editing an expectation to match an engine. As of this document the gate is **not met**: the c engine crashes on 64 or more nested function literals (`14-limits/nesting-functions-200`), which is a bug in the C VM to fix, not a rule to change.
