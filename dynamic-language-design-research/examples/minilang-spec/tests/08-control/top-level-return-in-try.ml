try { return 1; } catch (e) { print("caught"); }
print("after");

# ---
# spec: 8.4 10.7
# title: try does not catch a top-level return
# level: full
# status: legacy P4
# end: fatal
