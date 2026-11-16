print("start");
throw [1, "a", nil];

# ---
# spec: 10.5
# title: an uncaught thrown value is reported with its text
# end: runtime-error 2
# message: [1, a, nil]
#> start
