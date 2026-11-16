let original = print;
print = fn(x) { original("wrapped: " + x); };
print("hi");
{ let len = 5; original(len); }
original(len("abc"));

# ---
# spec: 7.3 11.9
# title: standard functions are global variables that can be reassigned or shadowed
# end: ok
#> wrapped: hi
#> 5
#> 3
