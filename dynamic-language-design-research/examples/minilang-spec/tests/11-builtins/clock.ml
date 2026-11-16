let t = clock();
print(type(t), t > 0, clock() >= t);

# ---
# spec: 11.8
# title: clock returns a number of seconds that does not go backwards between two calls
# end: ok
#> number true true
