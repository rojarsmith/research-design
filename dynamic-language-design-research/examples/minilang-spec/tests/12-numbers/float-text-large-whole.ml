let big = 1.0; let i = 0;
while (i < 22) { big = big * 10; i = i + 1; }
print(big, big * 10, 123456789012345678.0);

# ---
# spec: 12.5
# title: a whole float prints as its exact integer value
# level: full
# end: ok
#> 10000000000000000000000 99999999999999991611392 123456789012345680
