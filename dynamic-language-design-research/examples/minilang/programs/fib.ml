# Recursive and iterative Fibonacci, with timing.
fn fib(n) {
  if (n < 2) { return n; }
  return fib(n - 1) + fib(n - 2);
}

fn fib_iter(n) {
  let a = 0; let b = 1; let i = 0;
  while (i < n) { let t = a + b; a = b; b = t; i = i + 1; }
  return a;
}

let start = clock();
print("fib(20) =", fib(20));
print("fib_iter(90) =", fib_iter(90));
print("elapsed seconds:", clock() - start);
