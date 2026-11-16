let a = [1]; let b = a + [2]; push(b, 3);
print(a, b, a == b);

# ---
# spec: 4.2 6.3
# title: + on lists makes a new list
# end: ok
#> [1] [1, 2, 3] false
