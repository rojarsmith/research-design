print(num("1_000"), num("+5"), num(".5"), num("5."), num("1e3"), num("1E-2"), num("inf"), num("-Infinity"), num("nAn"), num("007"), num("-007"));

# ---
# spec: 11.6 12.6
# title: num accepts Python's int() and float() text
# level: full
# end: ok
#> 1000 5 0.5 5 1000 0.01 inf -inf nan 7 -7
