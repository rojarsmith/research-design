let i = 0; let total = 0;
while (i < 5) { total = total + i; i = i + 1; }
while (false) { print("never"); }
print(total, i);

# ---
# spec: 8.2
# title: while re-tests its condition; a false condition runs nothing
# end: ok
#> 10 5
