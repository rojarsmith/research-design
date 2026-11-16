let g = fn(a) {};
g(1, 2);

# ---
# spec: 9.2
# title: too many arguments to an anonymous function
# end: runtime-error 2
# message: <fn <anonymous>> expects 1 argument(s), got 2
