try {
  try { throw "a"; } catch (e) { print("inner: " + e); throw e + "!"; }
} catch (e2) { print("outer: " + e2); }
print("continues");

# ---
# spec: 10.2
# title: the innermost handler runs; a handler can throw to an outer one
# end: ok
#> inner: a
#> outer: a!
#> continues
