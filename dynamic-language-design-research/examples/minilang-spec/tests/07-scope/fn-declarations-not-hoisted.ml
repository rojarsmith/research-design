print(f());
fn f() { return 1; }

# ---
# spec: 7.1
# title: a function declaration takes effect when it runs
# end: runtime-error 1
# message: undefined variable 'f'
