fn mk() { return [1, 2]; }
print(mk()[0] = 9);

# ---
# spec: 6.4
# title: any index expression is a target, including a call result
# end: ok
#> 9
