let log = []; fn t(x) { push(log, x); return x; } fn f(a, b) { return a - b; }
let r = t(f)(t(5), t(3));
print(r, len(log), log[0] == f, log[1], log[2]);

# ---
# spec: 6.1 9.2
# title: the callee first, then the arguments left to right
# end: ok
#> 2 3 true 5 3
