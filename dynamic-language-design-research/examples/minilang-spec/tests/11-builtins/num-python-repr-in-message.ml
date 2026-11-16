try { num(nil); } catch (e) { print(e); }
try { num([nil, true, "a"]); } catch (e) { print(e); }
try { num(print); } catch (e) { print(e); }
try { num(fn() {}); } catch (e) { print(e); }

# ---
# spec: 11.6
# title: num's error message shows Python's repr of the value
# level: full
# status: legacy P6
# end: ok
#> num() cannot convert None
#> num() cannot convert [None, True, 'a']
#> num() cannot convert <builtin print>
#> num() cannot convert <fn <anonymous>>
