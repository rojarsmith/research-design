let xs = [1, 2]; let ys = xs;
push(ys, 3); ys[0] = 9;
print(xs, xs == ys, len(xs));

# ---
# spec: 4.2
# title: assignment shares a list; push and index assignment mutate it
# end: ok
#> [9, 2, 3] true 3
