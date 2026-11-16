fn f() { break; }
print(1);
f();
print(2);

# ---
# spec: 8.3 10.7
# title: the same from inside a function
# level: full
# status: legacy P3
# end: fatal
#> 1
