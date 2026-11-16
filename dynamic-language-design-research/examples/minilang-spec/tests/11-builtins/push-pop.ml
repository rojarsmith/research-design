let xs = [];
print(push(xs, 1) == xs);
print(push(xs, 2));
print(pop(xs), xs);

# ---
# spec: 11.3 11.4
# title: push returns the list; pop removes and returns the last element
# end: ok
#> true
#> [1, 2]
#> 2 [1]
