let a = []; push(a, a);
fn show() { print(a); }
try { show(); } catch (e) { print("caught: " + e); }
print("after");

# ---
# spec: 14.3 10.7
# title: printing a list that contains itself, inside a function, inside try
# status: impl
# end: ok | fatal
# output: unchecked
# doc-tree: ok
# doc-vm: fatal RecursionError
# doc-c: ok
