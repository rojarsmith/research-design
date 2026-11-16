fn f(a) {}
f(print("arg1"), print("arg2"));

# ---
# spec: 9.2 6.1
# title: arguments are evaluated before the count is checked
# end: runtime-error 2
# message: <fn f> expects 1 argument(s), got 2
#> arg1
#> arg2
