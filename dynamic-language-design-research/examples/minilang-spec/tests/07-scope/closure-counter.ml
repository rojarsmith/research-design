fn counter() { let n = 0; return fn() { n = n + 1; return n; }; }
let c1 = counter(); let c2 = counter();
print(c1(), c1(), c2());

# ---
# spec: 7.5
# title: closures capture variables, each call gets new ones
# end: ok
#> 1 2 1
