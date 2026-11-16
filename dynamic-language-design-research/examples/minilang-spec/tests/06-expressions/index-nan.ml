let xs = [1, 2, 3];
try { print(xs[num("nan")]); } catch (e) { print("caught"); }

# ---
# spec: 6.4 10.7
# title: NaN as an index
# level: full
# status: legacy P5
# end: fatal
