fn f() { return later; }
let later = "ok";
print(f());

# ---
# spec: 7.3
# title: a function body may name a global declared later
# end: ok
#> ok
