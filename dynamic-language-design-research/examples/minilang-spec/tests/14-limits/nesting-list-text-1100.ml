let a = []; let i = 0;
while (i < 1100) { a = [a]; i = i + 1; }
print(len(str(a)));

# ---
# spec: 14.3
# title: a list nested 1,100 deep
# status: impl
# end: ok | fatal
# output: unchecked
# doc-tree: ok
# doc-vm: ok
# doc-c: fatal RecursionError
#> 2202
