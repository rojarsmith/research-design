fn twice(f, x) { return f(f(x)); }
print(twice(fn(v) { return v * 3; }, 2));
let p = print; p("via alias");

# ---
# spec: 4.3 9.1
# title: functions are passed, stored and aliased like other values
# end: ok
#> 18
#> via alias
