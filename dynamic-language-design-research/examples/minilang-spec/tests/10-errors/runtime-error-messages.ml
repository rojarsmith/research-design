try { 1 / 0; } catch (e) { print(e); }
try { nope; } catch (e) { print(e); }
try { [][0]; } catch (e) { print(e); }

# ---
# spec: 10.3
# title: the caught string is the message without the line
# level: full
# end: ok
#> division by zero
#> undefined variable 'nope'
#> index 0 out of range for length 0
