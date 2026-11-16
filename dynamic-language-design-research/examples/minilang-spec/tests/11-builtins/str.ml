print(str(1) + str(2.5) + str(nil) + str(true) + str([1, "a"]) + str("s"));
let s = "x"; print(str(s) == s);

# ---
# spec: 11.5 13.3
# title: str gives the text print would show
# end: ok
#> 12.5niltrue[1, a]s
#> true
