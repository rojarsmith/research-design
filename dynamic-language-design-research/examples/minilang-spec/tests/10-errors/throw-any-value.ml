let l = [1];
try { throw l; } catch (e) { print(e == l, type(e)); }
try { throw 42; } catch (e) { print(e + 1); }
try { throw fn() { return "from thrown fn"; }; } catch (e) { print(e()); }

# ---
# spec: 10.2
# title: any value can be thrown and is caught unchanged
# end: ok
#> true list
#> 43
#> from thrown fn
