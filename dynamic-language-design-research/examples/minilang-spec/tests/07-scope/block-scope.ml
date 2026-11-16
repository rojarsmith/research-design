let x = 1;
{ let x = 2; print(x); }
print(x); x = 3; print(x);

# ---
# spec: 7.2
# title: a block's let shadows an outer variable until the block ends
# end: ok
#> 2
#> 1
#> 3
