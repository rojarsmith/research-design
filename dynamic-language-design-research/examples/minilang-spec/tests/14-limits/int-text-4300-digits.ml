let x = 1; let i = 0; while (i < 4299) { x = x * 10; i = i + 1; }
print(len(str(x)), len(str(-x)), str(x)[0], str(-x)[4300]);

# ---
# spec: 14.2 12.5
# title: integers of up to 4,300 digits convert to text; the sign does not count
# level: full
# end: ok
#> 4300 4301 1 0
