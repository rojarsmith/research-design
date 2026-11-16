try { 1 / 0; } catch (e) { print(type(e)); }

# ---
# spec: 10.3
# title: catch binds a runtime error as a string
# end: ok
#> string
