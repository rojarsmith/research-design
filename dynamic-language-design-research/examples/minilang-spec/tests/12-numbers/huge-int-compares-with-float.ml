let x = 1; let i = 0; while (i < 400) { x = x * 10; i = i + 1; }
let big = 1.0; let j = 0; while (j < 400) { big = big * 10; j = j + 1; }
print(x < big, x == big, x > 1.5, x == x * 1);

# ---
# spec: 12.3
# title: comparisons between huge integers and floats are exact and never overflow
# level: full
# end: ok
#> true false true true
