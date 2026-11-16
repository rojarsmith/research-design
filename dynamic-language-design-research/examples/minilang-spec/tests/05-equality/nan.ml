let n = num("nan");
print(n == n, n != n, n < n, n >= n, n < 1, n > 1);

# ---
# spec: 5.2 5.3 12.2
# title: NaN is not equal to itself and is unordered
# level: full
# end: ok
#> false true false false false false
