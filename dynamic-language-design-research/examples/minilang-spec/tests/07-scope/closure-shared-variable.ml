fn outer() { let v = 1; fn get() { return v; } fn set(x) { v = x; } return [get, set]; }
let p = outer(); p[1](42); print(p[0]());

# ---
# spec: 7.5
# title: two closures over one variable see each other's writes
# end: ok
#> 42
