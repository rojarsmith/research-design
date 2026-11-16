let xs = [10, 20, 30]; let s = "abc";
print(xs[0], xs[-1], xs[1.0], s[1], s[-3]);

# ---
# spec: 6.4
# title: lists and strings index from 0; negative indexes count from the end; integral floats are accepted
# end: ok
#> 10 30 20 b a
