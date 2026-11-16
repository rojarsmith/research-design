let a = 1; let b = 2;
a = b = 3;
print(a, b, (a = 7) + 1, a);

# ---
# spec: 3.4 6.5
# title: assignment is a right-associative expression
# end: ok
#> 3 3 8 7
