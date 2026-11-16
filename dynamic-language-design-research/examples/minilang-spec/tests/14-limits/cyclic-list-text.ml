let a = []; push(a, a);
print("before");
print(a);

# ---
# spec: 14.3 10.7
# title: printing a list that contains itself, at top level
# status: impl
# end: fatal | runtime-error 3
# doc-tree: fatal RecursionError
# doc-vm: fatal RecursionError
# doc-c: fatal RecursionError
#> before
