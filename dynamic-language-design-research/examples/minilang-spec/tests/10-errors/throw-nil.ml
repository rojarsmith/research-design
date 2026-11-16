try { throw nil; } catch (e) { print(type(e), e); }

# ---
# spec: 10.6
# title: throw nil is caught as the string nil
# level: full
# status: legacy P1
# end: ok
#> string nil
