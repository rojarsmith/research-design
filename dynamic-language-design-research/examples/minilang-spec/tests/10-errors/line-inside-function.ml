fn f() {
  return 1 / 0;
}
f();

# ---
# spec: 10.4
# title: an error inside a function is reported where it happens, not at the call
# end: runtime-error 2
# message: division by zero
