print("before");
print(
  1 +
  "a"
);

# ---
# spec: 10.4
# title: an operator error is reported at the operator's line
# end: runtime-error 3
# message: '+' expects two numbers, two strings or two lists, got number and string
#> before
