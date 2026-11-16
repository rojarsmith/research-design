print(num("٣"), num("１２"));

# ---
# spec: 11.6
# title: num of non-ASCII decimal digits (U+0663, full-width 12)
# status: impl
# end: ok | runtime-error 1
# output: unchecked
# doc-tree: ok
# doc-vm: ok
# doc-c: runtime-error 1
#> 3 12
