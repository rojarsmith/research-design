fn r() { return r(); }
try { r(); } catch (e) { print("caught"); }
print("after");

# ---
# spec: 10.2 14.1
# title: exceeding the call depth is a catchable runtime error
# end: ok
#> caught
#> after
