let i = 0; let fns = [];
while (i < 3) { let j = i; push(fns, fn() { return j; }); i = i + 1; }
print(fns[0](), fns[1](), fns[2]());

# ---
# spec: 7.2 7.5
# title: a let in a loop body is new on every iteration
# end: ok
#> 0 1 2
