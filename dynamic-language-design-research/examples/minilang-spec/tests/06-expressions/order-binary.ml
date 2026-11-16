fn t(x) { print(x); return x; }
print(t(1) + t(2) * t(3));

# ---
# spec: 6.1
# title: operands are evaluated left to right
# end: ok
#> 1
#> 2
#> 3
#> 7
