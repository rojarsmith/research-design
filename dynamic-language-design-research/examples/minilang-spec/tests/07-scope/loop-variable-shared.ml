let i = 0; let fns = [];
while (i < 3) { push(fns, fn() { return i; }); i = i + 1; }
print(fns[0](), fns[2]());

# ---
# spec: 7.5
# title: a variable declared outside the loop is shared by every closure
# end: ok
#> 3 3
