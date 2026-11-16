fn named() {} let anon = fn() {};
print(named, anon, print, str(len));

# ---
# spec: 9.1 13.3
# title: how functions print
# level: full
# end: ok
#> <fn named> <fn <anonymous>> <builtin print> <builtin len>
