5[print("i")] = print("v");

# ---
# spec: 6.1 6.4
# title: object, index and value are evaluated before the checks
# end: runtime-error 1
# message: can only assign into a list, got number
#> i
#> v
