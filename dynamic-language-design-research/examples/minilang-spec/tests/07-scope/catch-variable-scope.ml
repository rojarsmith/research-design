let saved = nil;
try { throw "caught value"; } catch (e) { saved = fn() { return e; }; }
print(saved());
print(e);

# ---
# spec: 7.2 10.2
# title: the catch variable belongs to the handler and can be captured
# end: runtime-error 4
# message: undefined variable 'e'
#> caught value
