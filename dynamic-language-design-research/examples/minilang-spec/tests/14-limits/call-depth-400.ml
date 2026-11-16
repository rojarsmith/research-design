fn d(n) { if (n == 0) { return 0; } return 1 + d(n - 1); }
print(d(399));

# ---
# spec: 14.1
# title: 400 nested calls are allowed
# end: ok
#> 399
