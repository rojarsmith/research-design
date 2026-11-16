let s = "first
second";
print(len(s));
print(s);
print(undefined_after);

# ---
# spec: 2.5 2.1
# title: a string may contain raw newlines; lines keep counting
# end: runtime-error 5
# message: undefined variable 'undefined_after'
#> 12
#> first
#> second
