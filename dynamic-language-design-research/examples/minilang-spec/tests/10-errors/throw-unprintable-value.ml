let a = []; push(a, a);
try { throw a; } catch (e) { print("caught"); }

# ---
# spec: 10.7
# title: throwing a list that contains itself fails at the throw, even inside try
# level: full
# status: legacy P5
# end: fatal
