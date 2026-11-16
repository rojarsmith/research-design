fn mk() { return fn() {}; }
print(mk() == mk());

# ---
# spec: 5.2 7.5
# title: each evaluation of fn makes a new function
# end: ok
#> false
