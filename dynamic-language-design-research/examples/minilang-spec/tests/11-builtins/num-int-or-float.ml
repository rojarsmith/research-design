print(num("12345678901234567890123"), num("12345678901234567890123.0"));

# ---
# spec: 11.6 12.6
# title: digits give an exact integer; a decimal point gives a float
# level: full
# end: ok
#> 12345678901234567890123 12345678901234567741440
