let x = ٣;
print(x + 1);

# ---
# spec: 2.4
# title: a non-ASCII decimal digit (U+0663) in a number literal
# status: impl
# end: ok | syntax-error 1
# output: unchecked
# doc-tree: ok
# doc-vm: ok
# doc-c: ok
#> 4
