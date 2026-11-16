# a comment line
print("one");

# another comment
print(missing);

# ---
# spec: 2.1 10.4
# title: lines are counted from 1; comments and blank lines count
# end: runtime-error 5
# message: undefined variable 'missing'
#> one
