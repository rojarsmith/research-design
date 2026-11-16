fn f(a) { return a; }
f
(
  1, 2);

# ---
# spec: 10.4 9.2
# title: a call error is reported at the line of the opening parenthesis
# end: runtime-error 3
# message: <fn f> expects 1 argument(s), got 2
