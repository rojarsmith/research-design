try { try { throw 1; } catch (e) { print(undefined_in_handler); } } catch (e) { print("outer caught"); }

# ---
# spec: 10.2
# title: an error inside a handler goes to the enclosing try
# end: ok
#> outer caught
