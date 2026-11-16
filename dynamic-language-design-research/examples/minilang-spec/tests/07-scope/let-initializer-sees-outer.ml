let x = 10;
{ let x = x + 1; print(x); }
print(x);

# ---
# spec: 7.1 7.3
# title: the initializer runs before the new name exists
# end: ok
#> 11
#> 10
