fn t(v) { if (v) { return "T"; } return "F"; }
print(t(0), t(""), t([]), t(nil), t(false), t(true), t(0.0), t(print));

# ---
# spec: 5.1
# title: only nil and false are falsy
# end: ok
#> T T T F F T T T
