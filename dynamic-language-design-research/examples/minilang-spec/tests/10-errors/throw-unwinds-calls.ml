fn deep(n) { if (n == 0) { throw "bottom"; } return deep(n - 1); }
try { deep(50); } catch (e) { print(e); }
fn count(n) { if (n == 0) { return 0; } return 1 + count(n - 1); }
print(count(399));

# ---
# spec: 10.2 14.1
# title: an error unwinds calls; the call depth is restored
# end: ok
#> bottom
#> 399
