fn a() { return; } fn b() {} fn c() { 1 + 1; }
print(a(), b(), c());

# ---
# spec: 9.3
# title: return without a value, and falling off the end, give nil
# end: ok
#> nil nil nil
