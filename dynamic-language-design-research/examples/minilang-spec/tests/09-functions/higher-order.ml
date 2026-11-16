fn map(xs, f) { let out = []; let i = 0; while (i < len(xs)) { push(out, f(xs[i])); i = i + 1; } return out; }
print(map([1, 2, 3], fn(x) { return x * x; }));

# ---
# spec: 9.1
# title: functions as arguments
# end: ok
#> [1, 4, 9]
