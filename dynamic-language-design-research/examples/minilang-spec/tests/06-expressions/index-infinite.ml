let xs = [1, 2, 3];
try { print(xs[num("inf")]); } catch (e) { print("caught"); }

# ---
# spec: 6.4 10.7
# title: an infinite float as an index
# level: full
# status: legacy P5
# end: fatal
