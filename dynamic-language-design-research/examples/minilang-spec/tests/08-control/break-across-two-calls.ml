fn a() { b(); }
fn b() { break; }
let i = 0;
while (i < 5) { i = i + 1; a(); print("not reached"); }
print(i);

# ---
# spec: 8.3
# title: the same through two calls
# level: full
# status: legacy P3
# end: ok
#> 1
