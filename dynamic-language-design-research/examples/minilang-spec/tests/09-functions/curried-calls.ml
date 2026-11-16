let add = fn(x) { return fn(y) { return x + y; }; };
print(add(1)(2), [fn(x) { return x * 2; }][0](21));

# ---
# spec: 9.1 9.2
# title: a call result can be called and a list element can be called
# end: ok
#> 3 42
