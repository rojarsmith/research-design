try { num("--5"); } catch (e) { print(e); }
try { num(""); } catch (e) { print(e); }
try { num("1__0"); } catch (e) { print(e); }
try { num("0x10"); } catch (e) { print(e); }
try { num("it's"); } catch (e) { print(e); }

# ---
# spec: 11.6 12.6
# title: text num rejects, quoted as in its message
# level: full
# end: ok
#> num() cannot convert '--5'
#> num() cannot convert ''
#> num() cannot convert '1__0'
#> num() cannot convert '0x10'
#> num() cannot convert "it's"
