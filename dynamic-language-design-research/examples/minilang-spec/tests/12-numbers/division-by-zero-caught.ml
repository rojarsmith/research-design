let n = 0;
try { 1 / 0; } catch (e) { n = n + 1; }
try { 1.5 / 0.0; } catch (e) { n = n + 1; }
try { 1 % 0; } catch (e) { n = n + 1; }
try { 2 / -0.0; } catch (e) { n = n + 1; }
print(n);

# ---
# spec: 12.3 12.4
# title: dividing by integer or float zero is a runtime error
# end: ok
#> 4
