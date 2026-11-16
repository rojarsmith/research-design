let x = 1; let i = 0; while (i < 400) { x = x * 10; i = i + 1; }
try { print(num(x)); } catch (e) { print("caught"); }

# ---
# spec: 11.6 10.7
# title: num of an integer too large for a float ends the program
# level: full
# status: legacy P5
# end: fatal
