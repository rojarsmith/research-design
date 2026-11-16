let s = "abc"; s[0] = "x";

# ---
# spec: 4.1 6.4
# title: index assignment into a string is an error
# end: runtime-error 1
# message: can only assign into a list, got string
