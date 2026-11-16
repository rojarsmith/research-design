let x = 1; let i = 0; while (i < 400) { x = x * 10; i = i + 1; }
try { print(x + 0.5); } catch (e) { print("caught"); }

# ---
# spec: 12.3 10.7
# title: mixing an integer too large for a float with a float ends the program
# level: full
# status: legacy P5
# end: fatal
