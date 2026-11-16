let x = 1; let i = 0; while (i < 4299) { x = x * 10; i = i + 1; }
let y = x * 10;
print(y > x);
try { print(y); } catch (e) { print("caught"); }

# ---
# spec: 14.2 10.7
# title: converting a 4,301-digit integer to text ends the program
# level: full
# status: legacy P5
# end: fatal
#> true
