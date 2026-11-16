fn outer() {
  let fact = fn(n) { if (n < 2) { return 1; } return n * fact(n - 1); };
  return fact(10);
}
print(outer());

# ---
# spec: 7.3
# title: a function stored by let can call itself through that name
# end: ok
#> 3628800
