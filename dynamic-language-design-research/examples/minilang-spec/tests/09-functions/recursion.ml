fn fib(n) { if (n < 2) { return n; } return fib(n - 1) + fib(n - 2); }
print(fib(20));

# ---
# spec: 9.2
# title: recursive Fibonacci
# end: ok
#> 6765
