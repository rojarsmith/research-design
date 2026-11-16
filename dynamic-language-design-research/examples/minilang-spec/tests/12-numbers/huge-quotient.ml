let x = 1; let i = 0; while (i < 400) { x = x * 10; i = i + 1; }
try { print(x / 10); } catch (e) { print("caught"); }

# ---
# spec: 12.3 10.7
# title: a quotient too large for a float ends the program
# level: full
# status: legacy P5
# end: fatal
