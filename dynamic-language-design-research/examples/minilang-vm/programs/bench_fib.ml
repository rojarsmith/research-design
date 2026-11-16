# Benchmark 1: recursive Fibonacci. Mostly calls, returns, comparisons and
# small-integer arithmetic; 242,785 calls.
fn fib(n) {
  if (n < 2) { return n; }
  return fib(n - 1) + fib(n - 2);
}
print(fib(25));
