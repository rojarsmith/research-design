fn f() { let i = 0; while (i < 10) { i = i + 1; if (i == 5) { return i; } } }
fn g() { try { return "from try"; } catch (e) { return "catch"; } }
print(f(), g());

# ---
# spec: 8.4
# title: return leaves loops and try blocks
# end: ok
#> 5 from try
