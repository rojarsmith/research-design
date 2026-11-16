let a = [1]; let b = [1]; fn f() {} let g = f;
print(a == a, a == b, a != b, f == g, f == fn() {}, print == print, len == print);

# ---
# spec: 5.2
# title: lists and functions compare by identity
# end: ok
#> true false true true false true false
