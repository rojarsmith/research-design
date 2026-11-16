let s = ""; let i = 0; while (i < 4300) { s = s + "7"; i = i + 1; }
print(num(s) % 1000, len(str(num(s))), num("-" + s) % 1000);
try { num(s + "7"); } catch (e) { print(len(e)); }

# ---
# spec: 14.2 11.6
# title: num reads up to 4,300 digits; more is a catchable error
# level: full
# end: ok
#> 777 4300 223
#> 4324
