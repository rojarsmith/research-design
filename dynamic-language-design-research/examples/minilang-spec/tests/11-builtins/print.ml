print();
print(1, "two", nil, true, [3]);
print("", "b");

# ---
# spec: 11.1
# title: print joins its arguments with one space; no arguments print an empty line
# end: ok
#>
#> 1 two nil true [3]
#>  b
