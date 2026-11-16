let m = [[0, 0], [0, 0]];
m[1][0] = 5; m[-1][-1] = 6;
print(m, m[0][1] = 7, m);

# ---
# spec: 6.4 6.5
# title: index assignment returns the value; lists print when print runs
# end: ok
#> [[0, 7], [5, 6]] 7 [[0, 7], [5, 6]]
