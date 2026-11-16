let i = 0;
while (i < 10) { try { if (i == 3) { break; } } catch (e) {} i = i + 1; }
let k = 0;
while (k < 10) { try { throw k; } catch (e) { if (e == 2) { break; } } k = k + 1; }
print(i, k);

# ---
# spec: 8.3 10.2
# title: break leaves a loop from inside try or catch
# end: ok
#> 3 2
