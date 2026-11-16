fn d(n) { if (n == 0) { return len("abc"); } return d(n - 1); }
print(d(399));
fn w() { return d(398); }
print(w());
fn w2() { return d(399); }
try { print(w2()); } catch (e) { print(e); }

# ---
# spec: 14.1
# title: the limit counts frames, not calls to standard functions
# level: full
# end: ok
#> 3
#> 3
#> stack overflow (max call depth 400)
