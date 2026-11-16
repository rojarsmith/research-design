fn stop() { break; }
let i = 0;
while (i < 5) { try { stop(); } catch (e) { print("not caught here"); } i = i + 1; }
print("out", i);

# ---
# spec: 8.3
# title: try does not stop a break that crosses a call
# level: full
# status: legacy P3
# end: ok
#> out 0
