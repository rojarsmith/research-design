print(true and 1, nil or false, false or nil, 0 and "x", nil and undefined_name, 1 or undefined_name);

# ---
# spec: 6.2
# title: and/or return an operand and short-circuit
# end: ok
#> 1 false nil x nil 1
