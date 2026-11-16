let m = [[fn(x) { return [x, x * 2]; }]];
print(m[0][0](5)[1]);

# ---
# spec: 3.3
# title: calls and indexing chain left to right
# end: ok
#> 10
