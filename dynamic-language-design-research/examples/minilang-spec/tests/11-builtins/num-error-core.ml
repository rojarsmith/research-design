try { num("abc"); } catch (e) { print(type(e)); }
try { num(nil); } catch (e) { print("nil rejected"); }

# ---
# spec: 11.6
# title: num of text that is not a number is a runtime error
# end: ok
#> string
#> nil rejected
