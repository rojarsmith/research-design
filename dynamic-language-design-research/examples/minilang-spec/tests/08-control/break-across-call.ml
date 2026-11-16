fn stop() { break; }
let i = 0;
while (i < 5) { print(i); if (i == 1) { stop(); } i = i + 1; }
print("done");

# ---
# spec: 8.3
# title: break in a called function stops the caller's loop
# level: full
# status: legacy P3
# end: ok
#> 0
#> 1
#> done
