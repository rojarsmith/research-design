let n = 0;
fn f() { n = n + 1; if (n > 2) { break; } return n < 6; }
while (f()) { print(n); }
print("after", n);

# ---
# spec: 8.3
# title: break in a function called by a loop condition stops that loop
# level: full
# status: legacy P3
# end: ok
#> 1
#> 2
#> after 3
