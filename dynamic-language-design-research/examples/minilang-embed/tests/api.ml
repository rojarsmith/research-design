# Functions that test_embed.c calls through the embedding API.
# test_embed.c checks line numbers 5, 9 and 33; keep them where they are.

fn fail(msg) {
  throw msg;
}
fn fail_value() { throw [1, 2]; }
fn divide(a, b) {
  return a / b;
}

fn echo(x) { return x; }
fn add(a, b) { return a + b; }
fn describe(x) { return type(x) + ":" + str(x); }
fn sum(xs) {
  let total = 0;
  let i = 0;
  while (i < len(xs)) { total = total + xs[i]; i = i + 1; }
  return total;
}
fn pow2(n) {
  let x = 1;
  while (n > 0) { x = x * 2; n = n - 1; }
  return x;
}
fn huge_plus_half() { return pow2(1100) + 0.5; }   # OverflowError, which minilang cannot catch
fn breaker() { break; }
fn deep(n) { return deep(n + 1); }
fn depth(n) { if (n == 0) { return 0; } return 1 + depth(n - 1); }
fn make_counter() { let n = 0; return fn() { n = n + 1; return n; }; }
fn read_setting() { return setting; }
fn call_host_double(x) {
  return host_double(x) + 1;
}

# Host functions called from minilang: errors raised in C are minilang exceptions.
fn catch_call(f, x) {
  try { return "ok: " + str(f(x)); } catch (e) { return "caught: " + str(e); }
}
fn catch_identity(f, x) {
  try { f(x); } catch (e) { return e == x; }
  return "not thrown";
}

# Reentrancy: minilang -> apply (C) -> minilang -> apply (C) -> minilang.
fn nested(x) {
  return apply(fn(y) { return apply(fn(z) { return z * 2; }, y) + 1; }, x);
}
fn nested_throw(x) {
  try {
    return apply(fn(y) { return apply(fn(z) { throw "inner " + str(z); }, y); }, x);
  } catch (e) { return "outer caught: " + e; }
}
fn rec_through_c(n) { return apply(rec_through_c, n + 1); }
fn catch_rec() {
  try { rec_through_c(0); } catch (e) { return e; }
}
fn keeps_locals() {
  let before = "kept";
  let r = apply(fn(x) { return depth(300); }, 0);   # moves the VM stack while this frame waits
  return before + " " + str(r);
}
fn break_through_c() {
  let i = 0;
  while (i < 3) { apply(fn(x) { break; }, i); i = i + 1; }
  return i;
}
fn fallback(x) { return apply_or(fn(y) { return 10 / y; }, x, "fallback"); }

# The heap limit.
fn hog() {
  let xs = [];
  while (true) { push(xs, "item " + str(len(xs))); }
}
fn try_hog() {
  try { hog(); } catch (e) { return "caught"; }
  return "done";
}
fn hog_recursive(xs) { return hog_recursive(xs + [xs]); }

fn run_side_from_minilang() { return run_side() + 1; }

let setting = "default";
"loaded";
