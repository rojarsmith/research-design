let big = 1.0; let i = 0;
while (i < 400) { big = big * 10; i = i + 1; }
print(big, -big, big - big);

# ---
# spec: 12.2
# title: float overflow gives infinity, and inf - inf gives nan
# level: full
# end: ok
#> inf -inf nan
